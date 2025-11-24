#include <memory>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <string>
#include <limits>
#include <chrono>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/images/image.hpp"
#include "vira/utils/utils.hpp"
#include "vira/utils/print_utils.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    CPURasterizer<TSpectral, TFloat, TMeshFloat>::CPURasterizer(CPURasterizerOptions newOptions) :
        options{ newOptions }
    {

    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void CPURasterizer<TSpectral, TFloat, TMeshFloat>::render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene)
    {
        camera.initialize();

        scene.processSceneGraph();

        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point stop_time;
        if (vira::getPrintStatus()) {
            start_time = std::chrono::high_resolution_clock::now();
            std::cout << "Rasterizing...\n" << std::flush;
        }

        renderPasses.initializeImages(camera.getResolution());

        // Extract camera details:
        vira::images::Resolution resolution = camera.getResolution();
        mat4<TFloat> viewMatrix = camera.getViewMatrix();
        mat3<TFloat> cameraNormalMatrix = camera.getViewNormalMatrix();

        auto& lights = scene.light_cache_;

        // Loop over models in the tile:
        for (const auto& [meshID, meshData] : scene.meshes_) {
            auto& mesh = meshData.mesh;
            bool smoothShading = mesh->getSmoothShading();

            for (auto& instance_data : meshData.instances) {

                // Extract the instance information:
                vira::scene::Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_data.instance;
                mat4<TFloat> modelMatrix = instance->getModelMatrix();
                mat3<TFloat> normalMatrix = instance->getModelNormalMatrix();

                mat4<TFloat> model2camera = viewMatrix * modelMatrix;

                // Loop over material groups in the model:
                const std::vector<vira::geometry::Triangle<TSpectral, TMeshFloat>>& triangleBuffer = mesh->getTriangles();

                // Loop over triangles:
                size_t triangleID = 0;
                for (const vira::geometry::Triangle<TSpectral, TMeshFloat>& tri : triangleBuffer) {
                    triangleID++;

                    // Check if triangle is valid (TODO this shouldn't be necessary?)
                    if (std::isinf(tri.vert[0].position[0]) || std::isinf(tri.vert[1].position[0]) || std::isinf(tri.vert[2].position[0])) {
                        continue;
                    }

                    // Transform points from local model space to camera space:
                    std::array<vira::geometry::Vertex<TSpectral, TMeshFloat>, 3> vert_camera;
                    vert_camera[0].position = vira::transformPoint(model2camera, tri.vert[0].position);
                    vert_camera[1].position = vira::transformPoint(model2camera, tri.vert[1].position);
                    vert_camera[2].position = vira::transformPoint(model2camera, tri.vert[2].position);

                    // Determine if the points are behind the camera:
                    if (camera.behind(vert_camera[0].position) ||
                        camera.behind(vert_camera[1].position) ||
                        camera.behind(vert_camera[2].position)) {
                        continue;
                    }

                    // Project points to image space:
                    auto p0 = camera.projectCameraPoint(vert_camera[0].position);
                    auto p1 = camera.projectCameraPoint(vert_camera[1].position);
                    auto p2 = camera.projectCameraPoint(vert_camera[2].position);

                    // Calculate the search bounds:
                    int startX_f = static_cast<int>(std::floor(std::min(p0.x, std::min(p1.x, p2.x))));
                    int startY_f = static_cast<int>(std::floor(std::min(p0.y, std::min(p1.y, p2.y))));
                    int stopX_f = static_cast<int>(std::ceil(std::max(p0.x, std::max(p1.x, p2.x))));
                    int stopY_f = static_cast<int>(std::ceil(std::max(p0.y, std::max(p1.y, p2.y))));

                    // Image-space bounds of the triangle are outside of the image:
                    int resX = resolution.x;
                    int resY = resolution.y;
                    if ((stopX_f < 0) ||
                        (startX_f > resX) ||
                        (stopY_f < 0) ||
                        (startY_f > resY)) {
                        continue;
                    }

                    // Clamp bounds to the image space:
                    int startX = std::clamp(startX_f, 0, resX);
                    int startY = std::clamp(startY_f, 0, resY);
                    int stopX = std::clamp(stopX_f, 0, resX);
                    int stopY = std::clamp(stopY_f, 0, resY);

                    // Calculate the depth:
                    float d0 = static_cast<float>(length(vert_camera[0].position));
                    float d1 = static_cast<float>(length(vert_camera[1].position));
                    float d2 = static_cast<float>(length(vert_camera[2].position));

                    // Calculate size of the triangle:
                    float area = vira::geometry::edgeFunction(p0, p1, p2);
                    float e01 = length(p1 - p0);
                    float e02 = length(p2 - p0);
                    float e12 = length(p2 - p1);
                    float triangleSize = std::max(e01, std::max(e02, e12));

                    // Loop over triangle bounds:
                    for (int X = startX; X < stopX; X++) {
                        for (int Y = startY; Y < stopY; Y++) {

                            // Barycentric coordinates:
                            vec2<float> point{ X, Y };
                            std::array<TMeshFloat, 3> w;
                            w[0] = vira::geometry::edgeFunction(p1, p2, point) / area;
                            w[1] = vira::geometry::edgeFunction(p2, p0, point) / area;
                            w[2] = vira::geometry::edgeFunction(p0, p1, point) / area;

                            // If within a triangle:
                            if (w[0] >= 0 && w[1] >= 0 && w[2] >= 0) {
                                float previous_depth = renderPasses.depth(X, Y);
                                float current_depth = (w[0] * d0) + (w[1] * d1) + (w[2] * d2);

                                // Get the material:
                                vira::materials::Material<TSpectral>* material = mesh->material_cache_[tri.material_cache_index];

                                if (current_depth < previous_depth) {
                                    renderPasses.depth(X, Y) = current_depth;

                                    // Compute uv-coordinates:
                                    vec2<float> uv =
                                        static_cast<float>(w[0]) * tri.vert[0].uv +
                                        static_cast<float>(w[1]) * tri.vert[1].uv +
                                        static_cast<float>(w[2]) * tri.vert[2].uv;

                                    TSpectral vertAlbedo =
                                        static_cast<float>(w[0]) * tri.vert[0].albedo +
                                        static_cast<float>(w[1]) * tri.vert[1].albedo +
                                        static_cast<float>(w[2]) * tri.vert[2].albedo;

                                    TSpectral albedo = vertAlbedo * material->getAlbedo(uv);

                                    // Transform normals to world frame:
                                    vec3<float> N_body = tri.face_normal;
                                    if (smoothShading) {
                                        N_body = normalize(
                                            static_cast<float>(w[0]) * tri.vert[0].normal +
                                            static_cast<float>(w[1]) * tri.vert[1].normal +
                                            static_cast<float>(w[2]) * tri.vert[2].normal
                                        );
                                    }
                                    vec3<float> N_global = normalMatrix * N_body;

                                    // Construct the tangent and bitangent vectors
                                    vec3<float> arbitraryVec = (std::abs(N_global[0]) < 0.9f) ? vec3<float>(1, 0, 0) : vec3<float>(0, 1, 0);
                                    vec3<float> tangent = normalize(cross(arbitraryVec, N_global));
                                    vec3<float> bitangent = cross(N_global, tangent);

                                    // Create the tangent-to-world transformation matrix
                                    // [tangent bitangent normal] transforms from tangent space to world space
                                    mat3<float> tangentToWorld = mat3<float>(
                                        tangent[0], tangent[1], tangent[2],      // Row 1: tangent
                                        bitangent[0], bitangent[1], bitangent[2], // Row 2: bitangent  
                                        N_global[0], N_global[1], N_global[2] // Row 3: normal
                                    );

                                    // Sample normal map:
                                    N_global = material->getNormal(uv, N_global, tangentToWorld);

                                    // Transform to camera frame:
                                    vec3<float> N_camera = cameraNormalMatrix * N_global;


                                    DataPayload<TSpectral, TFloat> dataPayload(X, Y);
                                    dataPayload.triangle_id = triangleID;
                                    dataPayload.mesh_id = meshID.id();
                                    dataPayload.instance_id = instance->getID().id();
                                    dataPayload.material_id = material->getID().id();
                                    dataPayload.triangle_size = triangleSize;
                                    dataPayload.depth = current_depth;
                                    dataPayload.albedo = albedo;

                                    vec3<TFloat> frag_camera =
                                        w[0] * vert_camera[0].position +
                                        w[1] * vert_camera[1].position +
                                        w[2] * vert_camera[2].position;

                                    vec3<TFloat> frag_global = camera.localToGlobal(frag_camera);

                                    if (renderPasses.save_velocity) {
                                        vec3<TFloat> frag_local = instance->globalToLocal(frag_global);
                                        dataPayload.velocity_global = instance->localPointToGlobalVelocity(frag_local);
                                        vec3<TFloat> relative_velocity_global = dataPayload.velocity_global - camera.localPointToGlobalVelocity(frag_global);
                                        dataPayload.velocity_camera = camera.globalDirectionToLocal(relative_velocity_global);
                                    }

                                    if (renderPasses.simulate_lighting) {
                                        vec3<float> V_global = normalize(-frag_global);

                                        // Evaluate material:
                                        TSpectral fragRadiance{ 0 };
                                        for (auto& light : lights) {
                                            Ray<TSpectral, TFloat> sample_ray;
                                            float distance;
                                            float lightPDF;
                                            // TODO get the current light transformState:
                                            TSpectral radiance = light->sample(frag_global, sample_ray, distance, lightPDF);
                                            vec3<TFloat>& L_global = sample_ray.direction;
                                            TSpectral bsdfValue = material->evaluateBSDF(uv, N_global, L_global, V_global, albedo) / lightPDF;
                                            fragRadiance += radiance * bsdfValue;
                                        }

                                        dataPayload.total_radiance = fragRadiance;
                                    }

                                    dataPayload.normal_camera = N_camera;
                                    dataPayload.normal_global = N_global;
                                    dataPayload.count = 1;

                                    // Update additional buffers:
                                    renderPasses.updateImages(dataPayload);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Restore light transforms:
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
            std::cout << vira::print::VIRA_INDENT + "Completed (" << duration.count() << " ms)\n";
        }
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool CPURasterizer<TSpectral, TFloat, TMeshFloat>::inFrame(Pixel& p, vira::images::Resolution& resolution)
    {
        return (p[0] > 0 && p[0] < static_cast<float>(resolution.x) && p[1] > 0 && p[1] < static_cast<float>(resolution.y));
    };
};
