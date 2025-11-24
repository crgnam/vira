#include <cmath>
#include <string>
#include <stdexcept>
#include <iostream>

#include "glm/glm.hpp"

#include "vira/vec.hpp"
#include "vira/math.hpp"
#include "vira/constraints.hpp"
#include "vira/units/units.hpp"

namespace vira {
    template <IsFloat TFloat>
    Rotation<TFloat>::Rotation(mat3<TFloat> matrix)
    {
        TFloat det = glm::determinant(matrix);
        static TFloat tol = static_cast<TFloat>(1e-3);
        if (glm::abs(det - 1) > tol) {
            throw std::runtime_error("The provided matrix is not a valid rotation matrix.  Determinant = " + std::to_string(det));
        }

        this->matrix_ = matrix;
        this->transpose_ = glm::transpose(matrix);
    };

    template <IsFloat TFloat>
    Rotation<TFloat>::Rotation(vec3<TFloat> x_axis, vec3<TFloat> y_axis, vec3<TFloat> z_axis)
    {
        TFloat tol = static_cast<TFloat>(1e-3);
        if (length(x_axis) <= tol || length(y_axis) <= tol || length(z_axis) <= tol) {
            throw std::runtime_error("Provided axes have a length near (or equal to) zero");
        }

        x_axis = normalize(x_axis);
        y_axis = normalize(y_axis);
        z_axis = normalize(z_axis);

        mat3<TFloat> matrix;
        matrix[0] = x_axis;
        matrix[1] = y_axis;
        matrix[2] = z_axis;

        TFloat det = glm::determinant(matrix);
        if (glm::abs(det - 1) > tol) {
            throw std::runtime_error("The resulting matrix is not a valid rotation matrix.  Determinant = " + std::to_string(det));
        }

        this->matrix_ = matrix;
        this->transpose_ = glm::transpose(matrix);
    };

