#ifndef VIRA_RENDERING_ACCELERATION_EMBREE_OPTIONS_HPP
#define VIRA_RENDERING_ACCELERATION_EMBREE_OPTIONS_HPP

#include <cstddef>
#include <concepts>

// TODO REMOVE THIRD PARTY HEADERS:
#include "embree3/rtcore.h"

namespace vira::rendering {
    template<size_t N>
    concept ValidPacketSize = (N == 4 || N == 8 || N == 16);

    enum BuildQuality {
        BUILD_QUALITY_LOW,
        BUILD_QUALITY_MEDIUM,
        BUILD_QUALITY_HIGH
    };

    struct EmbreeOptions {
        BuildQuality build_quality = BUILD_QUALITY_HIGH;
        bool robust_build = true;
    };


    // Helper templates:
    template <size_t N> requires ValidPacketSize<N>
    struct RTCRayHitSelector;

    template<>
    struct RTCRayHitSelector<4> {
        using RayHitType = RTCRayHit4;
        static void intersect(int* valid, RTCScene scene, RTCIntersectContext* context, RayHitType* rayhit) {
            rtcIntersect4(valid, scene, context, rayhit);
        }
    };

    template<>
    struct RTCRayHitSelector<8> {
        using RayHitType = RTCRayHit8;
        static void intersect(int* valid, RTCScene scene, RTCIntersectContext* context, RayHitType* rayhit) {
            rtcIntersect8(valid, scene, context, rayhit);
        }
    };

    template<>
    struct RTCRayHitSelector<16> {
        using RayHitType = RTCRayHit16;
        static void intersect(int* valid, RTCScene scene, RTCIntersectContext* context, RayHitType* rayhit) {
            rtcIntersect16(valid, scene, context, rayhit);
        }
    };
}

#endif