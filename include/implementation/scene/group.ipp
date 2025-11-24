#include <memory>
#include <algorithm>
#include <stdexcept>
#include <chrono>

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/scene/instance.hpp"
#include "vira/scene/ids.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/geometry/interfaces/geometry_interface.hpp"
#include "vira/quipu/dem_quipu.hpp"

#include "vira/lights/light.hpp"
#include "vira/lights/point_light.hpp"
#include "vira/lights/sphere_light.hpp"

#include "vira/utils/print_utils.hpp"
#include "vira/dems/set_proj_data.hpp"

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Group<TSpectral, TFloat, TMeshFloat>::Group(GroupID id, vira::Scene<TSpectral, TFloat, TMeshFloat>* scene)
        : id_{ id }
    {
        this->scene_ = scene;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::string Group<TSpectral, TFloat, TMeshFloat>::graphToString(std::string prefix)
    {
        std::string output;

        // Collect all non-group items
        std::vector<std::pair<std::string, std::string>> items; // {type, name}

        for (const auto& [id, data] : instances_) {
            items.push_back({ "Instance", "\"" + data.name + "\"" });
        }
        for (const auto& [id, data] : lights_) {
            items.push_back({ "Light", "\"" + data.name + "\"" });
        }
        for (const auto& [id, data] : cameras_) {
            items.push_back({ "Camera", "\"" + data.name + "\"" });
        }
        for (const auto& [id, data] : unresolved_objects_) {
            items.push_back({ "Unresolved", "\"" + data.name + "\"" });
        }

        // Display items
        for (size_t i = 0; i < items.size(); ++i) {
            bool isLastItem = (i == items.size() - 1) && groups_.empty();
            output += prefix + (isLastItem ? "+-- " : "|-- ") +
                items[i].first + ": " + items[i].second + "\n";
        }

        // Display groups
        size_t groupIndex = 0;
        for (const auto& [id, data] : groups_) {
            bool isLastGroup = (groupIndex == groups_.size() - 1);

            output += prefix + (isLastGroup ? "+-- " : "|-- ") +
                "Group: \"" + data.name + "\"\n";

            std::string childPrefix = prefix + (isLastGroup ? "    " : "|   ");
            output += data.data->graphToString(childPrefix);

            ++groupIndex;
        }

        return output;
    }

    // ====================== //
    // === Access Methods === //
    // ====================== //
    template<typename>
    inline constexpr bool always_false_v = false;

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType>
    auto& Group<TSpectral, TFloat, TMeshFloat>::getContainer() {
        if constexpr (std::is_same_v<IDType, GroupID>) {
            return groups_;
        }
        else if constexpr (std::is_same_v<IDType, InstanceID>) {
            return instances_;
        }
        else if constexpr (std::is_same_v<IDType, LightID>) {
            return lights_;
        }
        else if constexpr (std::is_same_v<IDType, UnresolvedID>) {
            return unresolved_objects_;
        }
        else if constexpr (std::is_same_v<IDType, CameraID>) {
            return cameras_;
        }
        else {
            static_assert(always_false_v<IDType>, "Unsupported ID type");
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType>
    const auto& Group<TSpectral, TFloat, TMeshFloat>::getContainer() const {
        if constexpr (std::is_same_v<IDType, GroupID>) {
            return groups_;
        }
        else if constexpr (std::is_same_v<IDType, InstanceID>) {
            return instances_;
        }
        else if constexpr (std::is_same_v<IDType, LightID>) {
            return lights_;
        }
        else if constexpr (std::is_same_v<IDType, UnresolvedID>) {
            return unresolved_objects_;
        }
        else if constexpr (std::is_same_v<IDType, CameraID>) {
            return cameras_;
        }
        else {
            static_assert(always_false_v<IDType>, "Unsupported ID type");
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType>
    const auto& Group<TSpectral, TFloat, TMeshFloat>::getData(const IDType& id) const
    {
        const auto& container = getContainer<IDType>();
        auto it = container.find(id);
        if (it == container.end()) {
            throw std::out_of_range(id.name() + " not found in " + id_.name());
        }

        return it->second;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType>
    auto& Group<TSpectral, TFloat, TMeshFloat>::getData(const IDType& id)
    {
        auto& container = getContainer<IDType>();
        auto it = container.find(id);
        if (it == container.end()) {
            throw std::out_of_range(id.name() + " not found in " + id_.name());
        }

        return it->second;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsTypedID IDType>
    const auto& Group<TSpectral, TFloat, TMeshFloat>::recursiveSearch(const IDType& id) const
    {
        // First, try to find the object in this group
        const auto& container = getContainer<IDType>();
        auto it = container.find(id);
        if (it != container.end()) {
            return *it->second.data.get();
        }

        // If not found in this group, search recursively in all child groups
        for (const auto& [groupId, groupData] : groups_) {
            try {
                return groupData.data->recursiveSearch(id);
            }
            catch (const std::out_of_range&) {
                // Continue searching in other groups
                continue;
            }
        }

        // If we reach here, the object was not found anywhere in the tree
        throw std::out_of_range(id.name() + " not found in scene tree");
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsTypedID IDType>
    auto& Group<TSpectral, TFloat, TMeshFloat>::recursiveSearch(const IDType& id)
    {
        // First, try to find the object in this group
        auto& container = getContainer<IDType>();
        auto it = container.find(id);
        if (it != container.end()) {
            return *it->second.data.get();
        }

        // If not found in this group, search recursively in all child groups
        for (auto& [groupId, groupData] : groups_) {
            try {
                return groupData.data->recursiveSearch(id);
            }
            catch (const std::out_of_range&) {
                // Continue searching in other groups
                continue;
            }
        }

        // If we reach here, the object was not found anywhere in the tree
        throw std::out_of_range(id.name() + " not found in scene tree");
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsTypedID IDType>
    IDType Group<TSpectral, TFloat, TMeshFloat>::recursiveSearchName(const std::string& name) const
    {
        // First, try to find the object in this group
        auto& container = getContainer<IDType>();

        for (auto& [id, data] : container) {
            if (name == data.name) {
                return id;
            }
        }

        // If not found in this group, search recursively in all child groups
        for (auto& [groupId, groupData] : groups_) {
            try {
                return groupData.data->template recursiveSearchName<IDType>(name);
            }
            catch (const std::out_of_range&) {
                // Continue searching in other groups
                continue;
            }
        }

        throw std::out_of_range("No " + std::string(IDType::type_name()) + " with corresponding name " + name + " was found in graph");
    };


    // ====================== //
    // === Graph creators === //
    // ====================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    CameraID Group<TSpectral, TFloat, TMeshFloat>::addCamera(std::unique_ptr<vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>> camera, std::string name) {
        this->processName(name, cameras_, "Camera");

        CameraID cameraID = this->scenePtr()->allocateCameraID();
        camera->id_ = cameraID;
        camera->setParent(this);

        ManagedData<vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>> cameraData(std::move(camera), name);

        cameras_.emplace(cameraID, std::move(cameraData));
        scenePtr()->markDirty();

        return cameraID;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    GroupID Group<TSpectral, TFloat, TMeshFloat>::newGroup(std::vector<MeshID> meshIDs, std::string name) {
        this->processName(name, groups_, "Group");

        GroupID newID = this->scenePtr()->allocateGroupID();

        std::unique_ptr<Group<TSpectral, TFloat, TMeshFloat>> group(new Group<TSpectral, TFloat, TMeshFloat>(newID, this->scenePtr()));
        group->setParent(this);

        ManagedData<Group<TSpectral, TFloat, TMeshFloat>> newEntry(std::move(group), name);

        groups_.emplace(newID, std::move(newEntry));
        scenePtr()->markDirty();

        for (const MeshID& newMeshID : meshIDs) {
            this->operator[](newID).newInstance(newMeshID);
        }

        return newID;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    InstanceID Group<TSpectral, TFloat, TMeshFloat>::newInstance(const MeshID& meshID, std::string name) {
        if (!this->scenePtr()) {
            throw std::runtime_error("Group does not belong to any Scene");
        }

        if (!this->scenePtr()->hasMesh(meshID)) {
            throw std::invalid_argument("Mesh '" + meshID.name() + "' not found in scene");
        }

        this->processName(name, instances_, "Instance");

        InstanceID newID = this->scenePtr()->allocateInstanceID();

        std::unique_ptr<Instance<TSpectral, TFloat, TMeshFloat>> instance(new Instance<TSpectral, TFloat, TMeshFloat>(newID, meshID));
        instance->setParent(this);

        ManagedData<Instance<TSpectral, TFloat, TMeshFloat>> newEntry(std::move(instance), name);

        instances_.emplace(newID, std::move(newEntry));
        scenePtr()->markDirty();

        return newID;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    InstanceID Group<TSpectral, TFloat, TMeshFloat>::addQuipuAsInstance(const fs::path& filepath, const MaterialID& materialID, bool smoothShading, std::string name)
    {
        vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu(filepath);

        MeshID meshID = this->scenePtr()->addQuipuMesh(quipu, materialID, smoothShading, name);

        InstanceID instanceID = newInstance(meshID);
        (*this)[instanceID].setLocalTransformation(quipu.getTransformation());

        return instanceID;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<InstanceID> Group<TSpectral, TFloat, TMeshFloat>::addQuipusAsInstances(const fs::path& glob, const MaterialID& materialID, bool smoothShading)
    {
        vira::dems::setProjDataSearchPaths();

        auto files = vira::utils::getFiles(glob);

        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point stop_time;
        std::unique_ptr<indicators::ProgressBar> progressBar;
        if (vira::getPrintStatus()) {
            std::cout << "Loading " << files.size() << " quipu files...\n" << std::flush;
            start_time = std::chrono::high_resolution_clock::now();
            progressBar = vira::print::makeProgressBar("Loading quipu files");
        }

        const size_t totalFiles = files.size();
        const size_t progressUpdateInterval = std::max(static_cast<size_t>(1), totalFiles / 10); // Update every 10%
        size_t lastReportedProgress = 0;

        std::vector<InstanceID> instanceIDsOutput(files.size());
        for (size_t i = 0; i < files.size(); ++i) {
            instanceIDsOutput[i] = addQuipuAsInstance(files[i], materialID, smoothShading);

            // Update progress every 10%
            if (vira::getPrintStatus() && progressBar && (i + 1 - lastReportedProgress >= progressUpdateInterval)) {
                float percentage = (static_cast<float>(i + 1) / static_cast<float>(totalFiles)) * 100.0f;
                vira::print::updateProgressBar(progressBar, percentage);
                lastReportedProgress = i + 1;
            }
        }

        if (vira::getPrintStatus()) {
            stop_time = std::chrono::high_resolution_clock::now();
            auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
            vira::print::updateProgressBar(progressBar, "Completed (" + std::to_string(duration.count()) + " ms)", 100.0f);
        }

        return instanceIDsOutput;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    GroupID Group<TSpectral, TFloat, TMeshFloat>::loadGeometryAsGroup(const fs::path& filepath, std::string format, bool keep_layout, std::string name)
    {
        auto loaded_meshes = this->scenePtr()->loadGeometry(filepath, format);

        GroupID new_id;

        if (loaded_meshes.has_hierarchy() && keep_layout) {
            // Lambda for recursive processing of nodes
            std::function<void(size_t, Group&)> processNode = [&](size_t node_index, Group& current_parent) {
                const auto& node = loaded_meshes.nodes[node_index];

                // Create a new group for this node
                std::string new_name = node.name.empty() ? ("Node_" + std::to_string(node_index)) : node.name;
                auto child_id = current_parent.newGroup(new_name);
                current_parent.setName(child_id, new_name);
                Group& current_group = current_parent[child_id];

                // Apply the local transform to this group
                current_group.setLocalTransformation(node.local_transform);

                // Create instances for all meshes associated with this node
                for (size_t mesh_idx : node.mesh_indices) {
                    if (mesh_idx < loaded_meshes.mesh_ids.size()) {
                        current_group.newInstance(loaded_meshes.mesh_ids[mesh_idx]);
                    }
                }

                // Recursively process all child nodes
                for (size_t child_index : node.children) {
                    if (child_index < loaded_meshes.nodes.size()) {
                        processNode(child_index, current_group);
                    }
                }

                return child_id;
                };

            // Handle multiple root nodes
            auto root_nodes = loaded_meshes.get_root_nodes();

            if (root_nodes.empty()) {
                throw std::runtime_error("No root nodes found in loaded mesh hierarchy");
            }
            else if (root_nodes.size() == 1) {
                // Single root case - process normally
                new_id = this->newGroup(name.empty() ? loaded_meshes.nodes[root_nodes[0]].name : name);
                Group& root_group = (*this)[new_id];

                const auto& root_node = loaded_meshes.nodes[root_nodes[0]];
                root_group.setLocalTransformation(root_node.local_transform);

                // Create instances for root node meshes
                for (size_t mesh_idx : root_node.mesh_indices) {
                    if (mesh_idx < loaded_meshes.mesh_ids.size()) {
                        root_group.newInstance(loaded_meshes.mesh_ids[mesh_idx]);
                    }
                }

                // Process children
                for (size_t child_index : root_node.children) {
                    if (child_index < loaded_meshes.nodes.size()) {
                        processNode(child_index, root_group);
                    }
                }
            }
            else {
                // Multiple roots case - create a container group
                new_id = this->newGroup(name.empty() ? "MultiRootContainer" : name);
                Group& container_group = (*this)[new_id];

                // Process each root as a separate subgroup
                for (size_t root_idx : root_nodes) {
                    processNode(root_idx, container_group);
                }
            }
        }
        else {
            // Pre-flattened representation (common for certain formats, especially DSK):
            new_id = this->newGroup(name);
            for (size_t i = 0; i < loaded_meshes.mesh_ids.size(); ++i) {
                auto instanceID = (*this)[new_id].newInstance(loaded_meshes.mesh_ids[i]);
                (*this)[new_id][instanceID].setLocalTransformation(loaded_meshes.transformations[i]);
            }
        }

        return new_id;
    }


    // ================== //
    // === Local Axes === //
    // ================== //    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::defineLocalAxes(units::Degree r1, units::Degree r2, units::Degree r3, std::string sequence)
    {
        this->defineLocalAxes(Rotation<TFloat>::eulerAngles(r1, r2, r3, sequence));
    };
    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::defineLocalAxes(vec4<TFloat> quaternion)
    {
        this->defineLocalAxes(Rotation<TFloat>::quaternion(quaternion));
    };
    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::defineLocalAxes(Rotation<TFloat> rotation)
    {
        // Check if identity.  If so, just exit (that is our default):
        if (rotation == Rotation<TFloat>{}) {
            return;
        }

        // TODO Move names to the actual objects themselves and keep registry at scene level  "TEMP" + " - LocalAxes"
        GroupID local_axes = this->newGroup("LocalAxes");
        //rotation = rotation.inverse();
        (*this)[local_axes].setLocalRotation(rotation);

        // Move all children to this new subgroup:
        std::vector<CameraID> camera_ids;
        for (const auto& [id, data] : cameras_) {
            camera_ids.push_back(id);
        }
        for (const auto& id : camera_ids) {
            scenePtr()->moveCamera(id, getID(), local_axes);
        }

        std::vector<InstanceID> instance_ids;
        for (const auto& [id, data] : instances_) {
            instance_ids.push_back(id);
        }
        for (const auto& id : instance_ids) {
            scenePtr()->moveInstance(id, getID(), local_axes);
        }

        std::vector<LightID> light_ids;
        for (const auto& [id, data] : lights_) {
            light_ids.push_back(id);
        }
        for (const auto& id : light_ids) {
            scenePtr()->moveLight(id, getID(), local_axes);
        }

        std::vector<UnresolvedID> unresolved_ids;
        for (const auto& [id, data] : unresolved_objects_) {
            unresolved_ids.push_back(id);
        }
        for (const auto& id : unresolved_ids) {
            scenePtr()->moveUnresolvedObject(id, getID(), local_axes);
        }

        std::vector<GroupID> group_ids;
        for (const auto& [id, group_data] : groups_) {
            if (id != local_axes) {
                group_ids.push_back(id);
            }
        }
        for (const auto& id : group_ids) {
            scenePtr()->moveGroup(id, getID(), local_axes);
        }
    };



    // ============== //
    // === Lights === //
    // ============== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    LightID Group<TSpectral, TFloat, TMeshFloat>::addLight(std::unique_ptr<vira::lights::Light<TSpectral, TFloat, TMeshFloat>> light, std::string name)
    {
        this->processName(name, lights_, "Light");

        LightID newID = this->scenePtr()->allocateLightID();
        light->id_ = newID;
        light->setParent(this);

        ManagedData<vira::lights::Light<TSpectral, TFloat, TMeshFloat>> newEntry(std::move(light), name);

        lights_.emplace(newID, std::move(newEntry));
        scenePtr()->markDirty();

        return newID;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    LightID Group<TSpectral, TFloat, TMeshFloat>::newPointLight(TSpectral2 spectralPower, std::function<TSpectral(const TSpectral2&)> converter, std::string name)
    {
        TSpectral usableSpectralPower;
        if constexpr (std::is_same_v<TSpectral2, TSpectral>) {
            usableSpectralPower = spectralPower;
        }
        else {
            usableSpectralPower = converter(spectralPower);
        }
        auto newLight = std::make_unique<lights::PointLight<TSpectral, TFloat, TMeshFloat>>(usableSpectralPower);
        return this->addLight(std::move(newLight), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    LightID Group<TSpectral, TFloat, TMeshFloat>::newPointLight(TSpectral2 spectralPower, std::string name)
    {
        TSpectral usableSpectralPower = spectralConvert_val<TSpectral2, TSpectral>(spectralPower);
        auto newLight = std::make_unique<lights::PointLight<TSpectral, TFloat, TMeshFloat>>(usableSpectralPower);
        return this->addLight(std::move(newLight), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    LightID Group<TSpectral, TFloat, TMeshFloat>::newSphereLight(TSpectral2 spectralInput, TFloat radius, bool isPower, std::function<TSpectral(const TSpectral2&)> converter, std::string name)
    {
        TSpectral usableSpectralInput;
        if constexpr (std::is_same_v<TSpectral2, TSpectral>) {
            usableSpectralInput = spectralInput;
        }
        else {
            usableSpectralInput = converter(spectralInput);
        }
        auto newLight = std::make_unique<lights::SphereLight<TSpectral, TFloat, TMeshFloat>>(usableSpectralInput, radius, isPower);
        return this->addLight(std::move(newLight), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    LightID Group<TSpectral, TFloat, TMeshFloat>::newSphereLight(TSpectral2 spectralInput, TFloat radius, bool isPower, std::string name)
    {
        TSpectral usableSpectralPower = spectralConvert_val<TSpectral2, TSpectral>(spectralInput);
        auto newLight = std::make_unique<lights::SphereLight<TSpectral, TFloat, TMeshFloat>>(usableSpectralPower, radius, isPower);
        return this->addLight(std::move(newLight), name);
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    LightID Group<TSpectral, TFloat, TMeshFloat>::newSun()
    {
        TFloat radius = SOLAR_RADIUS();
        TSpectral spectral_radiance = blackBodyRadiance<TSpectral>(5800, 1000);

        return this->newSphereLight(spectral_radiance, radius, false, "Sun");
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    UnresolvedID Group<TSpectral, TFloat, TMeshFloat>::addUnresolvedObject(std::unique_ptr<UNRESOLVED> unresolved_object, std::string name)
    {
        this->processName(name, unresolved_objects_, "UnresolvedObject");

        UnresolvedID new_id = this->scenePtr()->allocateUnresolvedID();

        unresolved_object->setParent(this);

        ManagedData<unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>> new_entry(std::move(unresolved_object), name);

        unresolved_objects_.emplace(new_id, std::move(new_entry));
        scenePtr()->markDirty();

        return new_id;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    UnresolvedID Group<TSpectral, TFloat, TMeshFloat>::newUnresolvedObject(float irradiance, std::string name)
    {
        auto unresolvedObject = std::make_unique<unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>>(TSpectral{ irradiance });
        return addUnresolvedObject(std::move(unresolvedObject), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    UnresolvedID Group<TSpectral, TFloat, TMeshFloat>::newUnresolvedObject(TSpectral2 irradiance, std::function<TSpectral(const TSpectral2&)> converter, std::string name)
    {
        TSpectral usableIrradiance;
        if constexpr (std::is_same_v<TSpectral2, TSpectral>) {
            usableIrradiance = irradiance;
        }
        else {
            usableIrradiance = converter(irradiance);
        }
        auto unresolvedObject = std::make_unique<unresolved::UnresolvedObject<TSpectral, TFloat, TMeshFloat>>(usableIrradiance);
        return addUnresolvedObject(std::move(unresolvedObject), name);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsSpectral TSpectral2>
    UnresolvedID Group<TSpectral, TFloat, TMeshFloat>::newUnresolvedObject(TSpectral2 irradiance, std::string name)
    {
        auto converter = [](const TSpectral2& input) -> TSpectral {
            return spectralConvert_val<TSpectral2, TSpectral>(input);
            };
        return this->newUnresolvedObject<TSpectral>(irradiance, converter, name);
    };


    // ====================== //
    // === Graph Deleters === //
    // ====================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Group<TSpectral, TFloat, TMeshFloat>::removeCamera(const CameraID& cameraID)
    {
        auto it = cameras_.find(cameraID);
        if (it == cameras_.end()) {
            return false;
        }
        cameras_.erase(it);
        scenePtr()->markDirty();

        this->scenePtr()->deallocateCameraID();

        return true;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Group<TSpectral, TFloat, TMeshFloat>::removeGroup(const GroupID& groupID) {
        auto it = groups_.find(groupID);
        if (it == groups_.end()) {
            return false;
        }
        groups_.erase(it);
        scenePtr()->markDirty();

        this->scenePtr()->deallocateGroupID();

        return true;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Group<TSpectral, TFloat, TMeshFloat>::removeInstance(const InstanceID& instanceID) {
        auto it = instances_.find(instanceID);
        if (it == instances_.end()) {
            return false;
        }
        instances_.erase(it);
        scenePtr()->markDirty();

        this->scenePtr()->deallocateInstanceID();

        return true;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Group<TSpectral, TFloat, TMeshFloat>::removeLight(const LightID& lightID) {
        auto it = lights_.find(lightID);
        if (it == lights_.end()) {
            return false;
        }
        lights_.erase(it);
        scenePtr()->markDirty();

        this->scenePtr()->deallocateLightID();

        return true;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Group<TSpectral, TFloat, TMeshFloat>::removeUnresolvedObject(const UnresolvedID& unresolvedID) {
        auto it = unresolved_objects_.find(unresolvedID);
        if (it == unresolved_objects_.end()) {
            return false;
        }
        unresolved_objects_.erase(it);
        scenePtr()->markDirty();

        this->scenePtr()->deallocateUnresolvedID();

        return true;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::removeInstancesOfMesh(const MeshID& meshID) {
        // This is called when a mesh is deleted

        // Remove instances in this group
        auto it = instances_.begin();
        while (it != instances_.end()) {
            if (it->second.data->getMeshID() == meshID) {
                it = instances_.erase(it);
                this->scenePtr()->deallocateInstanceID();
            }
            else {
                ++it;
            }
        }

        // Recursively remove from child groups
        for (auto& [groupID, groupData] : groups_) {
            groupData.data->removeInstancesOfMesh(meshID);
        }

        scenePtr()->markDirty();
    }


    // ========================= //
    // === Protected Helpers === //
    // ========================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::onReferenceFrameChanged() {
        this->scenePtr()->markDirty();
        this->updateChildren();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::updateChildren()
    {
        for (auto& [id, camera_data] : cameras_) {
            camera_data.data->updateGlobalTransformation();
        }

        for (auto& [id, instance_data] : instances_) {
            instance_data.data->updateGlobalTransformation();
        }

        for (auto& [id, light_data] : lights_) {
            light_data.data->updateGlobalTransformation();
        }

        for (auto& [id, unresolved_data] : unresolved_objects_) {
            unresolved_data.data->updateGlobalTransformation();
        }

        for (auto& [id, group_data] : groups_) {
            group_data.data->updateGlobalTransformation();
            group_data.data->updateChildren();
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::updateSPICE(double et)
    {
        for (auto& [id, camera_data] : cameras_) {
            camera_data.data->updateSPICEStates(et);
        }

        for (auto& [id, instance_data] : instances_) {
            instance_data.data->updateSPICEStates(et);
        }

        for (auto& [id, light_data] : lights_) {
            light_data.data->updateSPICEStates(et);
        }

        for (auto& [id, unresolved_data] : unresolved_objects_) {
            unresolved_data.data->updateSPICEStates(et);
        }

        for (auto& [id, group_data] : groups_) {
            group_data.data->updateSPICEStates(et);
            group_data.data->updateSPICE(et);
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType, typename T>
    void Group<TSpectral, TFloat, TMeshFloat>::processName(std::string& name, const std::unordered_map<IDType, T>& container, const std::string& containerName) {
        if (name.empty()) {
            size_t num = container.size() + 1;
            for (size_t i = 0; i < 100; ++i) {
                name = containerName + "." + utils::padZeros<3>(num);
                if (!isNameInUse(name, container)) {
                    break;
                }
                num++;
            }
        }

        if (isNameInUse(name, container)) {
            throw std::invalid_argument(containerName + " name '" + name + "' is already in use");
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template<IsTypedID IDType, typename T>
    bool Group<TSpectral, TFloat, TMeshFloat>::isNameInUse(const std::string& name, const std::unordered_map<IDType, T>& container) const {
        for (const auto& [id, value] : container) {
            if (id.name() == name) {
                return true;
            }
        }
        return false;
    }

    // ========================== //
    // === Private Operations === //
    // ========================== //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::Scene<TSpectral, TFloat, TMeshFloat>* Group<TSpectral, TFloat, TMeshFloat>::scenePtr() const
    {
        return this->template getScenePtr<TSpectral, TMeshFloat>();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::moveCamera(CameraID id, Group* to_group)
    {
        to_group->cameras_.emplace(id, std::move(this->getData(id)));
        auto it = cameras_.find(id);
        cameras_.erase(it);

        scenePtr()->markDirty();

        (*to_group)[id].setParent(to_group);
        (*to_group)[id].updateGlobalTransformation();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::moveInstance(InstanceID id, Group* to_group)
    {
        to_group->instances_.emplace(id, std::move(this->getData(id)));
        auto it = instances_.find(id);
        instances_.erase(it);

        scenePtr()->markDirty();

        (*to_group)[id].setParent(to_group);
        (*to_group)[id].updateGlobalTransformation();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::moveLight(LightID id, Group* to_group)
    {
        to_group->lights_.emplace(id, std::move(this->getData(id)));
        auto it = lights_.find(id);
        lights_.erase(it);

        scenePtr()->markDirty();

        (*to_group)[id].setParent(to_group);
        (*to_group)[id].updateGlobalTransformation();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::moveUnresolved(UnresolvedID id, Group* to_group)
    {
        to_group->unresolved_objects_.emplace(id, std::move(this->getData(id)));
        auto it = unresolved_objects_.find(id);
        unresolved_objects_.erase(it);

        scenePtr()->markDirty();

        (*to_group)[id].setParent(to_group);
        (*to_group)[id].updateGlobalTransformation();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Group<TSpectral, TFloat, TMeshFloat>::moveGroup(GroupID id, Group* to_group)
    {
        to_group->groups_.emplace(id, std::move(this->getData(id)));
        auto it = groups_.find(id);
        groups_.erase(it);

        scenePtr()->markDirty();

        (*to_group)[id].setParent(to_group);
        (*to_group)[id].updateGlobalTransformation();
        (*to_group)[id].updateChildren();
    }
};