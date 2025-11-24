#ifndef VIRA_RENDERING_ACCELERATION_TLAS_HPP
#define VIRA_RENDERING_ACCELERATION_TLAS_HPP

#include <vector>
#include <memory>
#include <cstdint>

#include "vira/constraints.hpp"
#include "vira/rendering/acceleration/nodes.hpp"
#include "vira/rendering/ray.hpp"

// Forward Declare:
namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
};

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class TLAS {
    public:
        virtual ~TLAS() = default;

        virtual void intersect(Ray<TSpectral, TFloat>& ray) = 0;

        virtual void init() {}

    protected:
        bool device_freed_ = false;
        friend class vira::Scene<TSpectral, TFloat, TMeshFloat>;
    };
};

#endif
