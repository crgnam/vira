#ifndef VIRAPY_VEC_PY
#define VIRAPY_VEC_PY

#include <string>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"

#include "vira/constraints.hpp"
#include "vira/vec.hpp"

namespace py = pybind11;

namespace vira {
    template <IsFloat TFloat>
    void bind_vec(py::module& m, const std::string& suffix) {
        // Bind vec2
        py::class_<vec2<TFloat>>(m, ("vec2_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with single value", py::arg("value"))
            .def(py::init<TFloat, TFloat>(), "Constructor with x, y components",
                py::arg("x"), py::arg("y"))
            .def_readwrite("x", &vec2<TFloat>::x, "X component")
            .def_readwrite("y", &vec2<TFloat>::y, "Y component")
            .def("__getitem__", [](const vec2<TFloat>& v, int i) {
            if (i >= 2) throw py::index_error();
            return v[i];
                })
            .def("__setitem__", [](vec2<TFloat>& v, int i, TFloat value) {
            if (i >= 2) throw py::index_error();
            v[i] = value;
                })
            .def(py::self + py::self, "Vector addition")
            .def(py::self - py::self, "Vector subtraction")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def(TFloat{} *py::self, "Scalar multiplication")
            .def(py::self / TFloat{}, "Scalar division")
            .def(py::self == py::self, "Equality comparison")
            .def(py::self != py::self, "Inequality comparison")
            .def("__repr__", [](const vec2<TFloat>& v) {
            return "vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
                });

        // Bind vec3
        py::class_<vec3<TFloat>>(m, ("vec3_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with single value", py::arg("value"))
            .def(py::init<TFloat, TFloat, TFloat>(), "Constructor with x, y, z components",
                py::arg("x"), py::arg("y"), py::arg("z"))
            .def_readwrite("x", &vec3<TFloat>::x, "X component")
            .def_readwrite("y", &vec3<TFloat>::y, "Y component")
            .def_readwrite("z", &vec3<TFloat>::z, "Z component")
            .def("__getitem__", [](const vec3<TFloat>& v, int i) {
            if (i >= 3) throw py::index_error();
            return v[i];
                })
            .def("__setitem__", [](vec3<TFloat>& v, int i, TFloat value) {
            if (i >= 3) throw py::index_error();
            v[i] = value;
                })
            .def(py::self + py::self, "Vector addition")
            .def(py::self - py::self, "Vector subtraction")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def(TFloat{} *py::self, "Scalar multiplication")
            .def(py::self / TFloat{}, "Scalar division")
            .def(py::self == py::self, "Equality comparison")
            .def(py::self != py::self, "Inequality comparison")
            .def("__repr__", [](const vec3<TFloat>& v) {
            return "vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
                });

        // Bind vec4
        py::class_<vec4<TFloat>>(m, ("vec4_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with single value", py::arg("value"))
            .def(py::init<TFloat, TFloat, TFloat, TFloat>(), "Constructor with x, y, z, w components",
                py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
            .def_readwrite("x", &vec4<TFloat>::x, "X component")
            .def_readwrite("y", &vec4<TFloat>::y, "Y component")
            .def_readwrite("z", &vec4<TFloat>::z, "Z component")
            .def_readwrite("w", &vec4<TFloat>::w, "W component")
            .def("__getitem__", [](const vec4<TFloat>& v, int i) {
            if (i >= 4) throw py::index_error();
            return v[i];
                })
            .def("__setitem__", [](vec4<TFloat>& v, int i, TFloat value) {
            if (i >= 4) throw py::index_error();
            v[i] = value;
                })
            .def(py::self + py::self, "Vector addition")
            .def(py::self - py::self, "Vector subtraction")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def(TFloat{} *py::self, "Scalar multiplication")
            .def(py::self / TFloat{}, "Scalar division")
            .def(py::self == py::self, "Equality comparison")
            .def(py::self != py::self, "Inequality comparison")
            .def("__repr__", [](const vec4<TFloat>& v) {
            return "vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
                std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
                });

        // Bind mat2
        py::class_<mat2<TFloat>>(m, ("mat2_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with diagonal value", py::arg("diagonal"))
            .def("__getitem__", [](const mat2<TFloat>& mat, py::tuple idx) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 2 || row >= 2) throw py::index_error();
            return mat[col][row];
                })
            .def("__setitem__", [](mat2<TFloat>& mat, py::tuple idx, TFloat value) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 2 || row >= 2) throw py::index_error();
            mat[col][row] = value;
                })
            .def(py::self + py::self, "Matrix addition")
            .def(py::self - py::self, "Matrix subtraction")
            .def(py::self * py::self, "Matrix multiplication")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def(py::self == py::self, "Equality comparison")
            .def("__repr__", [](const mat2<TFloat>&) {
            return "mat2";
                });

        // Bind mat3
        py::class_<mat3<TFloat>>(m, ("mat3_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with diagonal value", py::arg("diagonal"))
            .def("__getitem__", [](const mat3<TFloat>& mat, py::tuple idx) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 3 || row >= 3) throw py::index_error();
            return mat[col][row];
                })
            .def("__setitem__", [](mat3<TFloat>& mat, py::tuple idx, TFloat value) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 3 || row >= 3) throw py::index_error();
            mat[col][row] = value;
                })
            .def(py::self + py::self, "Matrix addition")
            .def(py::self - py::self, "Matrix subtraction")
            .def(py::self * py::self, "Matrix multiplication")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def("__mul__", [](const mat3<TFloat>& mat, const vec3<TFloat>& v) {
            return mat * v;
                }, "Matrix-vector multiplication")
            .def(py::self == py::self, "Equality comparison")
            .def("__repr__", [](const mat3<TFloat>&) {
            return "mat3";
                });

        // Bind mat4
        py::class_<mat4<TFloat>>(m, ("mat4_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")
            .def(py::init<TFloat>(), "Constructor with diagonal value", py::arg("diagonal"))
            .def("__getitem__", [](const mat4<TFloat>& mat, py::tuple idx) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 4 || row >= 4) throw py::index_error();
            return mat[col][row];
                })
            .def("__setitem__", [](mat4<TFloat>& mat, py::tuple idx, TFloat value) {
            if (py::len(idx) != 2) throw py::index_error();
            int col = idx[0].cast<int>();
            int row = idx[1].cast<int>();
            if (col >= 4 || row >= 4) throw py::index_error();
            mat[col][row] = value;
                })
            .def(py::self + py::self, "Matrix addition")
            .def(py::self - py::self, "Matrix subtraction")
            .def(py::self * py::self, "Matrix multiplication")
            .def(py::self * TFloat{}, "Scalar multiplication")
            .def("__mul__", [](const mat4<TFloat>& mat, const vec4<TFloat>& v) {
            return mat * v;
                }, "Matrix-vector multiplication")
            .def(py::self == py::self, "Equality comparison")
            .def("__repr__", [](const mat4<TFloat>&) {
            return "mat4";
                });

        // Vector operations
        m.def(("normalize_" + suffix).c_str(),
            py::overload_cast<const vec2<TFloat>&>(&normalize<vec2<TFloat>>),
            "Normalize vec2", py::arg("vec"));
        m.def(("normalize_" + suffix).c_str(),
            py::overload_cast<const vec3<TFloat>&>(&normalize<vec3<TFloat>>),
            "Normalize vec3", py::arg("vec"));
        m.def(("normalize_" + suffix).c_str(),
            py::overload_cast<const vec4<TFloat>&>(&normalize<vec4<TFloat>>),
            "Normalize vec4", py::arg("vec"));

        m.def(("dot_" + suffix).c_str(),
            py::overload_cast<const vec2<TFloat>&, const vec2<TFloat>&>(&dot<vec2<TFloat>>),
            "Dot product of vec2", py::arg("vec1"), py::arg("vec2"));
        m.def(("dot_" + suffix).c_str(),
            py::overload_cast<const vec3<TFloat>&, const vec3<TFloat>&>(&dot<vec3<TFloat>>),
            "Dot product of vec3", py::arg("vec1"), py::arg("vec2"));
        m.def(("dot_" + suffix).c_str(),
            py::overload_cast<const vec4<TFloat>&, const vec4<TFloat>&>(&dot<vec4<TFloat>>),
            "Dot product of vec4", py::arg("vec1"), py::arg("vec2"));

        m.def(("length_" + suffix).c_str(),
            py::overload_cast<const vec2<TFloat>&>(&length<vec2<TFloat>>),
            "Length of vec2", py::arg("vec"));
        m.def(("length_" + suffix).c_str(),
            py::overload_cast<const vec3<TFloat>&>(&length<vec3<TFloat>>),
            "Length of vec3", py::arg("vec"));
        m.def(("length_" + suffix).c_str(),
            py::overload_cast<const vec4<TFloat>&>(&length<vec4<TFloat>>),
            "Length of vec4", py::arg("vec"));

        m.def(("cross_" + suffix).c_str(),
            &cross<vec3<TFloat>>,
            "Cross product of vec3", py::arg("vec1"), py::arg("vec2"));

        m.def(("abs_" + suffix).c_str(),
            py::overload_cast<const vec2<TFloat>&>(&abs<vec2<TFloat>>),
            "Absolute value of vec2", py::arg("vec"));
        m.def(("abs_" + suffix).c_str(),
            py::overload_cast<const vec3<TFloat>&>(&abs<vec3<TFloat>>),
            "Absolute value of vec3", py::arg("vec"));
        m.def(("abs_" + suffix).c_str(),
            py::overload_cast<const vec4<TFloat>&>(&abs<vec4<TFloat>>),
            "Absolute value of vec4", py::arg("vec"));

        // Matrix operations
        m.def(("transpose_" + suffix).c_str(),
            py::overload_cast<const mat2<TFloat>&>(&transpose<mat2<TFloat>>),
            "Transpose mat2", py::arg("mat"));
        m.def(("transpose_" + suffix).c_str(),
            py::overload_cast<const mat3<TFloat>&>(&transpose<mat3<TFloat>>),
            "Transpose mat3", py::arg("mat"));
        m.def(("transpose_" + suffix).c_str(),
            py::overload_cast<const mat4<TFloat>&>(&transpose<mat4<TFloat>>),
            "Transpose mat4", py::arg("mat"));

        m.def(("inverse_" + suffix).c_str(),
            py::overload_cast<const mat2<TFloat>&>(&inverse<mat2<TFloat>>),
            "Inverse of mat2", py::arg("matrix"));
        m.def(("inverse_" + suffix).c_str(),
            py::overload_cast<const mat3<TFloat>&>(&inverse<mat3<TFloat>>),
            "Inverse of mat3", py::arg("matrix"));
        m.def(("inverse_" + suffix).c_str(),
            py::overload_cast<const mat4<TFloat>&>(&inverse<mat4<TFloat>>),
            "Inverse of mat4", py::arg("matrix"));

        // Transformation operations
        m.def(("transformPoint_" + suffix).c_str(),
            &transformPoint<TFloat>,
            "Transform point using mat4", py::arg("transform"), py::arg("point"));
        m.def(("transformDirection_" + suffix).c_str(),
            &transformDirection<TFloat>,
            "Transform direction using mat4", py::arg("transform"), py::arg("direction"));
        m.def(("transformNormal_" + suffix).c_str(),
            &transformNormal<TFloat>,
            "Transform normal using mat4", py::arg("transform"), py::arg("normal"));

        // Custom comparison functions for vectors (based on length)
        m.def(("vec_less_than_" + suffix).c_str(),
            [](const vec2<TFloat>& v1, const vec2<TFloat>& v2) {
                return length(v1) < length(v2);
            }, "Compare vec2 by length (less than)", py::arg("vec1"), py::arg("vec2"));
        m.def(("vec_less_than_" + suffix).c_str(),
            [](const vec3<TFloat>& v1, const vec3<TFloat>& v2) {
                return length(v1) < length(v2);
            }, "Compare vec3 by length (less than)", py::arg("vec1"), py::arg("vec2"));
        m.def(("vec_less_than_" + suffix).c_str(),
            [](const vec4<TFloat>& v1, const vec4<TFloat>& v2) {
                return length(v1) < length(v2);
            }, "Compare vec4 by length (less than)", py::arg("vec1"), py::arg("vec2"));
    }
}

#endif