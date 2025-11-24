#ifndef VIRAPY_ROTATION_PY
#define VIRAPY_ROTATION_PY

#include <string>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"

#include "vira/constraints.hpp"
#include "vira/rotation.hpp"

namespace py = pybind11;

namespace vira {
    template <IsFloat TFloat>
    void bind_rotation(py::module& m, const std::string& suffix) {
        py::class_<Rotation<TFloat>>(m, ("Rotation_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<mat3<TFloat>>(), "Construct from rotation matrix",
                py::arg("matrix"))
            .def(py::init<vec3<TFloat>, vec3<TFloat>, vec3<TFloat>>(),
                "Construct from x, y, z axes",
                py::arg("xAxis"), py::arg("yAxis"), py::arg("zAxis"))

            // Operators
            .def(py::self == py::self, "Equality comparison")
            .def(py::self * py::self, "Multiply two rotations",
                py::arg("rhs"))
            .def("__mul__",
                py::overload_cast<const vec3<TFloat>&>(&Rotation<TFloat>::operator*, py::const_),
                "Rotate a vector",
                py::arg("rhs"))

            // Instance methods
            .def(("inverse_" + suffix).c_str(),
                &Rotation<TFloat>::inverse,
                "Get inverse rotation")
            .def(("inverseMultiply_" + suffix).c_str(),
                py::overload_cast<const Rotation<TFloat>&>(&Rotation<TFloat>::inverseMultiply, py::const_),
                "Inverse multiply with another rotation",
                py::arg("rhs"))
            .def(("inverseMultiply_" + suffix).c_str(),
                py::overload_cast<const vec3<TFloat>&>(&Rotation<TFloat>::inverseMultiply, py::const_),
                "Inverse multiply with a vector",
                py::arg("rhs"))
            .def(("getQuaternion_" + suffix).c_str(),
                &Rotation<TFloat>::getQuaternion,
                "Get Hamilton quaternion representation")
            .def(("getShusterQuaternion_" + suffix).c_str(),
                &Rotation<TFloat>::getShusterQuaternion,
                "Get Shuster quaternion representation")
            .def(("getMatrix_" + suffix).c_str(),
                &Rotation<TFloat>::getMatrix,
                "Get rotation matrix",
                py::return_value_policy::reference_internal)
            .def(("getInverseMatrix_" + suffix).c_str(),
                &Rotation<TFloat>::getInverseMatrix,
                "Get inverse rotation matrix",
                py::return_value_policy::reference_internal)

            // Static factory methods
            .def_static(("rotationX_" + suffix).c_str(),
                &Rotation<TFloat>::rotationX,
                "Create rotation around X-axis",
                py::arg("angle"))
            .def_static(("rotationY_" + suffix).c_str(),
                &Rotation<TFloat>::rotationY,
                "Create rotation around Y-axis",
                py::arg("angle"))
            .def_static(("rotationZ_" + suffix).c_str(),
                &Rotation<TFloat>::rotationZ,
                "Create rotation around Z-axis",
                py::arg("angle"))
            .def_static(("eulerAngles_" + suffix).c_str(),
                &Rotation<TFloat>::eulerAngles,
                "Create rotation from Euler angles",
                py::arg("angle1"), py::arg("angle2"), py::arg("angle3"),
                py::arg("sequence") = "123")
            .def_static(("axisAngle_" + suffix).c_str(),
                &Rotation<TFloat>::axisAngle,
                "Create rotation from axis-angle representation",
                py::arg("axis"), py::arg("angle"))
            .def_static(("quaternion_" + suffix).c_str(),
                &Rotation<TFloat>::quaternion,
                "Create rotation from Hamilton quaternion",
                py::arg("hamiltonQuaternion"))
            .def_static(("shusterQuaternion_" + suffix).c_str(),
                &Rotation<TFloat>::shusterQuaternion,
                "Create rotation from Shuster quaternion",
                py::arg("shusterQuaternion"))

            // Static utility methods
            .def_static(("shusterToHamiltonian_" + suffix).c_str(),
                &Rotation<TFloat>::shusterToHamiltonian,
                "Convert Shuster quaternion to Hamilton quaternion",
                py::arg("shusterQuaternion"))
            .def_static(("hamiltonianToShuster_" + suffix).c_str(),
                &Rotation<TFloat>::hamiltonianToShuster,
                "Convert Hamilton quaternion to Shuster quaternion",
                py::arg("hamiltonQuaternion"));
    }
}

#endif