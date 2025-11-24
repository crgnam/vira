#include <cstddef>
#include <stdexcept>
#include <ostream>
#include <iomanip>

#include "vira/constraints.hpp"

namespace vira::images {
    template <IsNumeric T>
    Resolution::Resolution(T xy)
    {
        if (xy < 0) {
            throw std::runtime_error("Resolution must be positive!");
        }

        this->x = static_cast<int>(xy);
        this->y = static_cast<int>(xy);
    };

    template <IsNumeric T>
    Resolution::Resolution(T newX, T newY)
    {
        if (newX < 0 || newY < 0) {
            throw std::runtime_error("Resolution must be positive!");
        }

        this->x = static_cast<int>(newX);
        this->y = static_cast<int>(newY);
    };

    template <IsNumeric T>
    Resolution Resolution::operator* (T rhs)
    {
        float rhs_f = static_cast<float>(rhs);
        int newX = static_cast<int>(rhs_f * static_cast<float>(this->x));
        int newY = static_cast<int>(rhs_f * static_cast<float>(this->y));
        return Resolution{ newX, newY };
    };

    template <IsNumeric T>
    Resolution Resolution::operator*= (T rhs)
    {
        float rhs_f = static_cast<float>(rhs);
        this->x = static_cast<int>(rhs_f * static_cast<float>(this->x));
        this->y = static_cast<int>(rhs_f * static_cast<float>(this->y));
        return *this;
    };

    template <IsNumeric T>
    Resolution Resolution::operator/ (T rhs)
    {
        float rhs_f = static_cast<float>(rhs);
        int newX = static_cast<int>(rhs_f / static_cast<float>(this->x));
        int newY = static_cast<int>(rhs_f / static_cast<float>(this->y));
        return Resolution{ newX, newY };
    };

    template <IsNumeric T>
    Resolution Resolution::operator/= (T rhs)
    {
        float rhs_f = static_cast<float>(rhs);
        this->x = static_cast<int>(rhs_f / static_cast<float>(this->x));
        this->y = static_cast<int>(rhs_f / static_cast<float>(this->y));
        return *this;
    };

    template <IsNumeric T>
    Resolution operator* (T lhs, const Resolution& rhs)
    {
        float rhs_f = static_cast<float>(lhs);
        int newX = static_cast<int>(rhs_f * static_cast<float>(rhs.x));
        int newY = static_cast<int>(rhs_f * static_cast<float>(rhs.y));
        return Resolution{ newX, newY };
    };

    template <IsNumeric T>
    Resolution operator/ (T lhs, const Resolution& rhs)
    {
        float rhs_f = static_cast<float>(lhs);
        int newX = static_cast<int>(rhs_f / static_cast<float>(rhs.x));
        int newY = static_cast<int>(rhs_f / static_cast<float>(rhs.y));
        return Resolution{ newX, newY };
    };

    bool Resolution::operator==(const Resolution& rhs) const
    {
        return (this->x == rhs.x && this->y == rhs.y);
    };

    bool Resolution::operator!=(const Resolution& rhs) const
    {
        return (this->x != rhs.x && this->y != rhs.y);
    };

    std::ostream& operator<<(std::ostream& os, Resolution resolution)
    {
        // Save original stream state
        std::ios oldState(nullptr);
        oldState.copyfmt(os);

        os << "Resolution: [" << resolution.x << " x " << resolution.y << "]\n";

        os.copyfmt(oldState);
        return os;
    };
}