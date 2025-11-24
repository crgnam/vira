#include <stdexcept>

#include "vira/reference_frame.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/lights/light.hpp"

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Instance<TSpectral, TFloat, TMeshFloat>::Instance(const InstanceID& id, const MeshID& meshID)
        : id_{ id }, meshID_{ meshID }
    {
        
    };
};