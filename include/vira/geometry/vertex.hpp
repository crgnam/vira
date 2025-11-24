#ifndef VIRA_GEOMETRY_VERTEX_HPP
#define VIRA_GEOMETRY_VERTEX_HPP

#include <vector>
#include <cstdint>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"

namespace vira::geometry {
    /**
     * @brief Struct containing position, albedo, normal, and UV data to represent a single vertex
     * 
     * @tparam TMeshFloat Precision Type (float or double)
     * @tparam TSpectral IsSpectral Type
     */
	template <IsSpectral TSpectral, IsFloat TMeshFloat>
	struct Vertex {
		vec3<TMeshFloat> position{}; ///< Vertex Position
        TSpectral albedo{ 1 }; ///< Vertex IsSpectral
		Normal normal{0}; ///< Vertex Normal
	    UV uv{0}; ///< Vertex Texture Coordinates

        /**
         * @brief Operator overload to determine if two vertices are identical
         * 
         * @param other Vertex to be tested
         * @return true 
         * @return false 
         */
		bool operator==(const Vertex& other) const {
			return position == other.position &&
				albedo == other.albedo &&
				normal == other.normal &&
				uv == other.uv;
		}

        template <IsFloat TMeshFloat2>
        operator Vertex<TMeshFloat2, TSpectral>() {
            Vertex<TMeshFloat2, TSpectral> castVertex;
            castVertex.position = this->position;
            castVertex.albedo = this->albedo;
            castVertex.normal = this->normal;
            castVertex.uv = this->uv;
            return castVertex;
        }
	};


    /**
     * @brief A vector of vertices
     * 
     * @tparam TMeshFloat Precision Type (float or double)
     * @tparam TSpectral IsSpectral Type
     */
    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    using VertexBuffer = std::vector<Vertex<TSpectral, TMeshFloat>>;

    /**
     * @brief A vector of indices used to construct triangles
     * 
     * This data structure is common in traditional rendering applications
     */
    typedef std::vector<uint32_t> IndexBuffer;

    /**
     * @brief A vector of indices used to select a set of vertices from a VertexBuffer
     * 
     * This data structure is used in  <a href="https://developer.nvidia.com/blog/introduction-turing-mesh-shaders">meshlet rendering applications</a>
     */
    typedef std::vector<uint32_t> VertexIndexBuffer;

    /**
     * @brief A vector of indices used to select elements from an IndexBuffer
     * 
     * This data structure is used in  <a href="https://developer.nvidia.com/blog/introduction-turing-mesh-shaders">meshlet rendering applications</a>
     */
    typedef std::vector<uint8_t> TriangleIndexBuffer;
};

#endif