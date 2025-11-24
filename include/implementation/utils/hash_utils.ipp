#include <cstddef>
#include <cstdint>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"

namespace vira::utils {
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
    {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);

    };

    // A simple hashing function for a size_t:
    void hashUint32(size_t& a) {
        a = (a ^ 61) ^ (a >> 16);
        a = a + (a << 3);
        a = a ^ (a >> 4);
        a = a * 0x27d4eb2d;
        a = a ^ (a >> 15);
    };

    // A simple hashing struct for an std::vector<size_t> (for use as key in std::unordered_map)
    size_t hashUint32Vector::operator()(std::vector<size_t> const& vec) const {
        std::size_t seed = vec.size();
        for (auto x : vec) {
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = (x >> 16) ^ x;
            seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    };

    // A simple hashing struct for a vira::Vertex
    template<IsSpectral TSpectral, IsFloat TFloat>
    size_t hashVertex<TSpectral, TFloat>::operator()(vira::geometry::Vertex<TSpectral, TFloat> const& vertex) const
    {
        size_t seed = 0;
        hashCombine(seed, vertex.position, vertex.albedo, vertex.normal, vertex.uv);
        return seed;
    }


    // A function for mapping face indices to a unique color (using the previous defined hash):
    vira::ColorRGB idToColor(size_t id) {
        hashUint32(id);

        uint8_t R = static_cast<uint8_t>(id);
        uint8_t G = static_cast<uint8_t>(id >> 8);
        uint8_t B = static_cast<uint8_t>(id >> 16);

        return vira::ColorRGB{ static_cast<float>(R) / 255.f,
                          static_cast<float>(G) / 255.f,
                          static_cast<float>(B) / 255.f };
    };
};