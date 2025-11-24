#ifndef VIRA_SCENE_GROUP_HPP
#define VIRA_SCENE_GROUP_HPP

#include <memory>
#include <vector>

#include "vira/reference_frame.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/scene/instance.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/scene/ids.hpp"
#include "vira/unresolved/unresolved_object.hpp"
#include "vira/cameras/camera.hpp"

// Forward Declaration:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    namespace geometry {
        template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        class GeometryInterface;
    }
};

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Group : public ReferenceFrame<TFloat> {
        using CAMERA = cameras::Camera<TSpectral, TFloat, TMeshFloat>;
        using LIGHT = lights::Light<TSpectral, TFloat, TMeshFloat>;
        using INSTANCE = Instance<TSpectral, TFloat, TMeshFloat>;
        using UNRESOLVED = unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>;

    public:
        // Delete copying:
        Group(const Group&) = delete;
        Group& operator=(const Group&) = delete;

        // Delete moving:
        Group(Group&&) = delete;
        Group& operator=(Group&&) = delete;

        ~Group() override = default;

        const GroupID& getID() const { return id_; }

        void clear() {} // TODO Implement

        std::string graphToString(std::string prefix = "");


        // ====================== //
        // === Access Methods === //
        // ====================== //
        template <typename IDType>
        const std::string& getName(const IDType& id) const { return this->getData(id).name; }

        template <typename IDType>
        void setName(const IDType& id, std::string newName) { this->getData(id).name = newName; }

        const CAMERA& operator[](const CameraID& id) const { return *this->getData(id).data; }
        CAMERA& operator[](const CameraID& id) { return const_cast<CAMERA&>(static_cast<const Group&>(*this)[id]); }

        const Group& operator[](const GroupID& id) const { return *this->getData(id).data; }
        Group& operator[](const GroupID& id) { return const_cast<Group&>(static_cast<const Group&>(*this)[id]); }

        const INSTANCE& operator[](const InstanceID& id) const { return *this->getData(id).data; }
        INSTANCE& operator[](const InstanceID& id) { return const_cast<INSTANCE&>(static_cast<const Group&>(*this)[id]); }

        const LIGHT& operator[](const LightID& id) const { return *this->getData(id).data; }
        LIGHT& operator[](const LightID& id) { return const_cast<LIGHT&>(static_cast<const Group&>(*this)[id]); }

        const UNRESOLVED& operator[](const UnresolvedID& id) const { return *this->getData(id).data; }
        UNRESOLVED& operator[](const UnresolvedID& id) { return const_cast<UNRESOLVED&>(static_cast<const Group&>(*this)[id]); }


        CameraID searchCameraName(const std::string& name) const { return recursiveSearchName<CameraID>(name); }
        GroupID searchGroupName(const std::string& name) const { return recursiveSearchName<GroupID>(name); }
        InstanceID searchInstanceName(const std::string& name) const { return recursiveSearchName<InstanceID>(name); }
        LightID searchLightName(const std::string& name) const { return recursiveSearchName<LightID>(name); }
        UnresolvedID searchUnresolvedName(const std::string& name) const { return recursiveSearchName<UnresolvedID>(name); }


        // ====================== //
        // === Graph creators === //
        // ====================== //
        CameraID newCamera(std::string name = "") { return this->addCamera(std::make_unique<CAMERA>(), name); }

        CameraID addCamera(std::unique_ptr<CAMERA> camera, std::string name = "");

        GroupID newGroup(std::string name = "") { return newGroup(std::vector<MeshID>{}, name); }
        GroupID newGroup(std::vector<MeshID> meshIDs, std::string name = "");

        InstanceID newInstance(const MeshID& meshID, std::string name = "");

        // Quipus:
        InstanceID addQuipuAsInstance(const fs::path& filepath, const MaterialID& materialID, bool smoothShading, std::string name = "");
        std::vector<InstanceID> addQuipusAsInstances(const fs::path& glob, const MaterialID& materialID, bool smoothShading);

        GroupID loadGeometryAsGroup(const fs::path& filepath, std::string format, bool keep_layout = true, std::string name = "");

        void defineLocalAxes(units::Degree r1, units::Degree r2, units::Degree r3, std::string sequence = "XYZ");
        void defineLocalAxes(vec4<TFloat> quaternion);
        void defineLocalAxes(Rotation<TFloat> rotation);

        LightID addLight(std::unique_ptr<LIGHT> light, std::string name = "");


        // Point Lights:
        LightID newPointLight(float spectralPower, std::string name = "") { return this->newPointLight(TSpectral{ spectralPower }, name); }

        template <IsSpectral TSpectral2>
        LightID newPointLight(TSpectral2 spectralPower, std::function<TSpectral(const TSpectral2&)> converter, std::string name = "");

        template <IsSpectral TSpectral2>
        LightID newPointLight(TSpectral2 spectralPower, std::string name = "");



        // Sphere Lights:
        LightID newSphereLight(float spectralInput, TFloat radius, bool isPower, std::string name = "") { return this->newSphereLight(TSpectral{ spectralInput }, radius, isPower, name); }

        template <IsSpectral TSpectral2>
        LightID newSphereLight(TSpectral2 spectralInput, TFloat radius, bool isPower, std::function<TSpectral(const TSpectral2&)> converter, std::string name = "");

        template <IsSpectral TSpectral2>
        LightID newSphereLight(TSpectral2 spectralInput, TFloat radius, bool isPower, std::string name = "");



        LightID newSun();
        void setSunDirectionEulerAngles(LightID id, units::Degree r1, units::Degree r2, units::Degree r3, double distance = AUtoM<double>())
        {
            vec3<TFloat> sun_position{ 0,0,distance };
            sun_position = Rotation<TFloat>::eulerAngles(r1, r2, r3) * sun_position;
            (*this)[id].setLocalPosition(sun_position);
        }


        // Unresolved Objects:
        UnresolvedID addUnresolvedObject(std::unique_ptr<UNRESOLVED> unresolvedObject, std::string name = "");

        UnresolvedID newUnresolvedObject(std::string name = "") { return this->newUnresolvedObject(0.f, name); }
        UnresolvedID newUnresolvedObject(float irradiance, std::string name = "");

        template <IsSpectral TSpectral2>
        UnresolvedID newUnresolvedObject(TSpectral2 irradiance, std::function<TSpectral(const TSpectral2&)> converter, std::string name = "");

        template <IsSpectral TSpectral2>
        UnresolvedID newUnresolvedObject(TSpectral2 irradiance, std::string name = "");

        // ====================== //
        // === Graph Deleters === //
        // ====================== //
        bool removeCamera(const CameraID& cameraID);
        bool removeGroup(const GroupID& GroupID);
        bool removeInstance(const InstanceID& instanceID);
        bool removeLight(const LightID& lightID);
        bool removeUnresolvedObject(const UnresolvedID& unresolvedID);
        void removeInstancesOfMesh(const MeshID& meshID);

    protected:
        std::unordered_map<CameraID, ManagedData<vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>>> cameras_;
        std::unordered_map<GroupID, ManagedData<Group<TSpectral, TFloat, TMeshFloat>>> groups_;
        std::unordered_map<InstanceID, ManagedData<Instance<TSpectral, TFloat, TMeshFloat>>> instances_;
        std::unordered_map<LightID, ManagedData<vira::lights::Light<TSpectral, TFloat, TMeshFloat>>> lights_;
        std::unordered_map<UnresolvedID, ManagedData<unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>>> unresolved_objects_;

        template<IsTypedID IDType, typename T>
        void processName(std::string& name, const std::unordered_map<IDType, T>& container, const std::string& containerName);

        template<IsTypedID IDType, typename T>
        bool isNameInUse(const std::string& name, const std::unordered_map<IDType, T>& container) const;

        template <IsTypedID IDType>
        const auto& getData(const IDType& id) const;

        template <IsTypedID IDType>
        auto& getData(const IDType& id);

        template <IsTypedID IDType>
        const auto& recursiveSearch(const IDType& id) const;

        template <IsTypedID IDType>
        auto& recursiveSearch(const IDType& id);

        template <IsTypedID IDType>
        IDType recursiveSearchName(const std::string& name) const;

        template <IsTypedID IDType>
        const auto& getContainer() const;

        template <IsTypedID IDType>
        auto& getContainer();

        // Override the ReferenceFrame callback to propagate changes up the tree
        void onReferenceFrameChanged() override;

        void updateChildren();

        void updateSPICE(double et);

    private:
        GroupID id_{};

        Group(GroupID id, vira::Scene<TSpectral, TFloat, TMeshFloat>* scene);

        vira::Scene<TSpectral, TFloat, TMeshFloat>* scenePtr() const;

        void moveCamera(CameraID id, Group* to_group);
        void moveInstance(InstanceID id, Group* to_group);
        void moveLight(LightID id, Group* to_group);
        void moveUnresolved(UnresolvedID id, Group* to_group);
        void moveGroup(GroupID id, Group* to_group);


        friend class vira::Scene<TSpectral, TFloat, TMeshFloat>;
        friend class vira::geometry::GeometryInterface<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/scene/group.ipp"

#endif