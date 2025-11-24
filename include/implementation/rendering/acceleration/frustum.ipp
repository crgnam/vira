#include <array>
#include <cmath>
#include <cstddef>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/obb.hpp"

namespace vira::rendering {
    template <IsFloat TFloat>
    Plane<TFloat>::Plane(const vec3<TFloat>& p1, const vec3<TFloat>& p2, const vec3<TFloat>& p3) {
        normal_ = normalize(cross(p2 - p1, p3 - p1));
        distance_ = dot(normal_, p1);
    };

    template <IsFloat TFloat>
    Plane<TFloat>::Plane(vec3<TFloat> normal, TFloat distance) :
        normal_{ normal }, distance_{ distance }
    {

    };

    template <IsFloat TFloat>
    bool Plane<TFloat>::inside(const vec3<TFloat>& point) const {
        return dot(normal_, point) + distance_ <= 0;
    };

    template <IsFloat TFloat, IsSpectral TSpectral, size_t N>
    bool intersectsFrustum(const AABB<TSpectral, TFloat>& aabb, const std::array<Plane<TFloat>, N>& frustumPlanes)
    {
        auto corners = aabb.getCorners();

        // For each plane in the frustum
        for (const auto& plane : frustumPlanes) {
            int cornersOutside = 0;
            for (const auto& corner : corners) {
                if (!plane.inside(corner)) {
                    cornersOutside++;
                }
                else {
                    break;
                }
            }

            // If all corners are outside of this plane, the AABB is completely outside the frustum
            if (cornersOutside == 8) {
                return false;
            }
        }

        return true;
    };

    template <IsFloat TFloat, size_t N>
    bool intersectsFrustum(const OBB<TFloat>& obb, const std::array<Plane<TFloat>, N>& frustumPlanes)
    {
        // For each plane in the frustum
        for (const auto& plane : frustumPlanes) {
            vec3<TFloat> normal_ = plane.getNormal();
            TFloat distance_ = plane.getDistance();

            // Project box onto plane normal_
            TFloat radius = 0;
            for (int i = 0; i < 3; ++i) {
                // Project each OBB axis onto the plane normal_
                radius += std::fabs(obb.halfSize()[i] * dot(normal_, obb.axes()[static_cast<size_t>(i)]));
            }

            // Calculate distance_ from OBB center to plane
            TFloat center_distance = dot(normal_, obb.center()) + distance_;

            // If box is completely behind the plane, no intersection
            if (center_distance > radius) {
                return false;
            }
        }

        return true;
    };

    template <IsFloat TFloat>
    std::array<Plane<TFloat>, 6> makeShadowFrustum(const OBB<TFloat>& obb, const vec3<TFloat>& lightPosition)
    {
        // Declare variables to be used:
        vec3<TFloat> normal_;
        TFloat distance_;
        std::array<Plane<TFloat>, 6> frustumPlanes;

        // Generate OBB Corners:
        std::array<vec3<TFloat>, 8> corners = obb.getCorners();

        // Find center point of OBB
        vec3<TFloat> center = obb.center();

        // For each edge of the OBB
        const std::array<std::array<size_t, 2>, 12> edges = obb.getEdgeIndices();

        // Find the vertex furthest from the light (for far plane)
        size_t furthestIdx = 0;
        TFloat furthestDist = length(corners[0] - lightPosition);
        for (size_t i = 1; i < 8; i++) {
            TFloat dist = length(corners[i] - lightPosition);
            if (dist > furthestDist) {
                furthestDist = dist;
                furthestIdx = i;
            }
        }

        // Create near and far planes
        vec3<TFloat> lightDir = normalize(center - lightPosition);

        // Near plane (facing toward light)
        normal_ = -lightDir;  // Points toward light
        distance_ = dot(normal_, corners[furthestIdx]);
        frustumPlanes[0] = Plane<TFloat>(normal_, distance_);

        // Far plane (facing away from light)
        normal_ = lightDir;  // Points away from light
        distance_ = dot(normal_, lightPosition);
        frustumPlanes[1] = Plane<TFloat>(normal_, distance_);

        // Create side planes
        size_t planeIdx = 2;
        for (size_t i = 0; i < 12; i++) {
            vec3<TFloat> v1 = corners[edges[i][0]];
            vec3<TFloat> v2 = corners[edges[i][1]];

            // Vector from light to edge vertices
            vec3<TFloat> toV1 = normalize(v1 - lightPosition);

            // Edge vector
            vec3<TFloat> edge = v2 - v1;

            // Create plane containing light and edge
            vec3<TFloat> planeNormal = normalize(cross(edge, toV1));

            // Check if this edge is a silhouette edge
            // by checking if all other vertices are on one side of the plane
            bool isSilhouette = true;
            float side = 0;
            bool sideEstablished = false;
            for (size_t j = 0; j < 8; j++) {
                if (j != edges[i][0] && j != edges[i][1]) {
                    float d = dot(planeNormal, corners[j] - v1);

                    if (!sideEstablished) {
                        side = d;
                        sideEstablished = true;
                    }
                    else if (std::signbit(d) != std::signbit(side)) {
                        isSilhouette = false;
                        break;
                    }
                }
            }

            if (isSilhouette && planeIdx < 6) {
                normal_ = planeNormal;
                distance_ = dot(planeNormal, v1);
                frustumPlanes[planeIdx] = Plane<TFloat>(normal_, distance_);
                planeIdx++;
            }
        }


        return frustumPlanes;
    };
};

