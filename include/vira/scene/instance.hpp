#ifndef VIRA_SCENE_INSTANCE_HPP
#define VIRA_SCENE_INSTANCE_HPP

#include "vira/spectral_data.hpp"
#include "vira/reference_frame.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/constraints.hpp"
#include "vira/scene/ids.hpp"

// Forward Declaration:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
}

namespace vira::scene {
    // Forward Declaration:
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Group;

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Instance : public ReferenceFrame<TFloat> {
    public:
        // Delete copying:
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        // Delete moving:
        Instance(Instance&&) = delete;
        Instance& operator=(Instance&&) = delete;

        const InstanceID& getID() const { return id_; }
        const MeshID& getMeshID() const { return meshID_; }

    private:
        Instance(const InstanceID& id, const MeshID& meshID);

        InstanceID id_{};
        MeshID meshID_{};

        void onReferenceFrameChanged() override {
            this->template getScenePtr<TSpectral, TMeshFloat>()->markDirty();
        }

        friend class Group<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/scene/instance.ipp"

#endif