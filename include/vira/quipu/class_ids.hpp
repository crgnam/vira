#ifndef VIRA_QUIPU_CLASS_IDS_HPP
#define VIRA_QUIPU_CLASS_IDS_HPP

#include <cstdint>

namespace vira::quipu {

    enum ViraClassID : uint16_t {

        VIRA_CLASS_ID = 0,

        // Codes For Standard Types (1-1000):
        VIRA_FLOAT = 1,
        VIRA_DOUBLE = 2,

        VIRA_UINT8 = 3,
        VIRA_UINT16 = 4,
        VIRA_UINT32 = 5,
        VIRA_UINT64 = 6,

        VIRA_INT8 = 7,
        VIRA_INT16 = 8,
        VIRA_INT32 = 9,
        VIRA_INT64 = 10,


        // Codes for Vec and Math Types (1001 - 1999):
        VIRA_VEC1_F = 1001,
        VIRA_VEC2_F = 1002,
        VIRA_VEC3_F = 1003,
        VIRA_VEC4_F = 1004,

        VIRA_VEC1_D = 1005,
        VIRA_VEC2_D = 1006,
        VIRA_VEC3_D = 1007,
        VIRA_VEC4_D = 1008,

        VIRA_MAT_2x2_F = 1009,
        VIRA_MAT_2x2_D = 1010,

        VIRA_MAT3_3x3_F = 1011,
        VIRA_MAT3_3x3_D = 1012,

        VIRA_MAT4_4x4_F = 1013,
        VIRA_MAT4_4x4_D = 1014,


        // Codes for Vira Standard Types (2000-2999):
        VIRA_TRANSFORM_STATE = 2000,
        VIRA_DEM = 2001,
        VIRA_DEM_PYRAMID = 2002,


        // Error Codes (>65000)
        VIRA_UNDEFINED = 65535
    };

    template <typename T>
    inline ViraClassID getClassID();
};

#include "implementation/quipu/class_ids.ipp"

#endif
