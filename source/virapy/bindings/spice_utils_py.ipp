#ifndef VIRAPY_SPICE_UTILS_PY
#define VIRAPY_SPICE_UTILS_PY

#include <string>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"

#include "vira/constraints.hpp"
#include "vira/spice_utils.hpp"

namespace py = pybind11;

namespace vira {
    template <IsFloat TFloat>
    void bind_spice_utils(py::module& m, const std::string& suffix) {
        py::class_<SpiceUtils<TFloat>>(m, ("SpiceUtils_" + suffix).c_str())

            // Kernel loading methods
            .def_static(("furnsh_" + suffix).c_str(),
                &SpiceUtils<TFloat>::furnsh,
                "Load a SPICE kernel from absolute path",
                py::arg("kernelPath"))
            .def_static(("furnsh_relative_to_file_" + suffix).c_str(),
                &SpiceUtils<TFloat>::furnsh_relative_to_file,
                "Load a SPICE kernel from path relative to calling file",
                py::arg("kernelPath"))

            // Name/ID conversion methods
            .def_static(("idToName_" + suffix).c_str(),
                &SpiceUtils<TFloat>::idToName,
                "Convert NAIF ID to name",
                py::arg("id"))
            .def_static(("nameToID_" + suffix).c_str(),
                &SpiceUtils<TFloat>::nameToID,
                "Convert NAIF name to ID",
                py::arg("naif_name"))

            // Time conversion methods
            .def_static(("stringToET_" + suffix).c_str(),
                &SpiceUtils<TFloat>::stringToET,
                "Convert time string to ephemeris time",
                py::arg("timeString"))
            .def_static(("etToString_" + suffix).c_str(),
                &SpiceUtils<TFloat>::etToString,
                "Convert ephemeris time to string",
                py::arg("et"), py::arg("format") = "C", py::arg("precision") = 6)

            // Ephemeris query methods
            .def_static(("spkezr_" + suffix).c_str(),
                &SpiceUtils<TFloat>::spkezr,
                "Get position and velocity of target relative to observer",
                py::arg("target"), py::arg("et"), py::arg("frame_name"),
                py::arg("abcorr"), py::arg("observer"))
            .def_static(("spkpos_" + suffix).c_str(),
                &SpiceUtils<TFloat>::spkpos,
                "Get position of target relative to observer",
                py::arg("target"), py::arg("et"), py::arg("frame_name"),
                py::arg("abcorr"), py::arg("observer"))

            // Frame transformation methods
            .def_static(("pxform_" + suffix).c_str(),
                &SpiceUtils<TFloat>::pxform,
                "Get transformation matrix from one frame to another",
                py::arg("fromFrame"), py::arg("toFrame"), py::arg("et"))
            .def_static(("computeAngularRate_" + suffix).c_str(),
                &SpiceUtils<TFloat>::computeAngularRate,
                "Compute angular rate between two frames",
                py::arg("fromFrame"), py::arg("toFrame"), py::arg("et"))

            // Body property methods
            .def_static(("getRadii_" + suffix).c_str(),
                &SpiceUtils<TFloat>::getRadii,
                "Get radii of celestial body (x, y, z)",
                py::arg("bodyName"))
            .def_static(("getRadius_" + suffix).c_str(),
                &SpiceUtils<TFloat>::getRadius,
                "Get mean radius of celestial body",
                py::arg("bodyName"))

            // Coverage analysis methods
            .def_static(("spkcov_" + suffix).c_str(),
                &SpiceUtils<TFloat>::spkcov,
                "Get time coverage of SPK file for given body ID",
                py::arg("SPKfile"), py::arg("id"), py::arg("et_start"), py::arg("et_end"));
    }
}

#endif