#include <cstddef>
#include <vector>
#include <memory>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/unresolved/unresolved_object.hpp"
#include "vira/unresolved/star_light.hpp"
#include "vira/images/resolution.hpp"
#include "vira/images/image.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat>::CPUUnresolvedRenderer(CPUUnresolvedRendererOptions newOptions) :
        options{ newOptions }
    {

    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat>::render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene)
    {
        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)

        scene.processSceneGraph();

        // Get the stars and unresolved objects:
        const std::vector<vira::unresolved::StarLight<TSpectral, TFloat>>& stars = scene.getStarLight();

        // Consolidate all positional and irradiance data:
        size_t numStars = stars.size();
        size_t numUnresolvedObjs = scene.unresolved_cache_.size();
        size_t numUnresolved = numStars + numUnresolvedObjs;

        if (numUnresolved == 0) {
            return;
        }

        camera.initialize();

        // Begin path tracing:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point stop_time;
        if (vira::getPrintStatus()) {
            std::cout << "Unresolved Rendering...\n" << std::flush;
            start_time = std::chrono::high_resolution_clock::now();
        }

        renderPasses.unresolved_power = vira::images::Image<TSpectral>(camera.getResolution(), TSpectral{ 0 });

        // Computed to avoid processing objects too dim to see:
        float minimumIrradiance = camera.computeMinimumDetectableIrradiance();
        float minimumPower = minimumIrradiance * camera.getExposureTime();

        // Extract camera information:
        mat4<TFloat> camera_view_matrix = camera.getViewMatrix();
        mat3<TFloat> camera_view_normal = camera.getViewNormalMatrix();
        vira::images::Resolution resolution = camera.getResolution();

        Rotation<TFloat> icrf_to_scene{ 1 };
        vec3<TFloat> scene_velocity_ssb{ 0,0,0 };

        // If we are rendering Stars, we need ICRF/SSB data:
        if (numStars > 0) {
            icrf_to_scene = scene.getRotationFromICRF();
            scene_velocity_ssb = scene.getSSBVelocityInICRF();
        }


        std::vector<vec3<TFloat>> vectorsToObjects(numUnresolved);
        std::vector<TSpectral> irradiances(numUnresolved);

        tbb::parallel_for(tbb::blocked_range<size_t>(0, numUnresolved),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {

                    if (i < numStars) {
                        // This entry is a star

                        // This does NOT account for stellar parallax.  Tycho2 does not provide distance information from SSB required to
                        // compute that, so I have not yet figured out how to actually implement that in a reasonable way.                        

                        irradiances[i] = stars[i].getIrradiance();
                        vec3<TFloat> icrf_direction = stars[i].getICRFDirection();

                        vec3<TFloat> global_direction = icrf_to_scene * icrf_direction;

                        if (options.simulate_stellar_aberration) {
                            // TODO Compute stellar aberration
                            //vec3<TFloat> camera_velocity = camera.getGlobalVelocity();
                            //vec3<TFloat> camera_velocity_in_ssb = camera_velocity + scene_velocity_ssb;
                        }

                        // Transform global direction into camera frame using View Normal Matrix
                        vectorsToObjects[i] = camera_view_normal * global_direction;

                    }
                    else {
                        // This is entry is an unresolved object
                        size_t unresolvedIdx = i - numStars;
                        irradiances[i] = scene.unresolved_cache_[unresolvedIdx]->getIrradiance();
                        vec3<TFloat> global_position = scene.unresolved_cache_[unresolvedIdx]->getGlobalPosition();

                        if (options.simulate_stellar_aberration) {
                            // TODO Compute stellar aberration
                            //vec3<TFloat> global_velocity = scene.unresolved_cache_[unresolvedIdx]->getGlobalVelocity();
                            //vec3<TFloat> camera_velocity = camera.getGlobalVelocity();

                            if (options.simulate_light_time_correction) {
                                // TODO Compute light-time correction:
                                //vec3<TFloat> camera_position = camera.getGlobalPosition();
                            }
                        }

                        vectorsToObjects[i] = vec3<TFloat>{ camera_view_matrix * vec4<TFloat>{global_position, 1.f} };
                    }
                }
            }
        );


        // Process stars in parallel:
        int tileSize = 512; // TODO FIND A BETTER VALUE FOR THIS
        int numTilesX = (resolution.x + tileSize - 1) / tileSize;
        int numTilesY = (resolution.y + tileSize - 1) / tileSize;

        tbb::parallel_for(
            tbb::blocked_range2d<int>(0, numTilesY, 0, numTilesX),
            [&](const tbb::blocked_range2d<int>& r) {
                // Process each tile in the range
                for (int y = r.rows().begin(); y < r.rows().end(); y++) {
                    for (int x = r.cols().begin(); x < r.cols().end(); x++) {
                        // Calculate tile boundaries with overlap
                        int x0 = std::max(0, x * tileSize);
                        int y0 = std::max(0, y * tileSize);
                        int x1 = std::min(resolution.x, (x + 1) * tileSize);
                        int y1 = std::min(resolution.y, (y + 1) * tileSize);
                        vira::images::ROI roi(x0, y0, x1, y1, vira::images::ROI_CORNERS);
                        processRegion(roi, camera, vectorsToObjects, irradiances, minimumPower);
                    }
                }
            }
        );

        if (vira::getPrintStatus()) {
            stop_time = std::chrono::high_resolution_clock::now();
            auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
            std::cout << vira::print::VIRA_INDENT << "Completed (" << duration.count() << " ms)\n" << std::flush;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<ProjectedPoint<TSpectral>> CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat>::findPointsInRegion(const vira::images::ROI& roi, const vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, const std::vector<vira::vec3<TFloat>>& vectors, const std::vector<TSpectral>& irradiances, float minimumPower)
    {
        std::vector<ProjectedPoint<TSpectral>> pointsInRegion;

        for (size_t i = 0; i < vectors.size(); ++i) {
            ProjectedPoint<TSpectral> projectedPoint;
            projectedPoint.point = camera.projectCameraPoint(vectors[i]);

            float radius = 41;
            bool inRangeX = ((projectedPoint.point.x + radius) >= static_cast<float>(roi.x0)) && ((projectedPoint.point.x - radius) < static_cast<float>(roi.x1));
            bool inRangeY = ((projectedPoint.point.y + radius) >= static_cast<float>(roi.y0)) && ((projectedPoint.point.y - radius) < static_cast<float>(roi.y1));
            if (inRangeX && inRangeY) {
                projectedPoint.received_power = camera.calculateReceivedPowerIrr(irradiances[i]);

                if (projectedPoint.received_power.magnitude() > minimumPower) {
                    pointsInRegion.push_back(projectedPoint);
                }
            }
        }

        return pointsInRegion;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat>::processRegion(const vira::images::ROI& roi, vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, const std::vector<vira::vec3<TFloat>>& vectors, const std::vector<TSpectral>& irradiances, float minimumPower)
    {
        std::vector<ProjectedPoint<TSpectral>> points = findPointsInRegion(roi, camera, vectors, irradiances, minimumPower);

        Pixel regionOrigin{ roi.x0, roi.y0 };

        for (const auto& point : points) {
            Pixel absolutePoint = point.point;

            if (camera.hasPSF()) {
                const vira::images::Image<TSpectral>& kernel = camera.psf().getResponse(point.received_power, minimumPower);

                renderPasses.unresolved_power.addImage(kernel, absolutePoint, roi);
            }
            else {
                if (static_cast<int>(absolutePoint.x) >= roi.x0 && static_cast<int>(absolutePoint.x) < roi.x1 &&
                    static_cast<int>(absolutePoint.y) >= roi.y0 && static_cast<int>(absolutePoint.y) < roi.y1) {
                    renderPasses.unresolved_power(absolutePoint) = point.received_power;
                }
            }
        }
    };
};