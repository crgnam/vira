#ifndef VIRA_SCENE_LIGHT_HPP
#define VIRA_SCENE_LIGHT_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/reference_frame.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/scene/ids.hpp"

// Forward Decalartion:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
}

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Group;
}

namespace vira::lights {
    enum LightType {
        POINT_LIGHT,
        SPHERE_LIGHT
    };
    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Light : public vira::ReferenceFrame<TFloat> {
    public:
        Light() = default;

        // Delete copying:
        Light(const Light&) = delete;
        Light& operator=(const Light&) = delete;
        
        // Allow moving:
        Light(Light&&) = default;
        Light& operator=(Light&&) = default;

        virtual TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf) = 0;
        virtual TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist) = 0;

        virtual float getPDF(const vec3<TFloat>& intersection, const vec3<TFloat>& direction) = 0;

        virtual TSpectral getIrradiance(const vec3<TFloat>& point) = 0;

        virtual LightType getType() const = 0;

        const LightID& getID() const { return id_; }

    protected:
        LightID id_{};
        
        void onReferenceFrameChanged() override {
            this->template getScenePtr<TSpectral, TMeshFloat>()->markDirty();
        }

        friend class vira::scene::Group<TSpectral, TFloat, TMeshFloat>;
    };
};

#endif