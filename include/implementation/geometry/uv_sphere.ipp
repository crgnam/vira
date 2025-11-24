#include <cstddef>
#include <memory>
#include <vector>

#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/utils/utils.hpp"

namespace vira::geometry {
    IndexBuffer uvSphereIndexBuffer(size_t numCuts, size_t numRings)
    {
        size_t numVertices = numCuts * numRings;
        IndexBuffer indexBuffer(3 * 2 * numVertices);

        uint32_t rx = static_cast<uint32_t>(numRings);
        uint32_t ry = static_cast<uint32_t>(numCuts);

        size_t idx = 0;
        for (uint32_t i = 0; i < rx; i++) {
            for (uint32_t j = 0; j < ry; j++) {
                // Define the first triangle of the quad:
                uint32_t f0 = i + (j + 1) * rx;
                uint32_t f1 = i + 1 + j * rx;
                uint32_t f2 = i + j * rx;

                indexBuffer[idx + 0] = f0;
                indexBuffer[idx + 1] = f1;
                indexBuffer[idx + 2] = f2;
                idx = idx + 3;

                // Define the second triangle of the quad:
                f0 = i + 1 + j * rx;
                f1 = i + (j + 1) * rx;
                f2 = i + 1 + (j + 1) * rx;

                indexBuffer[idx + 0] = f0;
                indexBuffer[idx + 1] = f1;
                indexBuffer[idx + 2] = f2;
                idx = idx + 3;

            };
        };

        return indexBuffer;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::unique_ptr<Mesh<TSpectral, TFloat, TMeshFloat>> makeUVSphere(TMeshFloat radius = 1, size_t numCuts = 32, size_t numRings = 16)
    {
        // TODO Wrap the face definitions:
        numCuts = numCuts + 2;
        numRings = numRings + 2;

        IndexBuffer ib = uvSphereIndexBuffer(numCuts, numRings);

        // Pre-compute vertices:
        std::vector<TMeshFloat> longitudes = linspace<TMeshFloat>(0, 360, numCuts - 1);
        std::vector<TMeshFloat> latitudes = linspace<TMeshFloat>(-90, 90, numRings - 1);
        size_t N = numCuts * numRings;
        VertexBuffer<TSpectral, TMeshFloat> vb(N);
        size_t k = 0;
        for (TMeshFloat lon : longitudes) {
            for (TMeshFloat lat : latitudes) {
                vb[k].position = utils::ellipsoidToCartesian<TMeshFloat>(lon, lat, 0, radius, radius);
                vb[k].uv = UV{ lon / 360, (lat + 90) / 180 };
                k++;
            }
        }
        return std::make_shared<Mesh<TSpectral, TFloat, TMeshFloat>>(vb, ib);
    };
};