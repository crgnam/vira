#ifndef VIRA_IMAGES_RESOLUTION_HPP
#define VIRA_IMAGES_RESOLUTION_HPP

#include <cstddef>
#include <ostream>

#include "vira/constraints.hpp"

namespace vira::images {
    struct Resolution {
        Resolution() = default;

        template <IsNumeric T>
        Resolution(T xy);

        template <IsNumeric T>
        Resolution(T newX, T newY);

        int x = 0;
        int y = 0;

        inline bool operator==(const Resolution& rhs) const;
        inline bool operator!= (const Resolution& rhs) const;

        template <IsNumeric T>
        Resolution operator* (T rhs);

        template <IsNumeric T>
        Resolution operator*= (T rhs);

        template <IsNumeric T>
        Resolution operator/ (T rhs);

        template <IsNumeric T>
        Resolution operator/= (T rhs);


    };

    template <IsNumeric T>
    Resolution operator* (T lhs, const Resolution& rhs);

    template <IsNumeric T>
    Resolution operator/ (T lhs, const Resolution& rhs);

    inline std::ostream& operator<<(std::ostream& os, Resolution resolution);

};

#include "implementation/images/resolution.ipp"

#endif