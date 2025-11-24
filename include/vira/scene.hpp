#ifndef VIRA_SCENE_HPP
#define VIRA_SCENE_HPP

#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <functional>

// TODO REMOVE THIRD PARTY HEADERS:
#include "embree3/rtcore.h"

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/geometry/interfaces/load_result.hpp"
#include "vira/lights/light.hpp"
#include "vira/scene/group.hpp"
#include "vira/scene/instance.hpp"
#include "vira/scene/ids.hpp"
#include "vira/unresolved/unresolved_object.hpp"
#include "vira/unresolved/star_catalogue.hpp"
#include "vira/unresolved/star_light.hpp"
#include "vira/quipu/star_quipu.hpp"
#include "vira/rendering/acceleration/tlas.hpp"
#include "vira/scene/lod_manager.hpp"

#include "vira/cameras/camera.hpp"

#include "vira/rendering/cpu_path_tracer.hpp"
#include "vira/rendering/cpu_rasterizer.hpp"
#include "vira/rendering/cpu_unresolved_renderer.hpp"

namespace fs = std::filesystem;

// Forward Declare:
namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class LevelOfDetailManager;
};

namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class GeometryInterface;
}

namespace vira {
    template <IsSpectral TSpectral = ColorRGB, IsFloat TFloat = float, IsFloat TMeshFloat = float> requires LesserFloat<TFloat, TMeshFloat>
    class Scene : public scene::Group<TSpectral, TFloat, TMeshFloat> {
        using MESH = geometry::Mesh<TSpectral, TFloat, TMeshFloat>;
        using MATERIAL = materials::Material<TSpectral>;
        using CAMERA = cameras::Camera<TSpectral, TFloat, TMeshFloat>;
        using LIGHT = lights::Light<TSpectral, TFloat, TMeshFloat>;
        using GROUP = scene::Group<TSpectral, TFloat, TMeshFloat>;
        using INSTANCE = scene::Instance<TSpectral, TFloat, TMeshFloat>;
        using UNRESOLVED = unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>;

    public:
        Scene();

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        ~Scene() override;

        // Basic operations:
        void clear();

        void saveGroupAsGeometry(const GroupID& group_id, const fs::path& filepath, std::string format = "AUTO");


        // ======================= //
        // === SPICE Interface === //
        // ======================= //
        SpiceUtils<TFloat> spice;

        void setSpiceDatetime(const std::string& datetimeString);
        void setSpiceET(double et);
        void incrementSpiceET(double seconds_elapsed);

        double getSpiceET() { return ephemeris_time_; }


        // ======================= //
        // === Image Interface === //
        // ======================= //
        vira::images::ImageInterface imageInterface;


        // ====================== //
        // === Background API === //
        // ====================== //
        void setBackgroundEmission(images::Image<TSpectral> background_emission);

        void setBackgroundEmissionRGB(float red, float green, float blue);
        void setBackgroundEmissionRGB(ColorRGB background_emission, std::function<TSpectral(const ColorRGB&)> converter = nullptr);

        void setBackgroundEmission(TSpectral background_emission);
        void setBackgroundEmission(float background_emission) { this->setBackgroundEmission(TSpectral{ background_emission }); }

        const images::Image<TSpectral>& getBackgroundEmission() { return background_emission_; }


        // =================== //
        // === Ambient API === //
        // =================== //
        void setAmbient(float ambient_lighting);
        void setAmbient(TSpectral ambient_lighting);

        void setAmbientRGB(float red, float green, float blue);
        void setAmbientRGB(ColorRGB ambient_lighting, std::function<TSpectral(const ColorRGB&)> converter = nullptr);

        const TSpectral getAmbient() { return ambient_lighting_; }
        bool hasAmbient() { return has_ambient_; }

        // ============= //
        // === Stars === //
        // ============= //
        void loadTychoQuipu(fs::path quipuPath, const std::string& epoch);
        void loadTychoQuipu(const fs::path& quipuPath, double et);
        void addStarLight(const unresolved::StarCatalogue& starCat, double et);
        void addStarLight(std::vector<unresolved::StarLight<TSpectral, TFloat>> newStars) { stars_ = std::move(newStars); }

        Rotation<TFloat> getRotationToICRF() const;
        Rotation<TFloat> getRotationFromICRF() const;
        vec3<TFloat> getSSBPositionInICRF() const;

        vec3<TFloat> getSSBVelocityInICRF() const;
        vec3<TFloat> getAngularRateInICRF() const;

        std::vector<unresolved::StarLight<TSpectral, TFloat>>& getStarLight() { return stars_; }
        const std::vector<unresolved::StarLight<TSpectral, TFloat>>& getStarLight() const { return stars_; }


        // ====================== //
        // === Access Methods === //
        // ====================== //
        template <typename IDType>
        const std::string& getName(const IDType& id) const;

        template <typename IDType>
        void setName(const IDType& id, std::string newName);

        const MESH& operator[](const MeshID& id) const { return *this->getMeshData(id).mesh; }
        MESH& operator[](const MeshID& id) { return const_cast<MESH&>(static_cast<const Scene&>(*this)[id]); }

