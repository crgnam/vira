#ifndef VIRAPY_MATH_PY
#define VIRAPY_MATH_PY

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"

#include "vira/math.hpp"

namespace py = pybind11;

namespace vira {
    template <typename TFloat>
    void bind_math(py::module& m, const std::string& suffix) {
        // Math constants
        m.def(("PI_" + suffix).c_str(), &PI<TFloat>, "Mathematical constant pi");
        m.def(("INV_PI_" + suffix).c_str(), &INV_PI<TFloat>, "Inverse of pi (1/pi)");
        m.def(("PI_OVER_2_" + suffix).c_str(), &PI_OVER_2<TFloat>, "Pi divided by 2");
        m.def(("PI_OVER_4_" + suffix).c_str(), &PI_OVER_4<TFloat>, "Pi divided by 4");
        m.def(("INV_2_PI_" + suffix).c_str(), &INV_2_PI<TFloat>, "Inverse of 2*pi");
        m.def(("INV_4_PI_" + suffix).c_str(), &INV_4_PI<TFloat>, "Inverse of 4*pi");

        // Physical constants
        m.def(("SPEED_OF_LIGHT_" + suffix).c_str(), &SPEED_OF_LIGHT<TFloat>, "Speed of light in m/s");
        m.def(("PLANCK_CONSTANT_" + suffix).c_str(), &PLANCK_CONSTANT<TFloat>, "Planck constant in J/Hz");
        m.def(("BOLTZMANN_CONSTANT_" + suffix).c_str(), &BOLTZMANN_CONSTANT<TFloat>, "Boltzmann constant in J/K");

        // Unit conversion constants
        m.def(("RAD2DEG_" + suffix).c_str(), &RAD2DEG<TFloat>, "Radians to degrees conversion factor");
        m.def(("DEG2RAD_" + suffix).c_str(), &DEG2RAD<TFloat>, "Degrees to radians conversion factor");
        m.def(("NANOMETERS_" + suffix).c_str(), &NANOMETERS<TFloat>, "Nanometer scale factor (1e-9)");
        m.def(("AUtoKM_" + suffix).c_str(), &AUtoKM<TFloat>, "Astronomical unit to kilometers conversion");
        m.def(("AUtoM_" + suffix).c_str(), &AUtoM<TFloat>, "Astronomical unit to meters conversion");

        // Time constants
        m.def(("SECONDS_PER_DAY_" + suffix).c_str(), &SECONDS_PER_DAY<TFloat>, "Number of seconds in a day");
        m.def(("SECONDS_PER_YEAR_" + suffix).c_str(), &SECONDS_PER_YEAR<TFloat>, "Number of seconds in a year");

        // Special constants
        m.def("SOLAR_RADIUS", &SOLAR_RADIUS, "Solar radius in meters");
        m.def(("INF_" + suffix).c_str(), &INF<TFloat>, "Floating point infinity");

        // Physics functions
        m.def(("PhotonEnergy_" + suffix).c_str(), &PhotonEnergy<TFloat>, "Calculate photon energy from wavelength",
            py::arg("wavelength"));

        m.def(("PhotonEnergyFreq_" + suffix).c_str(), &PhotonEnergyFreq<TFloat>, "Calculate photon energy from frequency",
            py::arg("frequency"));

        // Planck's law functions
        m.def(("plancksLaw_" + suffix).c_str(),
            py::overload_cast<TFloat, TFloat>(&plancksLaw<TFloat>),
            "Planck's law for single wavelength",
            py::arg("temperature"), py::arg("wavelength"));

        m.def(("plancksLaw_" + suffix).c_str(),
            py::overload_cast<TFloat, std::vector<TFloat>>(&plancksLaw<TFloat>),
            "Planck's law for multiple wavelengths",
            py::arg("temperature"), py::arg("wavelengths"));

        m.def(("plancksLawFreq_" + suffix).c_str(),
            py::overload_cast<TFloat, TFloat>(&plancksLawFreq<TFloat>),
            "Planck's law for single frequency",
            py::arg("temperature"), py::arg("frequency"));

        m.def(("plancksLawFreq_" + suffix).c_str(),
            py::overload_cast<TFloat, std::vector<TFloat>>(&plancksLawFreq<TFloat>),
            "Planck's law for multiple frequencies",
            py::arg("temperature"), py::arg("frequencies"));

        // Utility functions
        m.def(("linspace_" + suffix).c_str(), &linspace<TFloat>,
            "Generate linearly spaced values",
            py::arg("min"), py::arg("max"), py::arg("N"));

        m.def(("linearInterpolate_" + suffix).c_str(), &linearInterpolate<TFloat>,
            "Linear interpolation",
            py::arg("sampleX"), py::arg("x"), py::arg("y"));

        m.def(("trapezoidIntegrate_" + suffix).c_str(), &trapezoidIntegrate<TFloat>,
            "Trapezoidal integration",
            py::arg("x"), py::arg("y"),
            py::arg("bmin") = -INF<TFloat>(),
            py::arg("bmax") = INF<TFloat>());

        // Bessel function
        m.def(("vira_cyl_bessel_j_" + suffix).c_str(), &vira_cyl_bessel_j<TFloat>,
            "Cylindrical Bessel function of the first kind",
            py::arg("nu"), py::arg("x"));
    }
}

#endif