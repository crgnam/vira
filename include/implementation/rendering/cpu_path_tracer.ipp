#include <memory>
#include <limits>
#include <random>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/math.hpp"
#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/lights/light.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/materials/material.hpp"
#include "vira/images/image.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"
#include "vira/rendering/cpu_denoise.hpp"
#include "vira/utils/utils.hpp"
#include "vira/utils/print_utils.hpp"

#include "vira/debug.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void CPUPathTracer<TSpectral, TFloat, TMeshFloat>::render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene)
    {
        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)

        // Build the acceleration structure:
        camera.initialize();

        scene.buildTLAS();

        // Begin path tracing:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point stop_time;
        if (vira::getPrintStatus()) {
            std::cout << "Pathtracing...\n" << std::flush;
            start_time = std::chrono::high_resolution_clock::now();
        }

        renderPasses.initializeImages(camera.getResolution());

        // Initialize:
        vira::images::Resolution resolution = camera.getResolution();

        // Progress tracking seutp:
        const int64_t totalPixels = static_cast<int64_t>(resolution.x) * resolution.y;
        std::atomic<int64_t> processedPixels{ 0 };
        std::atomic<int64_t> lastReportedProgress{ 0 };
        const int64_t percentageUpdate = 10;
        const int64_t progressUpdateInterval = std::max(percentageUpdate, totalPixels / percentageUpdate); // Update every 10%

        std::unique_ptr<indicators::ProgressBar> progressBar;
        if (vira::getPrintStatus()) {
            progressBar = vira::print::makeProgressBar("Rendering");
        }

        // Begin ray-tracing:
        if (options.tracingType == UNIDIRECTIONAL) {
            tbb::parallel_for(tbb::blocked_range2d<int>(0, resolution.y, 0, resolution.x), [&](const tbb::blocked_range2d<int>& r) {
                // Initialize RNG once per thread (avoiding using std::random_device because of entropy pool depletion
                static thread_local std::mt19937 rng = []() {
                    auto seed = std::hash<std::thread::id>{}(std::this_thread::get_id()) ^ static_cast<size_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
                    return std::mt19937(static_cast<std::mt19937::result_type>(seed));
                    }();
                std::uniform_real_distribution<float> distribution(0.f, 1.f);

                scene.initializeTLASThreads();

                // Local counter for batch updates
                int localPixelCount = 0;
                const int batchSize = 64;

                for (int i = static_cast<int>(r.cols().begin()), i_end = static_cast<int>(r.cols().end()); i < i_end; i++) {
                    for (int j = static_cast<int>(r.rows().begin()), j_end = static_cast<int>(r.rows().end()); j < j_end; j++) {
                        DataPayload<TSpectral, TFloat> dataPayload(i, j);

                        // Perform unidirectional (backwards) path tracing:
                        dataPayload.total_radiance = this->unidirectional(camera, scene, dataPayload, rng, distribution);

                        vira::debug::check_no_nan(dataPayload.total_radiance, "NaN detected in unidirectional path tracing");

                        // Update additional buffers:
                        renderPasses.updateImages(dataPayload);

                        // Progress tracking:
                        if (vira::getPrintStatus() && progressBar && (++localPixelCount % batchSize == 0)) {
                            int64_t newProcessed = processedPixels.fetch_add(batchSize, std::memory_order_relaxed) + batchSize;

                            // Check if we should update progress display (reduce frequency of expensive operations)
                            if (newProcessed - lastReportedProgress.load(std::memory_order_relaxed) >= progressUpdateInterval) {
                                int64_t expected = lastReportedProgress.load();
                                if (lastReportedProgress.compare_exchange_weak(expected, newProcessed, std::memory_order_relaxed)) {
                                    float percentage = (static_cast<float>(newProcessed) / static_cast<float>(totalPixels)) * 100.0f;
                                    vira::print::updateProgressBar(progressBar, percentage);
                                }
                            }
                            localPixelCount = 0; // Reset local counter
                        }
                    }
                }

                // Handle remaining pixels in the batch
                if (vira::getPrintStatus() && progressBar && localPixelCount > 0) {
                    int64_t newProcessed = processedPixels.fetch_add(localPixelCount, std::memory_order_relaxed) + localPixelCount;
                    int64_t expected = lastReportedProgress.load();
                    if (newProcessed - expected >= progressUpdateInterval &&
                        lastReportedProgress.compare_exchange_weak(expected, newProcessed, std::memory_order_relaxed)) {
                        float percentage = (static_cast<float>(newProcessed) / static_cast<float>(totalPixels)) * 100.0f;
                        vira::print::updateProgressBar(progressBar, percentage);
                    }
                }
                });
        }
        else {
            throw std::runtime_error("Invalid path tracing type selected");
        }

        if (options.denoise) {
            vira::images::Image<TSpectral>& albedo = renderPasses.albedo;
            vira::images::Image<float>& depth = renderPasses.depth;
            vira::images::Image<TSpectral>& direct = renderPasses.direct_radiance;
            vira::images::Image<TSpectral>& indirect = renderPasses.indirect_radiance;
            vira::images::Image<vec3<float>>& normal = renderPasses.normal_global;

            denoiseSpectralRadianceEATWT<TSpectral>(direct, indirect, albedo, depth, normal, denoiserOptions);

            renderPasses.total_radiance = direct + indirect;
        }

        // Convert radiance to received power:
        if (renderPasses.simulate_lighting) {

            tbb::parallel_for(tbb::blocked_range2d<int>(0, resolution.y, 0, resolution.x), [&](const tbb::blocked_range2d<int>& r) {
                for (int i = static_cast<int>(r.cols().begin()), i_end = static_cast<int>(r.cols().end()); i < i_end; i++) {
                    for (int j = static_cast<int>(r.rows().begin()), j_end = static_cast<int>(r.rows().end()); j < j_end; j++) {
                        renderPasses.received_power(i, j) = camera.calculateReceivedPower(renderPasses.total_radiance(i, j), i, j);
                    }
                }
                });
        }

        if (camera.hasPSF()) {
            // TODO Use maxReceivedPower and minimumPower:
            TSpectral maxReceivedPower{ 0 };
            float minimumPower = 0;
            vira::images::Image<TSpectral> kernel = camera.psf().getKernel(maxReceivedPower, minimumPower);
            renderPasses.received_power.convolve(kernel, true);
        }

        if (vira::getPrintStatus()) {
            stop_time = std::chrono::high_resolution_clock::now();
            auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
            vira::print::updateProgressBar(progressBar, "Completed (" + std::to_string(duration.count()) + " ms)", 100.0f);
        }
    };




    // =================================== //
    // === Path Tracer Implementations === //
    // =================================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral CPUPathTracer<TSpectral, TFloat, TMeshFloat>::unidirectional(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene,
        DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float s1 = 0.f;
        float s2 = 0.f;

        TSpectral radiance{ 0 };

        for (dataPayload.sample = 0; dataPayload.sample < options.samples; dataPayload.sample++) {

            // TODO Implement stratified ray sampling:
            Ray<TSpectral, TFloat> ray;
            float X = static_cast<float>(dataPayload.i);
            float Y = static_cast<float>(dataPayload.j);
            if (dataPayload.sample == 0) {
                ray = camera.pixelToRay(Pixel(X, Y));
            }
            else {
                // Uniformly sample a ray from the current pixel:
                float di = dist(rng);
                float dj = dist(rng);
                ray = camera.pixelToRay(Pixel(X + di, Y + dj), rng, dist);
            }

            TSpectral path_radiance = simulatePath(camera, scene, ray, dataPayload, rng, dist);
            radiance += path_radiance;

            // TODO Improve adaptive sampling:
            if (options.adaptive_sampling) {
                // This algorithm is from: https://cs184.eecs.berkeley.edu/sp23/docs/proj3-1-part-5

                float illum = path_radiance.magnitude();
                s1 += illum;
                s2 += (illum * illum);

                if (dataPayload.sample % options.samples_to_detect_miss == 0 && dataPayload.sample != 0) {
                    if (dataPayload.depth == 0) {
                        break;
                    }
                }

                if (dataPayload.sample % options.samples_per_batch == 0 && dataPayload.sample != 0) {
                    if (s1 == 0) {
                        break;
                    }

                    float n = static_cast<float>(dataPayload.sample) + 1.f;
                    float mu = s1 / n;
                    float variance = (1.f / (n - 1.f)) * (s2 - (s1 * s1) / n);

                    float I = 1.96f * std::sqrt(variance / n);

                    if (I <= options.sampling_tolerance * mu) {
                        break;
                    }
                }
            }
        }

        float samples_f = static_cast<float>(dataPayload.sample);
        radiance = radiance / samples_f;

        // Update individual components of the radiance contributions:
        dataPayload.direct_radiance = dataPayload.direct_radiance / samples_f;
        dataPayload.indirect_radiance = radiance - dataPayload.direct_radiance;
        dataPayload.albedo = dataPayload.albedo / samples_f;

        mat3<TFloat> normalMatrix = camera.getViewNormalMatrix();
        dataPayload.normal_global = normalize(dataPayload.normal_global);
        dataPayload.normal_camera = normalize(normalMatrix * dataPayload.normal_global);

        return radiance;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral CPUPathTracer<TSpectral, TFloat, TMeshFloat>::simulatePath(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene,
        Ray<TSpectral, TFloat>& ray, DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        const auto& background = scene.getBackgroundEmission();

        TSpectral pathRadiance{ 0 };
        dataPayload.throughput = TSpectral{ 1 };
        for (dataPayload.bounce = 0; dataPayload.bounce < options.bounces + 1; dataPayload.bounce++) {
            // Perform ray intersection:
            scene.intersect(ray);

            if (std::isinf(ray.hit.t)) {
                if (renderPasses.simulate_lighting && options.show_background) {
                    float x = static_cast<float>(ray.direction.x);
                    float y = static_cast<float>(ray.direction.y);
                    float z = static_cast<float>(ray.direction.z);

                    UV uv{ std::atan2(y, x) / (2 * PI<float>()), std::atan2(std::sqrt(x * x + y * y), z) / PI<float>() };

                    TSpectral backgroundRadiance = background.sampleUVs(uv);

                    float weight = 1.f;
                    pathRadiance += dataPayload.throughput * (backgroundRadiance * weight);
                }
                break;
            }
            else {
                pathRadiance += processIntersection(camera, scene, ray, dataPayload, rng, dist);
            }
        }

        return pathRadiance;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral CPUPathTracer<TSpectral, TFloat, TMeshFloat>::processIntersection(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene,
        Ray<TSpectral, TFloat>& ray, DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        // Extract the Mesh pointer:
        vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>* mesh = ray.hit.template getMeshPtr<TMeshFloat>();

        // Obtain the Material pointer:
        vira::materials::Material<TSpectral>* material = mesh->material_cache_[ray.hit.material_cache_index];
        vira::scene::Instance<TSpectral, TFloat, TMeshFloat>* instance = ray.hit.template getInstancePtr<TMeshFloat>();

        // Obtain transformation values:
        mat4<TFloat> model_matrix = instance->getModelMatrix();
        mat3<TFloat> normal_matrix = instance->getModelNormalMatrix();

        // Obtain references for easier access:
        const vec3<float>& V_global = -ray.direction;
        const std::array<TFloat, 3>& w = ray.hit.w;
        const std::array<vira::geometry::Vertex<TSpectral, TFloat>, 3>& vert = ray.hit.vert;

        // Compute UV coordinates:
        vec2<float> uv = static_cast<float>(w[0]) * vert[0].uv +
            static_cast<float>(w[1]) * vert[1].uv +
            static_cast<float>(w[2]) * vert[2].uv;

        // Compute the Normal vector:
        vec3<float> N_local = ray.hit.face_normal;
        if (mesh->getSmoothShading()) {
            N_local = normalize(
                static_cast<float>(w[0]) * vert[0].normal +
                static_cast<float>(w[1]) * vert[1].normal +
                static_cast<float>(w[2]) * vert[2].normal
            );
        }
        vec3<float> N_global = normal_matrix * N_local;

        // Construct the tangent and bitangent vectors
        vec3<float> arbitraryVec = (std::abs(N_global[0]) < 0.9f) ? vec3<float>(1, 0, 0) : vec3<float>(0, 1, 0);
        vec3<float> tangent = normalize(cross(arbitraryVec, N_global));
        vec3<float> bitangent = cross(N_global, tangent);

        // Create the tangent-to-world transformation matrix
        // [tangent bitangent normal] transforms from tangent space to world space
        mat3<float> tangent_to_global = mat3<float>(
            tangent[0], tangent[1], tangent[2],      // Row 1: tangent
            bitangent[0], bitangent[1], bitangent[2], // Row 2: bitangent  
            N_global[0], N_global[1], N_global[2] // Row 3: normal
        );

        // Sample normal map:
        N_global = material->getNormal(uv, N_global, tangent_to_global);

        // Get the albedo:
        TSpectral vertAlbedo =
            static_cast<float>(w[0]) * vert[0].albedo +
            static_cast<float>(w[1]) * vert[1].albedo +
            static_cast<float>(w[2]) * vert[2].albedo;

        TSpectral albedo = vertAlbedo * material->getAlbedo(uv);

        // Compute intersection:
        vec3<TFloat> intersection_local =
            w[0] * vert[0].position +
            w[1] * vert[1].position +
            w[2] * vert[2].position;

        // Update intersection point (to avoid terminator problem):
        intersection_local = computeShadingPoint(intersection_local, vert, w, N_local);

        // Transform intersection into global space:
        vec3<TFloat> intersection_global = transformPoint(model_matrix, intersection_local);

        // Update datapayload with meta data:
        if (dataPayload.bounce == 0) {
            if (dataPayload.first_hit) {
                dataPayload.triangle_id = ray.hit.tri_id;
                dataPayload.mesh_id = mesh->getID().id();
                dataPayload.instance_id = instance->getID().id();
                dataPayload.material_id = material->getID().id();

                if (renderPasses.save_triangle_size) {
                    TFloat e01 = length(vert[1].position - vert[0].position);
                    TFloat e02 = length(vert[2].position - vert[0].position);
                    TFloat e12 = length(vert[2].position - vert[1].position);
                    TFloat edgeSize = std::max(e01, std::max(e02, e12));

                    TFloat distance = length(intersection_global - camera.getGlobalPosition());
                    dataPayload.triangle_size = static_cast<float>(edgeSize / camera.calculateGSD(distance));
                }

                if (renderPasses.save_velocity) {
                    dataPayload.velocity_global = instance->localPointToGlobalVelocity(intersection_local);
                    vec3<TFloat> relative_velocity_global = dataPayload.velocity_global - camera.localPointToGlobalVelocity(intersection_global);
                    dataPayload.velocity_camera = camera.globalDirectionToLocal(relative_velocity_global);
                }

                dataPayload.first_hit = false;
            }
            dataPayload.albedo += albedo;
            dataPayload.normal_global += N_global;
            dataPayload.depth += ray.hit.t;
            dataPayload.count++;
        }

        // Simulate Lighting:
        TSpectral radiance{ 0 };
        if (renderPasses.simulate_lighting) {
            intersection_global = offsetIntersection<TMeshFloat>(intersection_global, normal_matrix * ray.hit.face_normal);
            auto& lights = scene.light_cache_;

            // AMBIENT LIGHTING:
            if (dataPayload.bounce == 0 && scene.hasAmbient()) {
                TSpectral ambient = scene.getAmbient();

                // Get radiance contribution from ambient lighting:
                TSpectral ambient_contribution = material->applyAmbient(ambient, albedo, uv);
                radiance += dataPayload.throughput * ambient_contribution;
            }

            // DIRECT LIGHTING (Light Sampling):
            for (auto& light : lights) {
                Ray<TSpectral, TFloat> sample_ray;
                float light_pdf = 0;
                float distance;

                // Sample the light source
                TSpectral light_radiance = light->sample(intersection_global, sample_ray, distance, light_pdf, rng, dist);

                if (light_pdf > 0) {
                    // Check for shadows
                    scene.intersect(sample_ray);

                    if (sample_ray.hit.t > distance) { // Not in shadow
                        vec3<TFloat> L_global = sample_ray.direction;

                        // Evaluate BSDF for this light direction
                        TSpectral bsdfValue = material->evaluateBSDF(uv, N_global, L_global, V_global, albedo);
                        float cos_theta = std::max(0.0f, dot(L_global, N_global));

                        // Get the material PDF for this same direction
                        float material_pdf = material->getPDF(V_global, N_global, L_global, tangent_to_global, uv);

                        // Calculate MIS weight using power heuristic
                        float weight = PowerHeuristic(1, light_pdf, 1, material_pdf);

                        // Add contribution
                        radiance += dataPayload.throughput * bsdfValue * light_radiance * cos_theta * weight / light_pdf;
                    }
                }
            }

            // INDIRECT LIGHTING (Material/BSDF Sampling):
            if (dataPayload.bounce < options.bounces) {
                float material_pdf = 0;

                // Sample a direction from the material BSDF
                vec3<TFloat> direction = material->sampleDirection(V_global, N_global, tangent_to_global, uv, material_pdf, rng, dist);

                if (material_pdf > 0) {
                    // Create new ray for the sampled direction
                    Ray<TSpectral, TFloat> new_ray = Ray<TSpectral, TFloat>(intersection_global, direction);

                    // Evaluate BSDF for the sampled direction
                    TSpectral bsdf_value = material->evaluateBSDF(uv, N_global, direction, V_global, albedo);
                    float cos_theta = std::max(0.0f, dot(direction, N_global));

                    // Calculate light PDF for this direction (for MIS)
                    float light_pdf = 0;
                    for (auto& light : lights) {
                        light_pdf += light->getPDF(intersection_global, direction);
                    }
                    light_pdf /= static_cast<float>(lights.size()); // Average over all lights

                    // Calculate MIS weight
                    float weight = PowerHeuristic(1, material_pdf, 1, light_pdf);

                    // Update throughput for next bounce
                    dataPayload.throughput *= bsdf_value * cos_theta * weight / material_pdf;

                    // Continue tracing
                    ray = new_ray;
                }
            }

            if (dataPayload.bounce == 0) {
                dataPayload.direct_radiance += radiance;
            }
        }

        return radiance;
    };

    // TODO Rework with MIS bugs (and consider moving if it is kept)
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float CPUPathTracer<TSpectral, TFloat, TMeshFloat>::PowerHeuristic(int numf, float fPdf, int numg, float gPdf) {
        float f = static_cast<float>(numf) * fPdf;
        float g = static_cast<float>(numg) * gPdf;

        return (f * f) / (f * f + g * g);
    }

    // The following two functions are adapted from: https://gist.github.com/pixnblox/5e64b0724c186313bc7b6ce096b08820
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> CPUPathTracer<TSpectral, TFloat, TMeshFloat>::projectOnPlane(vec3<TFloat> position, vec3<TFloat> origin, Normal normal)
    {
        if constexpr (std::same_as<TFloat, double>) {
            vec3<TFloat> n{ normal[0], normal[1], normal[2] };
            return position - dot(position - origin, n) * n;
        }
        else {
            return position - dot(position - origin, normal) * normal;
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> CPUPathTracer<TSpectral, TFloat, TMeshFloat>::computeShadingPoint(const vec3<TFloat>& intersection, const std::array<vira::geometry::Vertex<TSpectral, TFloat>, 3>& vert, const std::array<TFloat, 3>& w, const vec3<float>& shadingNormal)
    {
        // Project the geometric position (inside the triangle) to the planes defined by the vertex
        // positions and normals.
        vec3<TFloat> p0 = projectOnPlane(intersection, vert[0].position, vert[0].normal);
        vec3<TFloat> p1 = projectOnPlane(intersection, vert[1].position, vert[1].normal);
        vec3<TFloat> p2 = projectOnPlane(intersection, vert[2].position, vert[2].normal);

        // Interpolate the projected positions using the barycentric coordinates, which gives the
        // shading position.
        vec3<TFloat> shadingPoint = p0 * w[0] + p1 * w[1] + p2 * w[2];

        // Return the shading position for a convex triangle, where the shading point is above the
        // triangle based on the shading normal. Otherwise use the geometric position.
        vec3<float> diff = shadingPoint - intersection;
        bool convex = dot(diff, shadingNormal) > 0;
        return convex ? shadingPoint : intersection;
    };
};