        const MATERIAL& operator[](const MaterialID& id) const { return *this->getMaterialData(id).data; }
        MATERIAL& operator[](const MaterialID& id) { return const_cast<MATERIAL&>(static_cast<const Scene&>(*this)[id]); }

        const CAMERA& operator[](const CameraID& id) const { return this->recursiveSearch(id); }
        CAMERA& operator[](const CameraID& id) { return const_cast<CAMERA&>(static_cast<const Scene&>(*this)[id]); }

        const GROUP& operator[](const GroupID& id) const { return this->recursiveSearch(id); }
        GROUP& operator[](const GroupID& id) { return const_cast<GROUP&>(static_cast<const Scene&>(*this)[id]); }

        const INSTANCE& operator[](const InstanceID& id) const { return this->recursiveSearch(id); }
        INSTANCE& operator[](const InstanceID& id) { return const_cast<INSTANCE&>(static_cast<const Scene&>(*this)[id]); }

        const LIGHT& operator[](const LightID& id) const { return this->recursiveSearch(id); }
        LIGHT& operator[](const LightID& id) { return const_cast<LIGHT&>(static_cast<const Scene&>(*this)[id]); }

        const UNRESOLVED& operator[](const UnresolvedID& id) const { return this->recursiveSearch(id); }
        UNRESOLVED& operator[](const UnresolvedID& id) { return const_cast<UNRESOLVED&>(static_cast<const Scene&>(*this)[id]); }


        bool isDirty() const { return this->is_dirty_; }
        void markDirty() { this->is_dirty_ = true; }

        // ================= //
        // === Materials === //
        // ================= //
        MaterialID addMaterial(std::unique_ptr<MATERIAL> material, std::string name = "");

        bool removeMaterial(const MaterialID& materialID);
        bool hasMaterial(const MaterialID& materialID) const;

        MaterialID newLambertianMaterial(std::string name = "");
        MaterialID newMcEwenMaterial(std::string name = "");


        // ============== //
        // === Meshes === //
        // ============== //
        MeshID addMesh(std::unique_ptr<MESH> mesh, std::string name = "");
        bool hasLoadedQuipu(const fs::path& quipuPath) { return loadedQuipuPaths_.find(quipuPath.string()) != loadedQuipuPaths_.end(); }
        MeshID addQuipuMesh(const fs::path& quipuPath, const MaterialID& materialID, std::string name = "");
        MeshID addQuipuMesh(vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu, const MaterialID& materialID, bool smoothShading, std::string name = "");

        bool removeMesh(const MeshID& meshID);
        bool hasMesh(const MeshID& meshID) const;


        // ===================== //
        // === Asset Loaders === //
        // ===================== //
        geometry::LoadedMeshes<TFloat> loadGeometry(const fs::path& filepath, std::string format = "AUTO");


        // ================= //
        // === Renderers === //
        // ================= //
        rendering::BVHBuildOptions bvhBuildOptions{};

        void processSceneGraph();

        void buildTLAS();
        rendering::TLAS<TSpectral, TFloat, TMeshFloat>* getTLAS();

        vira::rendering::CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat> unresolvedRenderer;
        vira::images::Image<float> unresolvedRender(const vira::CameraID& cameraID);
        vira::images::Image<ColorRGB> unresolvedRenderRGB(const vira::CameraID& cameraID);

        vira::rendering::CPUPathTracer<TSpectral, TFloat, TMeshFloat> pathtracer;
        vira::images::Image<float> pathtraceRender(const vira::CameraID& cameraID);
        vira::images::Image<ColorRGB> pathtraceRenderRGB(const vira::CameraID& cameraID);

        vira::rendering::CPURasterizer<TSpectral, TFloat, TMeshFloat> rasterizer;
        vira::images::Image<float> rasterizeRender(const vira::CameraID& cameraID);
        vira::images::Image<ColorRGB> rasterizeRenderRGB(const vira::CameraID& cameraID);


        vira::images::Image<TSpectral> renderTotalPower(const vira::CameraID& cameraID);
        vira::images::Image<float> render(const vira::CameraID& cameraID);
        vira::images::Image<ColorRGB> renderRGB(const vira::CameraID& cameraID);

        void initializeTLASThreads()
        {
            this->tlas->init();
        }

        void intersect(vira::rendering::Ray<TSpectral, TFloat>& ray)
        {
            this->tlas->intersect(ray);
        }

        // ======================= //
        // === Graph Modifiers === //
        // ======================= //
        void moveCamera(CameraID id, GroupID from_group, GroupID to_group);
        void moveInstance(InstanceID id, GroupID from_group, GroupID to_group);
        void moveLight(LightID id, GroupID from_group, GroupID to_group);
        void moveUnresolvedObject(UnresolvedID id, GroupID from_group, GroupID to_group);
        void moveGroup(GroupID id, GroupID from_group, GroupID to_group);


        // ======================= //
        // === Level of Detail === //
        // ======================= //
        scene::LevelOfDetailManager<TSpectral, TFloat, TMeshFloat> lodManager;
        void updateLevelOfDetail(const vira::CameraID& cameraID);

