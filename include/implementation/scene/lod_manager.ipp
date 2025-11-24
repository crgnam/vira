#include <stdexcept>
#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <array>

#include "vira/math.hpp"
#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/scene.hpp"
#include "vira/images/image.hpp"
#include "vira/rendering/cpu_rasterizer.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/utils/utils.hpp"

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void LevelOfDetailManager<TSpectral, TFloat, TMeshFloat>::update(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene)
    {
        scene.processSceneGraph();

        // TODO Allow other configurations:
        updateByMapping(camera, scene);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void LevelOfDetailManager<TSpectral, TFloat, TMeshFloat>::updateByMapping(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene)
    {
        camera.initialize();

        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point stop_time;
        if (vira::getPrintStatus()) {
            start_time = std::chrono::high_resolution_clock::now();
            std::cout << "Computing New LoD...\n" << std::flush;
        }

        // Extract camera details:
        mat4<TFloat> view_matrix = camera.getViewMatrix();

        // Get the lights:
        auto& lights = scene.light_cache_;

        size_t num_to_update = 0;
        float max_view_angle = DEG2RAD<float>() * options.max_view_angle;

        std::vector<std::array<vira::rendering::Plane<TFloat>, 6>> shadow_frustums;

        // Determine direct visibility:
        for (auto& [meshID, mesh_data] : scene.meshes_) {
            auto& mesh = mesh_data.mesh;

            // Get the mesh bounding box:
            vira::rendering::AABB<TSpectral, TFloat> aabb = mesh->getAABB();

            // Get the visibility cone:
            float cone_angle = DEG2RAD<float>() * options.cone_angle;
            if (cone_angle == 0) {
                cone_angle = mesh->getConeAngle();
            }
            vec3<float> N_local = mesh->getNormal();

            // Determine which instances are visible:
            for (auto& instance_data : mesh_data.instances) {
                instance_data.visibility = false;
                Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_data.instance;

                vec3<TFloat> view_vector = camera.getGlobalPosition() - instance->getGlobalPosition();

                // Check if view vector is within the normal cone:
                vec3<float> V_global = normalize(view_vector);
                vec3<float> N_global = instance->localDirectionToGlobal(N_local);

                // Determine if instance is facing a light:
                if (options.check_shadows) {
                    instance_data.casting_shadow = false;
                    for (auto& light : lights) {
                        vec3<float> L_global = normalize(instance->getGlobalPosition() - light->getGlobalPosition());
                        float viewAngle = std::acos(dot(L_global, N_global)) - cone_angle;
                        if (viewAngle < 90) {
                            instance_data.casting_shadow = true;
                            break;
                        }
                    }
                }

                // Determine if instance is facing the camera:
                float view_angle = std::acos(dot(V_global, N_global)) - cone_angle;
                if (view_angle >= max_view_angle) {
                    continue;
                }

                // Compute the OBB:
                mat4<TFloat> local_to_camera = view_matrix * instance->getModelMatrix();
                vira::rendering::OBB<TFloat> obb = aabb.toOBB(local_to_camera);

                // Check if the OBB is within the view frustum:
                if (!camera.obbInView(obb)) {
                    continue;
                }

                instance_data.visibility = true;

                // Compute the shadow frustum:
                if (options.check_shadows) {
                    for (auto& light : lights) {
                        std::array<vira::rendering::Plane<TFloat>, 6> frustum_planes = makeShadowFrustum(obb, light->getGlobalPosition());
                        shadow_frustums.push_back(frustum_planes);
                    }
                }
            }
        }

        // Determine indirect (shadow) visibility:
        if (options.check_shadows) {
            for (auto& [meshID, mesh_data] : scene.meshes_) {
                auto& mesh = mesh_data.mesh;

                // Get the mesh bounding box:
                vira::rendering::AABB<TSpectral, TFloat> aabb = mesh->getAABB();

                // Determine which instances are visible:
                for (auto& instance_data : mesh_data.instances) {
                    Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_data.instance;

                    // Compute the OBB:
                    mat4<TFloat> local_to_camera = view_matrix * instance->getModelMatrix();
                    vira::rendering::OBB<TFloat> obb = aabb.toOBB(local_to_camera);

                    // If object is not detected as visible, check if it is casting a shadow:
                    if (!instance_data.visibility && instance_data.casting_shadow) {
                        for (std::array<vira::rendering::Plane<TFloat>, 6>&frustum_planes : shadow_frustums) {
                            if (intersectsFrustum(obb, frustum_planes)) {
                                instance_data.visibility = true;
                            }
                        }
                    }
                }
            }
        }


        // Compute target GSDs:
        for (auto& [meshID, meshData] : scene.meshes_) {
            auto& mesh = meshData.mesh;

            TFloat distance = std::numeric_limits<TFloat>::infinity();

            for (auto& instance_data : meshData.instances) {
                if (instance_data.visibility) {
                    Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_data.instance;
                    vec3<TFloat> view_vector = camera.getGlobalPosition() - instance->getGlobalPosition();

                    // TODO Use AABB Corners to compute minimum distance:
                    //std::array<vec3<TFloat>, 8> aabbCorners = aabb.getCorners();

                    // If the instance is potentially visible, update the distance:
                    TFloat dist = length(view_vector);
                    if (dist < distance) {
                        distance = dist;
                    }
                }
            }

            if (std::isinf(distance)) {
                continue;
            }

            // Compute the required GSD:
            meshData.target_gsd = options.target_triangle_size * static_cast<float>(camera.calculateGSD(distance));

            // Determine if should be updated:
            float current_gsd = mesh->getGSD();

            // If it is currently lower resolution then needed:
            if (current_gsd > meshData.target_gsd) {
                num_to_update++;
                continue;
            }

            // If it currently higher resolution than needed, check if updating would lower the resolution too much:
            if (current_gsd < meshData.target_gsd) {
                if ((current_gsd * 2.f) > meshData.target_gsd) {
                    meshData.target_gsd = current_gsd;
                    continue;
                }
                else {
                    num_to_update++;
                }
            }
        }

        // Remove meshes that are no longer visible:
        for (auto& [meshID, meshData] : scene.meshes_) {
            for (auto& instance : meshData.instances) {
                if (!instance.visibility) {
                    meshData.target_gsd = std::numeric_limits<float>::infinity();
                }
            }
        }

        if (vira::getPrintStatus()) {
            stop_time = std::chrono::high_resolution_clock::now();
            auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
            std::cout << vira::print::VIRA_INDENT << "Completed (" << duration.count() << " ms)\n";
        }

        // Update meshes:
        scene.updateMeshLoDs(options.parallel_update, num_to_update);
    };

    //template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    //void LevelOfDetailManager<TSpectral, TFloat, TMeshFloat>::updateByDistance(Camera<TSpectral, TFloat, TMeshFloat>& camera, Scene<TSpectral, TFloat, TMeshFloat>& scene, bool checkAngle)
    //{
    //    std::chrono::high_resolution_clock::time_point start_time;
    //    std::chrono::high_resolution_clock::time_point stop_time;
    //    if (vira::getPrintStatus()) {
    //        start_time = std::chrono::high_resolution_clock::now();
    //    }
    //
    //    scene.processSceneGraph();
    //
    //    // TODO Consider parallelizing?
    //    const std::vector<std::shared_ptr<Mesh<TSpectral, TFloat, TMeshFloat>>>& meshes = scene.getMeshes();
    //    const std::vector<std::vector<TransformState<TFloat>>>& meshGlobalTransformStates = scene.getMeshGlobalTransformStates();
    //
    //    size_t numToUpdate = 0;
    //
    //    float maxViewAngle = DEG2RAD<float>() * options.maxViewAngle;
    //
    //    // Loop over all meshes:
    //    vec3<TFloat> camPosition = camera.getTransformState().position;
    //    std::vector<float> targetGSDs(meshes.size(), std::numeric_limits<float>::infinity());
    //    for (size_t i = 0; i < meshes.size(); i++) {
    //        float coneAngle = DEG2RAD<float>() * options.coneAngle;
    //        if (coneAngle == 0) {
    //            coneAngle = mesh->getConeAngle();
    //        }
    //        Normal N_body = mesh->getNormal();
    //
    //        // Determine distance to the closest instance of each mesh:
    //        auto& globalTransformStates = meshGlobalTransformStates[i];
    //        TFloat distance = std::numeric_limits<TFloat>::infinity();
    //        for (size_t j = 0; j < globalTransformStates.size(); ++j) {
    //            vec3<TFloat> viewVector = camPosition - globalTransformStates[j].position;
    //
    //            // Check if view vector is within the normal cone:
    //            if (checkAngle) {
    //                Normal V = normalize(viewVector);
    //                Normal N = normalize(globalTransformStates[j].rotation.inverseMultiply(N_body));
    //
    //                float viewAngle = std::acos(dot(V, N)) - coneAngle;
    //                if (viewAngle >= maxViewAngle) {
    //                    continue;
    //                }
    //            }
    //
    //            TFloat dist = length(viewVector);
    //            if (dist < distance) {
    //                distance = dist;
    //            }
    //        }
    //
    //        if (std::isinf(distance)) {
    //            continue;
    //        }
    //
    //        // Compute the required GSD:
    //        meshData.target_gsd = options.targetTriangleSize * static_cast<float>(camera.calculateGSD(distance));
    //
    //        // Determine if should be updated:
    //        float current_gsd = mesh->getGSD();
    //
    //        // If it is currently lower resolution then needed:
    //        if (current_gsd > meshData.target_gsd) {
    //            numToUpdate++;
    //            continue;
    //        }
    //
    //        // If it currently higher resolution than needed, check if updating would lower the resolution too much:
    //        if (current_gsd < meshData.target_gsd) {
    //            if ((current_gsd * 2.f) > meshData.target_gsd) {
    //                meshData.target_gsd = current_gsd;
    //                continue;
    //            }
    //            else {
    //                numToUpdate++;
    //            }
    //        }
    //    }
    //
    //    if (vira::getPrintStatus()) {
    //        stop_time = std::chrono::high_resolution_clock::now();
    //        auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
    //        std::cout << "  Computed required LoD via Distance";
    //        if (checkAngle) {
    //            std::cout << " + Angle";
    //        }
    //        std::cout << " (DURATION = " << duration.count() << " milliseconds)\n";
    //    }
    //
    //    // Update meshes:
    //    this->updateMeshes(meshes, targetGSDs, numToUpdate);
    //};

    //template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    //void LevelOfDetailManager<TSpectral, TFloat, TMeshFloat>::updateByRaster(Camera<TSpectral, TFloat, TMeshFloat>& camera, Scene<TSpectral, TFloat, TMeshFloat>& scene)
    //{
    //    // Rasterize the Scene to Obtain the Level-of-Detail:
    //    std::chrono::high_resolution_clock::time_point start_time;
    //    std::chrono::high_resolution_clock::time_point stop_time;
    //    if (vira::getPrintStatus()) {
    //        start_time = std::chrono::high_resolution_clock::now();
    //    }
    //
    //    rasterizer.render(camera, scene);
    //
    //    // Get the list of visible meshes:
    //    std::unordered_map<size_t, size_t> meshIDtoIndex = scene.getMeshIDtoIndex();
    //    std::vector<size_t> meshIDs = camera.meshIDImage.getVector();
    //    std::sort(meshIDs.begin(), meshIDs.end());
    //    std::vector<size_t>::iterator it;
    //    it = unique(meshIDs.begin(), meshIDs.end());
    //    meshIDs.resize(std::distance(meshIDs.begin(), it));
    //
    //    // Calculate which meshes to update:
    //    vec3<TFloat> camPosition = camera.getTransformState().position;
    //    const std::vector<std::shared_ptr<Mesh<TSpectral, TFloat, TMeshFloat>>>& meshes = scene.getMeshes();
    //    const std::vector<std::vector<TransformState<TFloat>>>& meshGlobalTransformStates = scene.getMeshGlobalTransformStates();
    //
    //    size_t numToUpdate = 0;
    //    std::vector<uint8_t> unseenMesh(meshes.size(), true);
    //    std::vector<float> targetGSDs(meshes.size(), std::numeric_limits<float>::infinity());
    //
    //    for (size_t meshID : meshIDs) {
    //        size_t meshIndex = meshIDtoIndex[meshID];
    //        unseenMesh[meshIndex] = false;
    //
    //        // Determine distance to the closest instance of each mesh:
    //        auto& globalTransformStates = meshGlobalTransformStates[meshIndex];
    //        TFloat distance = std::numeric_limits<TFloat>::infinity();
    //        for (size_t j = 0; j < globalTransformStates.size(); ++j) {
    //            TFloat dist = length(camPosition - globalTransformStates[j].position);
    //            if (dist < distance) {
    //                distance = dist;
    //            }
    //        }
    //
    //        // Compute the required GSD:
    //        targetGSDs[meshIndex] = options.targetTriangleSize * static_cast<float>(camera.calculateGSD(distance));
    //        float targetSize = targetGSDs[meshIndex];
    //
    //        // Determine if should be updated:
    //        float current_gsd = meshes[meshIndex]->getGSD();
    //
    //        // If it is currently lower resolution then needed:
    //        if (current_gsd > targetSize) {
    //            numToUpdate++;
    //            continue;
    //        }
    //
    //        // If it currently higher resolution than needed, check if updating would lower the resolution too much:
    //        if (current_gsd < targetSize) {
    //            if ((current_gsd * 2.f) > targetSize) {
    //                continue;
    //            }
    //            else {
    //                numToUpdate++;
    //            }
    //        }
    //    };
    //
    //    if (vira::getPrintStatus()) {
    //        stop_time = std::chrono::high_resolution_clock::now();
    //        auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
    //        std::cout << "  Computed required LoD via CPURasterizer (DURATION = " << duration.count() << " milliseconds)\n";
    //    }
    //
    //    // Update meshes:
    //    this->updateMeshes(meshes, targetGSDs, numToUpdate);
    //};
};