    template <IsFloat TFloat>
    bool Rotation<TFloat>::operator==(const Rotation<TFloat>& rhs) const
    {
        bool isEqual = true;
        isEqual = isEqual && (matrix_[0][0] == rhs.matrix_[0][0]);
        isEqual = isEqual && (matrix_[0][1] == rhs.matrix_[0][1]);
        isEqual = isEqual && (matrix_[0][2] == rhs.matrix_[0][2]);

        isEqual = isEqual && (matrix_[1][0] == rhs.matrix_[1][0]);
        isEqual = isEqual && (matrix_[1][1] == rhs.matrix_[1][1]);
        isEqual = isEqual && (matrix_[1][2] == rhs.matrix_[1][2]);

        isEqual = isEqual && (matrix_[2][0] == rhs.matrix_[2][0]);
        isEqual = isEqual && (matrix_[2][1] == rhs.matrix_[2][1]);
        isEqual = isEqual && (matrix_[2][2] == rhs.matrix_[2][2]);
        return isEqual;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::operator*(const Rotation<TFloat>& rhs) const
    {
        mat3<TFloat> newmatrix_ = this->matrix_ * rhs.matrix_;
        return Rotation<TFloat>(newmatrix_);
    };

    template <IsFloat TFloat>
    vec3<TFloat> Rotation<TFloat>::operator* (const vec3<TFloat>& rhs) const
    {
        return this->matrix_ * rhs;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::inverse() const
    {
        return Rotation<TFloat>(this->getInverseMatrix());
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::inverseMultiply(const Rotation<TFloat>& rhs) const
    {
        mat3<TFloat> newmatrix_ = this->transpose_ * rhs.matrix_;
        return Rotation<TFloat>(newmatrix_);
    };

    template <IsFloat TFloat>
    vec3<TFloat> Rotation<TFloat>::inverseMultiply(const vec3<TFloat>& rhs) const
    {
        return this->transpose_ * rhs;
    };

    template <IsFloat TFloat>
    vec4<TFloat> Rotation<TFloat>::getQuaternion() const
    {
        // Hamilton quaternion (w, x, y, z) using Shepperd's method
        // This is numerically stable for all rotation matrices

        vec4<TFloat> q;
        TFloat trace = matrix_[0][0] + matrix_[1][1] + matrix_[2][2];

        if (trace > 0) {
            // Case 1: trace > 0 (most common case)
            TFloat s = glm::sqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
            q[0] = 0.25f * s;                          // w
            q[1] = (matrix_[2][1] - matrix_[1][2]) / s; // x
            q[2] = (matrix_[0][2] - matrix_[2][0]) / s; // y
            q[3] = (matrix_[1][0] - matrix_[0][1]) / s; // z
        }
        else if (matrix_[0][0] > matrix_[1][1] && matrix_[0][0] > matrix_[2][2]) {
            // Case 2: M[0][0] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[0][0] - matrix_[1][1] - matrix_[2][2]) * 2.0f; // s = 4 * qx
            q[0] = (matrix_[2][1] - matrix_[1][2]) / s; // w
            q[1] = 0.25f * s;                          // x
            q[2] = (matrix_[0][1] + matrix_[1][0]) / s; // y
            q[3] = (matrix_[0][2] + matrix_[2][0]) / s; // z
        }
        else if (matrix_[1][1] > matrix_[2][2]) {
            // Case 3: M[1][1] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[1][1] - matrix_[0][0] - matrix_[2][2]) * 2.0f; // s = 4 * qy
            q[0] = (matrix_[0][2] - matrix_[2][0]) / s; // w
            q[1] = (matrix_[0][1] + matrix_[1][0]) / s; // x
            q[2] = 0.25f * s;                          // y
            q[3] = (matrix_[1][2] + matrix_[2][1]) / s; // z
        }
        else {
            // Case 4: M[2][2] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[2][2] - matrix_[0][0] - matrix_[1][1]) * 2.0f; // s = 4 * qz
            q[0] = (matrix_[1][0] - matrix_[0][1]) / s; // w
            q[1] = (matrix_[0][2] + matrix_[2][0]) / s; // x
            q[2] = (matrix_[1][2] + matrix_[2][1]) / s; // y
            q[3] = 0.25f * s;                          // z
        }

        return q;
    }

    template <IsFloat TFloat>
    vec4<TFloat> Rotation<TFloat>::getShusterQuaternion() const
    {
        // Shuster quaternion (x, y, z, w) using Shepperd's method

        vec4<TFloat> q;
        TFloat trace = matrix_[0][0] + matrix_[1][1] + matrix_[2][2];

        if (trace > 0) {
            // Case 1: trace > 0 (most common case)
            TFloat s = glm::sqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
            q[3] = 0.25f * s;                          // w
            q[0] = (matrix_[2][1] - matrix_[1][2]) / s; // x
            q[1] = (matrix_[0][2] - matrix_[2][0]) / s; // y
            q[2] = (matrix_[1][0] - matrix_[0][1]) / s; // z
        }
        else if (matrix_[0][0] > matrix_[1][1] && matrix_[0][0] > matrix_[2][2]) {
            // Case 2: M[0][0] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[0][0] - matrix_[1][1] - matrix_[2][2]) * 2.0f; // s = 4 * qx
            q[3] = (matrix_[2][1] - matrix_[1][2]) / s; // w
            q[0] = 0.25f * s;                          // x
            q[1] = (matrix_[0][1] + matrix_[1][0]) / s; // y
            q[2] = (matrix_[0][2] + matrix_[2][0]) / s; // z
        }
        else if (matrix_[1][1] > matrix_[2][2]) {
            // Case 3: M[1][1] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[1][1] - matrix_[0][0] - matrix_[2][2]) * 2.0f; // s = 4 * qy
            q[3] = (matrix_[0][2] - matrix_[2][0]) / s; // w
            q[0] = (matrix_[0][1] + matrix_[1][0]) / s; // x
            q[1] = 0.25f * s;                          // y
            q[2] = (matrix_[1][2] + matrix_[2][1]) / s; // z
        }
        else {
            // Case 4: M[2][2] is largest diagonal element
            TFloat s = glm::sqrt(1.0f + matrix_[2][2] - matrix_[0][0] - matrix_[1][1]) * 2.0f; // s = 4 * qz
            q[3] = (matrix_[1][0] - matrix_[0][1]) / s; // w
            q[0] = (matrix_[0][2] + matrix_[2][0]) / s; // x
            q[1] = (matrix_[1][2] + matrix_[2][1]) / s; // y
            q[2] = 0.25f * s;                          // z
        }

        return q;
    }

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::rotationX(units::Radian angle)
    {
        TFloat angle_rad = static_cast<TFloat>(angle.getValue());
        TFloat c = std::cos(angle_rad);
        TFloat s = std::sin(angle_rad);

        mat3<TFloat> matrix{ 1 };
        matrix[0][0] = 1; matrix[1][0] = 0; matrix[2][0] =  0;
        matrix[0][1] = 0; matrix[1][1] = c; matrix[2][1] = -s;
        matrix[0][2] = 0; matrix[1][2] = s; matrix[2][2] =  c;

        Rotation<TFloat> rotation(matrix);

        return rotation;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::rotationY(units::Radian angle)
    {
        TFloat angle_rad = static_cast<TFloat>(angle.getValue());
        TFloat c = std::cos(angle_rad);
        TFloat s = std::sin(angle_rad);

        mat3<TFloat> matrix{ 1 };
        matrix[0][0] =  c; matrix[1][0] = 0; matrix[2][0] = s;
        matrix[0][1] =  0; matrix[1][1] = 1; matrix[2][1] = 0;
        matrix[0][2] = -s; matrix[1][2] = 0; matrix[2][2] = c;

        Rotation<TFloat> rotation(matrix);

        return rotation;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::rotationZ(units::Radian angle)
    {
        TFloat angle_rad = static_cast<TFloat>(angle.getValue());
        TFloat c = std::cos(angle_rad);
        TFloat s = std::sin(angle_rad);

        mat3<TFloat> matrix{ 1 };
        matrix[0][0] = c; matrix[1][0] = -s; matrix[2][0] = 0;
        matrix[0][1] = s; matrix[1][1] =  c; matrix[2][1] = 0;
        matrix[0][2] = 0; matrix[1][2] =  0; matrix[2][2] = 1;

        Rotation<TFloat> rotation(matrix);

        return rotation;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::eulerAngles(units::Degree xAngle, units::Degree yAngle, units::Degree zAngle, const std::string& sequence)
    {
        Rotation<TFloat> rotX = rotationX(xAngle);
        Rotation<TFloat> rotY = rotationY(yAngle);
        Rotation<TFloat> rotZ = rotationZ(zAngle);

        if (sequence.length() != 3) {
            throw std::runtime_error("Euler sequnce must be 3 elements long");
        }

        Rotation<TFloat> rotation;
        for (uint8_t i = 0; i < sequence.length(); i++) {
            if (sequence[i] == '1' || sequence[i] == 'X' || sequence[i] == 'x') {
                rotation = rotX * rotation;
            }
            else if (sequence[i] == '2' || sequence[i] == 'Y' || sequence[i] == 'y') {
                rotation = rotY * rotation;
            }
            else if (sequence[i] == '3' || sequence[i] == 'Z' || sequence[i] == 'z') {
                rotation = rotZ * rotation;
            }
            else {
                throw std::runtime_error("Euler sequence definition must contain values of only 1,2,3 or X,Y,Z");
            }
        }

        return rotation;
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::axisAngle(vec3<TFloat> axis, units::Degree angle)
    {
        TFloat angleVal = static_cast<TFloat>(units::Radian(angle).getValue());

        TFloat sinAngle = std::sin(angleVal / 2);
        TFloat cosAngle = std::cos(angleVal / 2);

        vec4<TFloat> quat{
            axis[0] * sinAngle,
            axis[1] * sinAngle,
            axis[2] * sinAngle,
            cosAngle
        };

        return Rotation<TFloat>::quaternion(quat);
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::quaternion(vec4<TFloat> hamiltonQuaternion)
    {
        // This assumes hamilton quaternion: (w, x,y,z)
        vec4<TFloat> q = glm::normalize(hamiltonQuaternion);

        // Extract components:
        TFloat qr = q[0];
        TFloat qi = q[1];
        TFloat qj = q[2];
        TFloat qk = q[3];

        // Precompute squares:
        TFloat qi2 = qi * qi;
        TFloat qj2 = qj * qj;
        TFloat qk2 = qk * qk;

        // Recall glm matrices are column-major:
        mat3<TFloat> matrix;
        matrix[0][0] = 1 - 2 * (qj2 + qk2);
        matrix[1][0] = 2 * (qi * qj - qk * qr);
        matrix[2][0] = 2 * (qi * qk + qj * qr);

        matrix[0][1] = 2 * (qi * qj + qk * qr);
        matrix[1][1] = 1 - 2 * (qi2 + qk2);
        matrix[2][1] = 2 * (qj * qk - qi * qr);

        matrix[0][2] = 2 * (qi * qk - qj * qr);
        matrix[1][2] = 2 * (qj * qk + qi * qr);
        matrix[2][2] = 1 - 2 * (qi2 + qj2);

        return Rotation<TFloat>(matrix);
    };

    template <IsFloat TFloat>
    Rotation<TFloat> Rotation<TFloat>::shusterQuaternion(vec4<TFloat> shusterQuaternion)
    {
        return Rotation<TFloat>::quaternion(Rotation<TFloat>::shusterToHamiltonian(shusterQuaternion));
    };

    template <IsFloat TFloat>
    vec4<TFloat> Rotation<TFloat>::shusterToHamiltonian(vec4<TFloat> q)
    {
        return vec4<TFloat>{ q[3], q[0], q[1], q[2] };
    };

    template <IsFloat TFloat>
    vec4<TFloat> Rotation<TFloat>::hamiltonianToShuster(vec4<TFloat> q)
    {
        return vec4<TFloat>{ q[1], q[2], q[3], q[0] };
    };
};