        // ======================= //
        // === Drawing methods === //
        // ======================= //
        images::Image<ColorRGB> drawBoundingBoxes(const images::Image<ColorRGB>& image, const CameraID& cameraID, float width = 3.f);
        images::Image<ColorRGB> drawBoundingBoxes(const images::Image<ColorRGB>& image, images::Image<float>& depth, const CameraID& cameraID, float width = 3.f);


    private:
        // Scene data:
        TSpectral ambient_lighting_ = TSpectral{ 0 };
        bool has_ambient_ = false;
        images::Image<TSpectral> background_emission_ = images::Image<TSpectral>(images::Resolution{ 1,1 }, TSpectral{ 0.f });
        std::vector<unresolved::StarLight<TSpectral, TFloat>> stars_;

        struct TransformData {
            scene::Instance<TSpectral, TFloat, TMeshFloat>* instance;
            bool visibility = true;
            bool casting_shadow = false;
        };

        struct MeshData {
            std::unique_ptr<MESH> mesh;
            std::vector<TransformData> instances;
            float target_gsd = std::numeric_limits<float>::infinity();
            std::string name;
        };

        std::unordered_map<std::string, MeshID> loadedQuipuPaths_;
        std::unordered_map<MeshID, MeshData> meshes_;
        std::unordered_map<MaterialID, ManagedData<MATERIAL>> materials_;
        MaterialID defaultMaterialID;

        std::vector<unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>*> unresolved_cache_;
        std::vector<lights::Light<TSpectral, TFloat, TMeshFloat>*> light_cache_;
        std::unordered_map<MaterialID, MaterialID::ValueType> material_cache_indices_;

        MaterialID::ValueType getMaterialIndex(const MaterialID& materialID)
        {
            return material_cache_indices_[materialID];
        }

        const MeshData& getMeshData(const MeshID& id) const;
        MeshData& getMeshData(const MeshID& id);

        const ManagedData<MATERIAL>& getMaterialData(const MaterialID& id) const;
        ManagedData<MATERIAL>& getMaterialData(const MaterialID& id);

        // Rendering Acceleration Structures:
        bool rebuildTLAS = true;
        std::unique_ptr<rendering::TLAS<TSpectral, TFloat, TMeshFloat>> tlas;

        RTCDevice rtc_device_ = nullptr;

        void recursivelyPopulateCaches(const GROUP* parentGroup);

        void updateMeshLoDs(bool parallelUpodate, size_t numToUpdate);

        bool is_dirty_ = false; // TODO Make this true by default

        std::unique_ptr<geometry::GeometryInterface<TSpectral, TFloat, TMeshFloat>> geometryInterface = nullptr;

        // ====================== //
        // === Asset Counting === //
        // ====================== //
        IDManager<MeshID> mesh_manager_{};
        IDManager<MaterialID> material_manager_{};
        IDManager<LightID> light_manager_{};
        IDManager<GroupID> group_manager_{};
        IDManager<InstanceID> instance_manager_{};
        IDManager<UnresolvedID> unresolved_manager_{};
        IDManager<CameraID> camera_manager_{};

        MeshID allocateMeshID() { return mesh_manager_.allocate(); }
        void deallocateMeshID() { mesh_manager_.deallocate(); }

        MaterialID allocateMaterialID() { return material_manager_.allocate(); }
        void deallocateMaterialID() { material_manager_.deallocate(); }

        LightID allocateLightID() { return light_manager_.allocate(); }
        void deallocateLightID() { light_manager_.deallocate(); }

        GroupID allocateGroupID() { return group_manager_.allocate(); }
        void deallocateGroupID() { group_manager_.deallocate(); }

        InstanceID allocateInstanceID() { return instance_manager_.allocate(); }
        void deallocateInstanceID() { instance_manager_.deallocate(); }

        UnresolvedID allocateUnresolvedID() { return unresolved_manager_.allocate(); }
        void deallocateUnresolvedID() { unresolved_manager_.deallocate(); }

        CameraID allocateCameraID() { return camera_manager_.allocate(); }
        void deallocateCameraID() { camera_manager_.deallocate(); }


        // SPICE ephemeris time:
        double ephemeris_time_ = std::numeric_limits<double>::quiet_NaN();


        // Friend Classes:
        friend class geometry::Mesh<TSpectral, TFloat, TMeshFloat>;
        friend class vira::geometry::GeometryInterface<TSpectral, TFloat, TMeshFloat>;

        friend class scene::Group<TSpectral, TFloat, TMeshFloat>;
        friend class scene::Instance<TSpectral, TFloat, TMeshFloat>;
        friend class scene::LevelOfDetailManager<TSpectral, TFloat, TMeshFloat>;

        friend class rendering::CPUPathTracer<TSpectral, TFloat, TMeshFloat>;
        friend class rendering::CPURasterizer<TSpectral, TFloat, TMeshFloat>;
        friend class rendering::CPUUnresolvedRenderer<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/scene.ipp"

#endif