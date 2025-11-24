#ifndef VIRA_ROTATIONS_HPP
#define VIRA_ROTATIONS_HPP

#include <string>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/units/units.hpp"

namespace vira {
    template <IsFloat TFloat>
    class Rotation {
    public:
        /**
         * @brief Construct a new Rotation
         *
         */
        Rotation() = default;

        /**
         * @brief Construct a new Rotation given a matrix
         *
         * @param matrix The rotation matrix
         */
        Rotation(mat3<TFloat> matrix);

        Rotation(vec3<TFloat> xAxis, vec3<TFloat> yAxis, vec3<TFloat> zAxis);

        template <IsFloat TFloat2>
        operator Rotation<TFloat2>() {
            return Rotation<TFloat2>(this->matrix_);
        }

        bool operator== (const Rotation& rhs) const;

        /**
         * @brief Operator overload for multiplying two rotations together
         *
         * @param rhs The rotation being operated on
         * @return Rotation The resulting rotation
         */
        Rotation operator* (const Rotation& rhs) const;

        /**
         * @brief Operator overload for rotating a vector
         *
         * @param rhs The vector being operated on
         * @return vec3<TFloat> The resulting vector
         */
        vec3<TFloat> operator* (const vec3<TFloat>& rhs) const;

        Rotation<TFloat> inverse() const;
        Rotation inverseMultiply(const Rotation& rhs) const;
        vec3<TFloat> inverseMultiply(const vec3<TFloat>& rhs) const;

        /**
         * @brief Get the rotation's corresponding quaternion
         *
         * @return vec4<TFloat> The corresponding quaternion
         */
        vec4<TFloat> getQuaternion() const;
        vec4<TFloat> getShusterQuaternion() const;

        /**
         * @brief Get the rotation's corresponding matrix
         *
         * @return const mat3<TFloat>& The corresponding rotation matrix
         */
        const mat3<TFloat>& getMatrix() const { return this->matrix_; }

        /**
         * @brief Get the rotation's corresponding inverse matrix
         *
         * @return const mat3<TFloat>& The corresponding inverse rotation matrix
         */
        const mat3<TFloat>& getInverseMatrix() const { return this->transpose_; }

        /**
         * @brief Static method for creating a rotation around the x-axis
         *
         * @param angle The angle around the X-axis
         * @return Rotation The basis rotation about the X-axis
         */
        static Rotation rotationX(units::Radian angle);

        /**
         * @brief Static method for creating a roation around the y-axis
         *
         * @param angle The angle around the Y-axis
         * @return Rotation The basis rotation about the Y-axis
         */
        static Rotation rotationY(units::Radian angle);

        /**
         * @brief Static method for creating a rotation around the z-axis
         *
         * @param angle The angle around the Z-axis
         * @return Rotation The basis rotation about the Z-axis
         */
        static Rotation rotationZ(units::Radian angle);

        /**
         * @brief Staic method for creating a Rotation based on euler angles
         *
         * @param angle1 The angle around the first axis
         * @param angle2 The angle around the second axis
         * @param angle3 The angle around the third axis
         * @param sequence The sequence of basis vectors (defaults to 321)
         * @return Rotation The corresponding rotation
         */
        static Rotation eulerAngles(units::Degree angle1, units::Degree angle2, units::Degree angle3, const std::string& sequence = "123");

        static Rotation axisAngle(vec3<TFloat> axis, units::Degree angle);

        static Rotation quaternion(vec4<TFloat> hamiltonQuaternion);

        static Rotation shusterQuaternion(vec4<TFloat> shusterQuaternion);

        static vec4<TFloat> shusterToHamiltonian(vec4<TFloat> shusterQuaternion);

        static vec4<TFloat> hamiltonianToShuster(vec4<TFloat> hamiltonQuaternion);

    private:
        mat3<TFloat> matrix_{ 1 };

        mat3<TFloat> transpose_{ 1 };
    };
};

#include "implementation/rotation.ipp"

#endif