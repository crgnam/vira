#ifndef VIRA_UTILS_HASH_UTILS_HPP
#define VIRA_UTILS_HASH_UTILS_HPP

#include <cstddef>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::utils {
    // Must be header implemented as its a generic template:
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest);

    // A simple hashing function for a size_t:
    inline void hashUint32(size_t& a);

    // A simple hashing struct for an std::vector<size_t> (for use as key in std::unordered_map)
    struct hashUint32Vector {
        inline size_t operator()(std::vector<size_t> const& vec) const;
    };

    // A simple hashing struct for a vira::Vertex
    template<IsSpectral TSpectral, IsFloat TFloat>
    struct hashVertex {
        size_t operator()(vira::geometry::Vertex<TSpectral, TFloat> const& vertex) const;
    };

    // A function for mapping face indices to a unique color (using the previous defined hash):
    inline vira::ColorRGB idToColor(size_t id);
};

#include "implementation/utils/hash_utils.ipp"

#endif