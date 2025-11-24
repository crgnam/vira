#include <memory>
#include <stdexcept>
#include <filesystem>
#include <chrono>

#include "embree3/rtcore.h"
#ifdef _WIN32
#define YAML_CPP_API
#endif
#include "yaml-cpp/yaml.h"

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/geometry/interfaces/geometry_interface.hpp"
#include "vira/scene/group.hpp"
#include "vira/scene/instance.hpp"

#include "vira/lights/light.hpp"
#include "vira/lights/point_light.hpp"
#include "vira/lights/sphere_light.hpp"

#include "vira/rendering/acceleration/vira_tlas.hpp"
#include "vira/rendering/acceleration/embree_tlas.hpp"
#include "vira/rendering/acceleration/vira_blas.hpp"
#include "vira/rendering/acceleration/embree_blas.hpp"
#include "vira/rendering/bresenham.hpp"

#include "vira/materials/default.hpp"
#include "vira/materials/lambertian.hpp"
#include "vira/materials/mcewen.hpp"

#include "vira/unresolved/unresolved_object.hpp"

#include "vira/rendering/cpu_path_tracer.hpp"
#include "vira/rendering/cpu_rasterizer.hpp"
#include "vira/rendering/cpu_unresolved_renderer.hpp"

#include "vira/utils/hash_utils.hpp"

namespace fs = std::filesystem;

namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Scene<TSpectral, TFloat, TMeshFloat>::Scene() : GROUP(GroupID{ 1 }, nullptr) {
        this->parent_ = this;
        this->scene_ = this;

        // Create the default material:
        auto defaultMaterial = std::make_unique<materials::DefaultMaterial<TSpectral>>();
        this->defaultMaterialID = this->addMaterial(std::move(defaultMaterial), "DefaultMaterial");

        // Account for the root scene being 1:
        (void)group_manager_.allocate();

        geometryInterface = std::make_unique<geometry::GeometryInterface<TSpectral, TFloat, TMeshFloat>>();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Scene<TSpectral, TFloat, TMeshFloat>::~Scene()
    {
        if (tlas) {
            tlas->device_freed_ = true;
        }

        for (auto& [meshID, meshData] : meshes_) {
            meshData.mesh->deviceFreed();
        }

        if (rtc_device_ != nullptr) {
            rtcReleaseDevice(rtc_device_);
            rtc_device_ = nullptr;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::clear()
    {
        // TODO Refactor this to be called from destructor?
        scene::Group<TSpectral, TFloat, TMeshFloat>::clear();

        this->stars_.clear();

        this->background_emission_ = images::Image<TSpectral>(images::Resolution{ 1,1 }, TSpectral{ 0.f });

        this->tlas.reset();
        this->markDirty();
        this->rebuildTLAS = true;

        for (auto& [meshID, meshData] : meshes_) {
            meshData.mesh->deviceFreed();
        }
        if (rtc_device_ != nullptr) {
            rtcReleaseDevice(rtc_device_);
            rtc_device_ = nullptr;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::saveGroupAsGeometry(const GroupID& group_id, const fs::path& filepath, std::string format)
    {
        scene::Group<TSpectral, TFloat, TMeshFloat>& group = (*this)[group_id];
        this->geometryInterface->saveGroup(group, filepath, format);
    };


    // ======================= //
    // === SPICE Interface === //
    // ======================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setSpiceDatetime(const std::string& datetimeString)
    {
        double newET = spice.stringToET(datetimeString);
        this->setSpiceET(newET);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::incrementSpiceET(double seconds_elapsed)
    {
        this->setSpiceET(this->ephemeris_time_ + seconds_elapsed);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setSpiceET(double newET)
    {
        this->ephemeris_time_ = newET;

        this->updateSPICE(ephemeris_time_);
    };


    // ====================== //
    // === Background API === //
    // ====================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setBackgroundEmission(images::Image<TSpectral> background_emission)
    {
        this->background_emission_ = background_emission;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setBackgroundEmissionRGB(float red, float green, float blue)
    {
        this->setBackgroundEmissionRGB(ColorRGB{ red, green, blue });
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setBackgroundEmissionRGB(ColorRGB background_emission, std::function<TSpectral(const ColorRGB&)> converter)
    {
        if constexpr (std::is_same_v<ColorRGB, TSpectral>) {
            // Direct assignment for same type
            this->getBackgroundEmission(background_emission);
        }
        else {
            // Use provided converter or default spectralConvert
            auto actualConverter = converter ? converter : [](const ColorRGB& input) {
                return spectralConvert_val<ColorRGB, TSpectral>(input);
                };
            this->setBackgroundEmission(actualConverter(background_emission));
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setBackgroundEmission(TSpectral background_emission)
    {
        this->background_emission_ = images::Image<TSpectral>(images::Resolution{ 1,1 }, background_emission);
    };

    // =================== //
    // === Ambient API === //
    // =================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setAmbient(float ambient_lighting)
    {
        ambient_lighting_ = TSpectral{ ambient_lighting };
        if (ambient_lighting_.magnitude() != 0) {
            has_ambient_ = true;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setAmbient(TSpectral ambient_lighting)
    {
        ambient_lighting_ = ambient_lighting;
        if (ambient_lighting_.magnitude() != 0) {
            has_ambient_ = true;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setAmbientRGB(float red, float green, float blue)
    {
        this->setAmbientRGB(ColorRGB{ red, green, blue });
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::setAmbientRGB(ColorRGB ambient_lighting, std::function<TSpectral(const ColorRGB&)> converter)
    {
        if constexpr (std::is_same_v<ColorRGB, TSpectral>) {
            // Direct assignment for same type
            this->setAmbient(ambient_lighting);
        }
        else {
            // Use provided converter or default spectralConvert
            auto actualConverter = converter ? converter : [](const ColorRGB& input) {
                return spectralConvert_val<ColorRGB, TSpectral>(input);
                };
            this->setAmbient(actualConverter(ambient_lighting));
        }
    };

    // ============= //
    // === Stars === //
    // ============= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::loadTychoQuipu(fs::path quipuPath, const std::string& epoch)
    {
        unresolved::StarCatalogue starCat = quipu::StarQuipu::read(quipuPath);
        double et = SpiceUtils<float>::stringToET(epoch);
        this->loadTychoQuipu(quipuPath, et);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::loadTychoQuipu(const fs::path& quipuPath, double et)
    {
        vira::unresolved::StarCatalogue starCat = vira::quipu::StarQuipu::read(quipuPath);
        this->addStarLight(starCat, et);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::addStarLight(const unresolved::StarCatalogue& starCat, double et)
    {
        this->addStarLight(starCat.makeStarLight<TSpectral, TFloat>(et));
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Rotation<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::getRotationToICRF() const
    {
        if (this->isConfiguredSpiceFrame()) {
            // According to SPICE Documentation, rotations to ICRF are achieved by using "J2000" code
            // Reference: https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/frames.html#ICRF%20vs%20J2000
            return Rotation<TFloat>(SpiceUtils<TFloat>::pxform(this->getFrameName(), "J2000", ephemeris_time_));
        }
        else {
            return this->getLocalRotation();
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Rotation<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::getRotationFromICRF() const
    {
        return this->getRotationToICRF().inverse();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::getSSBPositionInICRF() const
    {
        if (this->isConfiguredSpiceFrame()) {
            return SpiceUtils<TFloat>::spkpos(this->getNAIFName(), ephemeris_time_, "J2000", "NONE", "SSB");
        }
        else {
            return this->getLocalPositon();
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::getSSBVelocityInICRF() const
    {
        if (this->isConfiguredSpiceFrame()) {
            return SpiceUtils<TFloat>::computeVelocity(this->getNAIFName(), ephemeris_time_, "J2000", "NONE", "SSB");
            auto state = SpiceUtils<TFloat>::spkezr(this->getNAIFName(), ephemeris_time_, "J2000", "NONE", "SSB");
            return state[1];
        }
        else {
            return this->getLocalVelocity();
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::getAngularRateInICRF() const
    {
        if (this->isConfiguredSpiceFrame()) {
            return SpiceUtils<TFloat>::computeAngularRate(this->getFrameName(), "J2000", ephemeris_time_);
        }
        else {
            return this->getLocalAngularRate();
        }
    };


    // ====================== //
    // === Access Methods === //
    // ====================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<typename IDType>
    const std::string& Scene<TSpectral, TFloat, TMeshFloat>::getName(const IDType& id) const
    {
        if constexpr (std::is_same_v<IDType, MeshID>) {
            return this->getMeshData(id).name;
        }
        else if constexpr (std::is_same_v<IDType, MaterialID>) {
            return this->getMaterialData(id).name;
        }
        else {
            return scene::Group<TSpectral, TFloat, TMeshFloat>::getName(id);
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<typename IDType>
    void Scene<TSpectral, TFloat, TMeshFloat>::setName(const IDType& id, std::string newName)
    {
        if constexpr (std::is_same_v<IDType, MeshID>) {
            this->getMeshData(id).name = newName;
        }
        else if constexpr (std::is_same_v<IDType, MaterialID>) {
            this->getMaterialData(id).name = newName;
        }
        else {
            scene::Group<TSpectral, TFloat, TMeshFloat>::setName(id, newName);
        }
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    const typename Scene<TSpectral, TFloat, TMeshFloat>::MeshData& Scene<TSpectral, TFloat, TMeshFloat>::getMeshData(const MeshID& id) const
    {
        auto it = meshes_.find(id);
        if (it == meshes_.end()) {
            throw std::out_of_range(id.name() + " not found in Scene");
        }
        return it->second;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    typename Scene<TSpectral, TFloat, TMeshFloat>::MeshData& Scene<TSpectral, TFloat, TMeshFloat>::getMeshData(const MeshID& id)
    {
        auto it = meshes_.find(id);
        if (it == meshes_.end()) {
            throw std::out_of_range(id.name() + " not found in Scene");
        }
        return it->second;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    const vira::ManagedData<materials::Material<TSpectral>>& Scene<TSpectral, TFloat, TMeshFloat>::getMaterialData(const MaterialID& id) const
    {
        auto it = materials_.find(id);
        if (it == materials_.end()) {
            throw std::out_of_range(id.name() + " not found in Scene");
        }
        return it->second;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::ManagedData<materials::Material<TSpectral>>& Scene<TSpectral, TFloat, TMeshFloat>::getMaterialData(const MaterialID& id)
    {
        auto it = materials_.find(id);
        if (it == materials_.end()) {
            throw std::out_of_range(id.name() + " not found in Scene");
        }
        return it->second;
    };


    // ================= //
    // === Materials === //
    // ================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MaterialID Scene<TSpectral, TFloat, TMeshFloat>::addMaterial(std::unique_ptr<materials::Material<TSpectral>> material, std::string name)
    {
        this->processName(name, materials_, "Material");

        MaterialID materialID = this->allocateMaterialID();
        material->id_ = materialID;

        ManagedData<materials::Material<TSpectral>> newEntry(std::move(material), name);

        materials_.emplace(materialID, std::move(newEntry));
        this->markDirty();

        return materialID;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Scene<TSpectral, TFloat, TMeshFloat>::removeMaterial(const MaterialID& materialID)
    {
        if (materialID == this->defaultMaterialID) {
            return false; // Cannot delete the default material
        }

        auto it = materials_.find(materialID);
        if (it == materials_.end()) {
            return false;
        }

        for (auto& [meshID, meshData] : meshes_) {
            (void)meshData.mesh->removeMaterial(materialID);
        }

        // Remove the material:
        materials_.erase(it);

        this->markDirty();
        this->deallocateMaterialID();

        return true;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Scene<TSpectral, TFloat, TMeshFloat>::hasMaterial(const MaterialID& materialID) const
    {
        return materials_.find(materialID) != materials_.end();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MaterialID Scene<TSpectral, TFloat, TMeshFloat>::newLambertianMaterial(std::string name)
    {
        return addMaterial(std::make_unique<materials::Lambertian<TSpectral>>(), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MaterialID Scene<TSpectral, TFloat, TMeshFloat>::newMcEwenMaterial(std::string name)
    {
        return addMaterial(std::make_unique<materials::McEwen<TSpectral>>(), name);
    };


    // ============== //
    // === Meshes === //
    // ============== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MeshID Scene<TSpectral, TFloat, TMeshFloat>::addMesh(std::unique_ptr<geometry::Mesh<TSpectral, TFloat, TMeshFloat>> mesh, std::string name)
    {
        this->processName(name, meshes_, "Mesh");

        MeshID meshID = this->allocateMeshID();
        mesh->id_ = meshID;
        mesh->setScene(this);
        mesh->default_material_cache_ = this->materials_.at(this->defaultMaterialID).data.get();
        for (size_t i = 0; i < mesh->material_cache_.size(); ++i) {
            mesh->material_cache_[i] = mesh->default_material_cache_;
            mesh->materialIDs_[i] = this->defaultMaterialID;
        }

        MeshData meshData{};
        meshData.mesh = std::move(mesh);
        meshData.name = name;
        meshes_.emplace(meshID, std::move(meshData));

        this->markDirty();

        return meshID;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MeshID Scene<TSpectral, TFloat, TMeshFloat>::addQuipuMesh(const fs::path& quipuPath, const MaterialID& materialID, std::string name)
    {
        vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu(quipuPath);
        return addQuipuMesh(quipu, materialID, name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    MeshID Scene<TSpectral, TFloat, TMeshFloat>::addQuipuMesh(vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu, const MaterialID& materialID, bool smoothShading, std::string name)
    {
        fs::path quipuPath = quipu.getFilepath();
        if (hasLoadedQuipu(quipuPath)) {
            return loadedQuipuPaths_.at(quipuPath.string());
        }
        else {
            if (name.empty()) {
                name = quipuPath.stem().string();
            }
            auto newMesh = std::make_unique<MESH>(quipu);
            newMesh->setSmoothShading(smoothShading);

            // Assign the material:
            MeshID meshID = addMesh(std::move(newMesh), name);
            (*this)[meshID].setMaterial(0, materialID);

            return meshID;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Scene<TSpectral, TFloat, TMeshFloat>::removeMesh(const MeshID& meshID)
    {
        auto it = meshes_.find(meshID);
        if (it == meshes_.end()) {
            return false;
        }

        // Check if this mesh was loaded from a Quipu and clean up the cache
        std::string quipuPath = it->second.mesh->getQuipuFilepath().string();
        if (!quipuPath.empty()) {
            loadedQuipuPaths_.erase(quipuPath);
        }

        // Remove all instances of this mesh throughout the scene graph
        this->removeInstancesOfMesh(meshID);

        // Remove the mesh:
        meshes_.erase(it);

        this->markDirty();
        this->deallocateMeshID();

        return true;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Scene<TSpectral, TFloat, TMeshFloat>::hasMesh(const MeshID& meshID) const
    {
        return meshes_.find(meshID) != meshes_.end();
    }



    // ===================== //
    // === Asset Loaders === //
    // ===================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    geometry::LoadedMeshes<TFloat> Scene<TSpectral, TFloat, TMeshFloat>::loadGeometry(const fs::path& filepath, std::string format) {
        return geometryInterface->load(*this, filepath, format);
    }


    // ========================= //
    // === Rendering Methods === //
    // ========================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::processSceneGraph()
    {
        if (this->is_dirty_) {
            std::chrono::high_resolution_clock::time_point start_time;
            std::chrono::high_resolution_clock::time_point stop_time;
            if (vira::getPrintStatus()) {
                start_time = std::chrono::high_resolution_clock::now();
                std::cout << "Pre-processing Scene Graph...\n" << std::flush;
            }

            this->light_cache_.clear();
            this->unresolved_cache_.clear();

            // Reset instance cache and update material cache:
            for (auto& [meshID, meshData] : meshes_) {
                meshData.instances.clear();

                for (size_t i = 0; i < meshData.mesh->materialIDs_.size(); ++i) {
                    if (hasMaterial(meshData.mesh->materialIDs_[i])) {
                        meshData.mesh->material_cache_[i] = materials_.at(meshData.mesh->materialIDs_[i]).data.get();
                    }
                    else {
                        meshData.mesh->materialIDs_[i] = defaultMaterialID;
                        meshData.mesh->material_cache_[i] = materials_.at(defaultMaterialID).data.get();
                    }
                }
            }

            // Check that SPICE Configuration is valid:
            if (this->isConfiguredSpiceObject() && !this->isConfiguredSpiceFrame()) {
                throw std::runtime_error("If using SPICE, Scene must be a fully defined Frame (Must provide both NAIF ID and FrameName)");
            }

            // Start traversal from the root (Scene) with identity transformState
            recursivelyPopulateCaches(this);

            this->is_dirty_ = false;
            rebuildTLAS = true;

            if (vira::getPrintStatus()) {
                stop_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);
                std::cout << vira::print::VIRA_INDENT << "Completed (" << duration.count() << " ms)\n" << std::flush;
            }
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::recursivelyPopulateCaches(const scene::Group<TSpectral, TFloat, TMeshFloat>* parent_group)
    {
        // Update the meshes instance cache:
        for (const auto& [instance_id, instance_entry] : parent_group->instances_) {
            vira::scene::Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_entry.data.get();

            MeshID meshID = instance->getMeshID();

            TransformData transform_data;
            transform_data.instance = instance;
            transform_data.visibility = true; // This can be turned off by the LoDManager
            transform_data.casting_shadow = false; // This can be turned on by the LoDManager

            meshes_.at(meshID).instances.push_back(transform_data);
        }

        // Update the light cache:
        for (const auto& [light_id, light_entry] : parent_group->lights_) {
            if (light_entry.data != nullptr) {
                light_cache_.push_back(light_entry.data.get());
            }
        }

        // Update the unresolved object cache:
        for (const auto& [unresolved_id, unresolved_entry] : parent_group->unresolved_objects_) {
            if (unresolved_entry.data != nullptr) {
                unresolved_cache_.push_back(unresolved_entry.data.get());
            }
        }

        // Recursively process all child groups
        for (const auto& [group_id, child_group] : parent_group->groups_) {
            recursivelyPopulateCaches(child_group.data.get());
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::updateMeshLoDs(bool parallelUpdate, size_t numToUpdate)
    {
        if (numToUpdate != 0) {
            std::chrono::high_resolution_clock::time_point start_time;
            std::chrono::high_resolution_clock::time_point stop_time;
            std::unique_ptr<indicators::ProgressBar> progressBar;
            if (vira::getPrintStatus()) {
                start_time = std::chrono::high_resolution_clock::now();
                std::cout << "Updating " << numToUpdate << " meshes...\n" << std::flush;
                progressBar = vira::print::makeProgressBar("Updating meshes");
            }

            const size_t totalMeshes = meshes_.size();
            const size_t progressUpdateInterval = std::max(static_cast<size_t>(1), totalMeshes / 10); // Update every 10%

            if (parallelUpdate) {
                // Convert map to vector of references for parallel processing
                std::vector<std::reference_wrapper<const std::pair<const MeshID, MeshData>>> meshPairs;
                meshPairs.reserve(meshes_.size());

                for (const auto& pair : meshes_) {
                    meshPairs.emplace_back(std::cref(pair));
                }

                // Atomic counter for thread-safe progress tracking
                std::atomic<size_t> processedMeshes{ 0 };
                std::atomic<size_t> lastReportedProgress{ 0 };

                vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)
                tbb::parallel_for(tbb::blocked_range<size_t>(0, meshPairs.size()),
                    [&](tbb::blocked_range<size_t> r)
                    {
                        // Local counter for batch updates
                        size_t localMeshCount = 0;
                        const size_t batchSize = 16; // Update progress every 16 meshes per thread

                        for (size_t i = r.begin(); i < r.end(); ++i) {
                            const auto& [meshID, meshData] = meshPairs[i].get();
                            const auto& mesh = meshData.mesh;
                            mesh->updateGSD(meshData.target_gsd);

                            // Progress tracking - batch update to reduce atomic operations
                            if (vira::getPrintStatus() && progressBar && (++localMeshCount % batchSize == 0)) {
                                size_t newProcessed = processedMeshes.fetch_add(batchSize, std::memory_order_relaxed) + batchSize;

                                // Check if we should update progress display
                                if (newProcessed - lastReportedProgress.load(std::memory_order_relaxed) >= progressUpdateInterval) {
                                    size_t expected = lastReportedProgress.load();
                                    if (lastReportedProgress.compare_exchange_weak(expected, newProcessed, std::memory_order_relaxed)) {
                                        float percentage = (static_cast<float>(newProcessed) / static_cast<float>(totalMeshes)) * 100.0f;
                                        vira::print::updateProgressBar(progressBar, percentage);
                                    }
                                }
                                localMeshCount = 0; // Reset local counter
                            }
                        }

                        // Handle remaining meshes in the batch
                        if (vira::getPrintStatus() && progressBar && localMeshCount > 0) {
                            size_t newProcessed = processedMeshes.fetch_add(localMeshCount, std::memory_order_relaxed) + localMeshCount;
                            size_t expected = lastReportedProgress.load();
                            if (newProcessed - expected >= progressUpdateInterval &&
                                lastReportedProgress.compare_exchange_weak(expected, newProcessed, std::memory_order_relaxed)) {
                                float percentage = (static_cast<float>(newProcessed) / static_cast<float>(totalMeshes)) * 100.0f;
                                vira::print::updateProgressBar(progressBar, percentage);
                            }
                        }
                    });
            }
            else {
                // Sequential processing with progress updates
                size_t lastReportedProgress = 0;
                size_t processedCount = 0;

                for (const auto& [meshID, meshData] : meshes_) {
                    auto& mesh = meshData.mesh;
                    mesh->updateGSD(meshData.target_gsd);
                    ++processedCount;

                    // Update progress every 10%
                    if (vira::getPrintStatus() && progressBar && (processedCount - lastReportedProgress >= progressUpdateInterval)) {
                        float percentage = (static_cast<float>(processedCount) / static_cast<float>(totalMeshes)) * 100.0f;
                        vira::print::updateProgressBar(progressBar, percentage);
                        lastReportedProgress = processedCount;
                    }
                }
            }

            if (vira::getPrintStatus()) {
                stop_time = std::chrono::high_resolution_clock::now();
                auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
                vira::print::updateProgressBar(progressBar, "Completed " + std::to_string(duration.count()) + " ms)", 100.0f);
            }
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::buildTLAS()
    {
        this->processSceneGraph();

        constexpr const bool FULL_EMBREE_COMPATIBLE = std::same_as<TMeshFloat, float> and std::same_as<TFloat, float>;
        constexpr const bool BLAS_EMBREE_COMPATIBLE = std::same_as<TMeshFloat, float> and !std::same_as<TFloat, float>;

        if (rebuildTLAS) {
            std::chrono::high_resolution_clock::time_point start_time;
            std::chrono::high_resolution_clock::time_point stop_time;
            if (vira::getPrintStatus()) {
                std::cout << "Building Acceleration Structure...\n" << std::flush;
                start_time = std::chrono::high_resolution_clock::now();
            }

            if constexpr (FULL_EMBREE_COMPATIBLE) {
                if (rtc_device_ == nullptr) {
                    rtc_device_ = rtcNewDevice(nullptr);
                }

                auto embreeTLAS = std::make_unique<rendering::EmbreeTLAS<TSpectral>>(rtc_device_, bvhBuildOptions.embree_options);

                // Loop over all meshes in the map
                for (const auto& [meshID, mesh_data] : meshes_) {
                    auto& mesh = mesh_data.mesh;

                    // Reconstruct base BVH if required:
                    mesh->buildBVH(rtc_device_, bvhBuildOptions);

                    // Loop over all global transformState instances for the current Mesh:
                    for (auto& instance_data : mesh_data.instances) {
                        // Only include instances which are visible:
                        if (instance_data.visibility) {
                            embreeTLAS->newInstance(rtc_device_, dynamic_cast<rendering::EmbreeBLAS<TSpectral, float>*>(mesh->bvh.get()), mesh.get(), instance_data.instance);
                        }
                    }
                }

                // Build the new TLAS:
                embreeTLAS->build();

                tlas.reset();
                tlas = std::move(embreeTLAS);
            }
            else if constexpr (BLAS_EMBREE_COMPATIBLE) {
                if (rtc_device_ == nullptr) {
                    rtc_device_ = rtcNewDevice(nullptr);
                }

                std::vector<rendering::TLASLeaf<TSpectral, TFloat, TMeshFloat>> leafVec;

                // Loop over all meshes:
                for (const auto& [meshID, meshData] : meshes_) {
                    auto& mesh = meshData.mesh;

                    // Reconstruct base BVH if required:
                    mesh->buildBVH(rtc_device_, bvhBuildOptions);

                    // Loop over all global transformState instances for the current Mesh:
                    for (auto& instance : meshData.instances) {
                        if (instance.visibility) {
                            auto leafNode = TLASLeaf<TSpectral, TFloat, TMeshFloat>(mesh->bvh.get(), instance.globalTransformState, meshID, instance.instanceID);
                            leafVec.push_back(leafNode);
                        }
                    }
                }

                // Construct the TLAS:
                tlas = std::make_unique<rendering::ViraTLAS<TSpectral, TMeshFloat>>(leafVec);
            }
            else {
                std::vector<rendering::TLASLeaf<TSpectral, TFloat, TMeshFloat>> leafVec;

                // Loop over all meshes:
                for (const auto& [meshID, meshData] : meshes_) {
                    auto& mesh = meshData.mesh;

                    // Reconstruct base BVH if required:
                    mesh->buildBVH(bvhBuildOptions);

                    // Loop over all global transformState instances for the current Mesh:
                    for (auto& instance : meshData.instances) {
                        auto leafNode = rendering::TLASLeaf<TSpectral, TFloat, TMeshFloat>(mesh->bvh.get(), instance.global_transform_state, meshID, instance.instance_id);
                        leafVec.push_back(leafNode);
                    }
                }

                // Construct the TLAS:
                tlas = std::make_unique<rendering::ViraTLAS<TSpectral, TMeshFloat>>(leafVec);
            }

            // Reset flag:
            rebuildTLAS = false;

            if (vira::getPrintStatus()) {
                stop_time = std::chrono::high_resolution_clock::now();
                auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
                std::cout << vira::print::VIRA_INDENT + "Completed (" << duration.count() << " ms)\n";
            }
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    rendering::TLAS<TSpectral, TFloat, TMeshFloat>* Scene<TSpectral, TFloat, TMeshFloat>::getTLAS()
    {
        this->buildTLAS();
        return tlas.get();
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<float> Scene<TSpectral, TFloat, TMeshFloat>::unresolvedRender(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->unresolvedRenderer.render(camera, *this);
        return camera.simulateSensor(unresolvedRenderer.renderPasses.unresolved_power);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::unresolvedRenderRGB(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->unresolvedRenderer.render(camera, *this);
        return camera.simulateSensorRGB(unresolvedRenderer.renderPasses.unresolved_power);
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<float> Scene<TSpectral, TFloat, TMeshFloat>::pathtraceRender(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->pathtracer.render(camera, *this);
        return camera.simulateSensor(pathtracer.renderPasses.received_power);
    };
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::pathtraceRenderRGB(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->pathtracer.render(camera, *this);
        auto& powerImage = pathtracer.renderPasses.received_power;
        return camera.simulateSensorRGB(powerImage);
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<float> Scene<TSpectral, TFloat, TMeshFloat>::rasterizeRender(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->rasterizer.render(camera, *this);
        return camera.simulateSensor(rasterizer.renderPasses.received_power);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::rasterizeRenderRGB(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        this->rasterizer.render(camera, *this);
        return camera.simulateSensorRGB(rasterizer.renderPasses.received_power);
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<TSpectral> Scene<TSpectral, TFloat, TMeshFloat>::renderTotalPower(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);

        // Only re-run rendering if something in the Scene has been changed
        // Avoiding this allows you to take multiple different exposures without re-rendering!
        if (this->is_dirty_) {
            // Perform pathtracing:
            pathtracer.render(camera, *this);

            // Render unresolved:
            unresolvedRenderer.render(camera, *this);
        }

        // Compute the total power image:
        //const vira::images::Image<float>& alphaImage = pathtracer.renderPasses.alphaImage;
        const vira::images::Image<TSpectral>& received_power = pathtracer.renderPasses.received_power;
        const vira::images::Image<TSpectral>& unresolved_power = unresolvedRenderer.renderPasses.unresolved_power;

        vira::images::Image<TSpectral> total_power;
        if (unresolved_power.size() == 0 && received_power.size() == 0) {
            return total_power;
        }
        else if (unresolved_power.size() > 0 && received_power.size() == 0) {
            total_power = unresolved_power;
        }
        else if (unresolved_power.size() == 0 && received_power.size() > 0) {
            total_power = received_power;
        }
        else if (unresolved_power.size() == received_power.size()) {
            // TODO alpha does not work because it is not convolved in the pathtracer/rasterizer
            //totalPowerImage = receivedPowerImage;
            //totalPowerImage.setAlpha(alphaImage);
            //totalPowerImage = vira::images::alphaOver(unresolvedPowerImage, totalPowerImage);

            // This is only an approximation.  It will cause bright objects behind a dim path-traced object
            // to appear.  This needs to be addressed!
            total_power = received_power + unresolved_power;
        }

        return total_power;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<float> Scene<TSpectral, TFloat, TMeshFloat>::render(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        vira::images::Image<TSpectral> total_power = renderTotalPower(cameraID);
        return camera.simulateSensor(total_power);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::renderRGB(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        vira::images::Image<TSpectral> total_power = renderTotalPower(cameraID);
        return camera.simulateSensorRGB(total_power);
    };


    // ======================= //
    // === Graph Modifiers === //
    // ======================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::moveCamera(CameraID id, GroupID from_group, GroupID to_group)
    {
        if (from_group == to_group) {
            return;
        }

        scene::Group<TSpectral, TFloat, TMeshFloat>* from_group_ptr;
        scene::Group<TSpectral, TFloat, TMeshFloat>* to_group_ptr;
        if (from_group == this->getID()) {
            from_group_ptr = this;
        }
        else {
            from_group_ptr = &(*this)[from_group];
        }
        if (to_group == this->getID()) {
            to_group_ptr = this;
        }
        else {
            to_group_ptr = &(*this)[to_group];
        }

        from_group_ptr->moveCamera(id, to_group_ptr);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::moveInstance(InstanceID id, GroupID from_group, GroupID to_group)
    {
        if (from_group == to_group) {
            return;
        }

        scene::Group<TSpectral, TFloat, TMeshFloat>* from_group_ptr;
        scene::Group<TSpectral, TFloat, TMeshFloat>* to_group_ptr;
        if (from_group == this->getID()) {
            from_group_ptr = this;
        }
        else {
            from_group_ptr = &(*this)[from_group];
        }
        if (to_group == this->getID()) {
            to_group_ptr = this;
        }
        else {
            to_group_ptr = &(*this)[to_group];
        }

        from_group_ptr->moveInstance(id, to_group_ptr);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::moveLight(LightID id, GroupID from_group, GroupID to_group)
    {
        if (from_group == to_group) {
            return;
        }

        scene::Group<TSpectral, TFloat, TMeshFloat>* from_group_ptr;
        scene::Group<TSpectral, TFloat, TMeshFloat>* to_group_ptr;
        if (from_group == this->getID()) {
            from_group_ptr = this;
        }
        else {
            from_group_ptr = &(*this)[from_group];
        }
        if (to_group == this->getID()) {
            to_group_ptr = this;
        }
        else {
            to_group_ptr = &(*this)[to_group];
        }

        from_group_ptr->moveLight(id, to_group_ptr);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::moveUnresolvedObject(UnresolvedID id, GroupID from_group, GroupID to_group)
    {
        if (from_group == to_group) {
            return;
        }

        scene::Group<TSpectral, TFloat, TMeshFloat>* from_group_ptr;
        scene::Group<TSpectral, TFloat, TMeshFloat>* to_group_ptr;
        if (from_group == this->getID()) {
            from_group_ptr = this;
        }
        else {
            from_group_ptr = &(*this)[from_group];
        }
        if (to_group == this->getID()) {
            to_group_ptr = this;
        }
        else {
            to_group_ptr = &(*this)[to_group];
        }

        from_group_ptr->moveUnresolved(id, to_group_ptr);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::moveGroup(GroupID id, GroupID from_group, GroupID to_group)
    {
        if (id == to_group) {
            throw std::runtime_error("Cannot move Group into itself");
        }

        if (from_group == to_group) {
            return;
        }

        scene::Group<TSpectral, TFloat, TMeshFloat>* from_group_ptr;
        scene::Group<TSpectral, TFloat, TMeshFloat>* to_group_ptr;
        if (from_group == this->getID()) {
            from_group_ptr = this;
        }
        else {
            from_group_ptr = &(*this)[from_group];
        }
        if (to_group == this->getID()) {
            to_group_ptr = this;
        }
        else {
            to_group_ptr = &(*this)[to_group];
        }
        from_group_ptr->moveGroup(id, to_group_ptr);
    };


    // ======================= //
    // === Level of Detail === //
    // ======================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Scene<TSpectral, TFloat, TMeshFloat>::updateLevelOfDetail(const vira::CameraID& cameraID)
    {
        auto& camera = this->operator[](cameraID);
        lodManager.update(camera, *this);
    };


    // ======================= //
    // === Drawing Methods === //
    // ======================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::drawBoundingBoxes(const images::Image<ColorRGB>& image, const CameraID& cameraID, float width)
    {
        images::Image<float> depth(images::Resolution{ 0,0 });
        return this->drawBoundingBoxes(image, depth, cameraID, width);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Scene<TSpectral, TFloat, TMeshFloat>::drawBoundingBoxes(const images::Image<ColorRGB>& image, images::Image<float>& depth, const CameraID& cameraID, float width)
    {
        this->processSceneGraph();
        auto& camera = this->operator[](cameraID);

        images::Image<ColorRGB> output = image;

        for (const auto& [mesh_id, mesh_data] : meshes_) {
            auto& mesh = mesh_data.mesh;

            ColorRGB color = utils::idToColor(mesh_id.id());

            rendering::AABB<TSpectral, TFloat> aabb = mesh->getAABB();

            // Loop over all global transformState instances for the current Mesh:
            for (auto& instance_data : mesh_data.instances) {
                vira::scene::Instance<TSpectral, TFloat, TMeshFloat>* instance = instance_data.instance;
                std::array<std::array<vec3<TFloat>, 4>, 6> faces = aabb.getFaces();

                mat4<TFloat> model_to_camera = camera.getViewMatrix() * instance->getModelMatrix();

                for (const std::array<vec3<TFloat>, 4>&face : faces) {
                    std::vector<Pixel> pixelCorners(4);
                    std::vector<float> pixelDepths(4);
                    for (size_t i = 0; i < 4; ++i) {
                        vec3<TFloat> face_corner = transformPoint(model_to_camera, face[i]);

                        pixelCorners[i] = camera.projectCameraPoint(face_corner);
                        //pixelCorners[i] = clipPixel(pixelCorners[i], camera.getResolution());

                        pixelDepths[i] = static_cast<float>(length(face_corner));
                    }
                    rendering::drawPolygon(output, depth, color, pixelCorners, pixelDepths, width);
                }
            }
        }
        return output;
    };
};