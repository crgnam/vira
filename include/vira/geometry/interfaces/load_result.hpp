#ifndef VIRA_GEOMETRY_INTERFACES_LOAD_RESULT_HPP
#define VIRA_GEOMETRY_INTERFACES_LOAD_RESULT_HPP

#include <vector>
#include <string>
#include <limits>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/scene/ids.hpp"

namespace vira::geometry {
    /**
     * @brief A container for loaded mesh data supporting both flat and hierarchical representations.
     *
     * This templated struct manages mesh data in two formats:
     * 1. A flat representation using vectors of mesh IDs and transformations (for backwards compatibility)
     * 2. A hierarchical scene graph representation using nodes with parent-child relationships
     *
     * The struct supports loading mesh data from formats that may contain scene hierarchies
     * while maintaining compatibility with systems that expect flat mesh arrays.
     *
     * @tparam TFloat The floating-point type used for transformations (must satisfy IsFloat concept)
     *
     * @note This struct provides both representations simultaneously - the flat vectors contain
     *       all meshes while the node hierarchy describes their relationships and local transforms.
     *
     * @see Node for the hierarchical representation structure
     */
    template <IsFloat TFloat>
    struct LoadedMeshes {
        LoadedMeshes(size_t num_meshes)
            : mesh_ids(num_meshes),
            transformations(num_meshes)
        {
        }

        // Existing flat data (for backwards compatibility)
        std::vector<vira::MeshID> mesh_ids;
        std::vector<vira::mat4<TFloat>> transformations;

        // New: Graph structure data
        struct Node {
            std::string name;
            std::vector<size_t> mesh_indices;         // Indices into mesh_ids
            vira::mat4<TFloat> local_transform;
            std::vector<size_t> children;            // Indices into nodes vector
            size_t parent = std::numeric_limits<size_t>::max(); // Index of parent, or max for root

            Node() = default;
            Node(const std::string& node_name, const vira::mat4<TFloat>& transform)
                : name(node_name), local_transform(transform) {
            }
        };

        std::vector<Node> nodes;
        size_t root_node = 0;

        // Helper methods
        bool has_hierarchy() const { return !nodes.empty(); }

        const Node& get_root() const {
            if (nodes.empty()) throw std::runtime_error("No hierarchy data available");
            return nodes[root_node];
        }

        // Get all root nodes (nodes with no parent)
        std::vector<size_t> get_root_nodes() const {
            std::vector<size_t> roots;
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].parent == std::numeric_limits<size_t>::max()) {
                    roots.push_back(i);
                }
            }
            return roots;
        }

        // Check if there are multiple root nodes
        bool has_multiple_roots() const {
            return get_root_nodes().size() > 1;
        }
    };
}

#endif