#include <filesystem>
#include <string>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

#include "bindings/math_py.ipp"
#include "bindings/reference_frame_py.ipp"
#include "bindings/rotation_py.ipp"
#include "bindings/spice_utils_py.ipp"
#include "bindings/vec_py.ipp"

namespace py = pybind11;

// Custom type caster for std::filesystem::path
namespace pybind11 {
    namespace detail {
        template <> struct type_caster<std::filesystem::path> {
        public:
            PYBIND11_TYPE_CASTER(std::filesystem::path, _("path"));

            bool load(handle src, bool) {
                if (src.is_none()) return false;
                if (PyUnicode_Check(src.ptr())) {
                    value = std::filesystem::path(pybind11::str(src).cast<std::string>());
                    return true;
                }
                return false;
            }

            static handle cast(const std::filesystem::path& src, return_value_policy /* policy */, handle /* parent */) {
                return pybind11::str(src.string());
            }
        };
    }
}

// ========================= //
// === Binding functions === //
// ========================= //

namespace vira {
    static inline void bind_non_templates(py::module& m)
    {
        (void)m;
    };

    template <IsSpectral TSpectral>
    void bind_spectral_templates(py::module& m, const std::string& suffix)
    {
        (void)m;
        (void)suffix;
    };

    template <IsFloat TFloat>
    void bind_precision_templates(py::module& m, const std::string& suffix)
    {
        bind_math<TFloat>(m, suffix);
        bind_reference_frame<TFloat>(m, suffix);
        bind_rotation<TFloat>(m, suffix);
        bind_spice_utils<TFloat>(m, suffix);
        bind_vec<TFloat>(m, suffix);
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void bind_spectral_precision_templates(py::module& m, const std::string& suffix)
    {
        (void)m;
        (void)suffix;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void bind_full_templates(py::module& m, const std::string& suffix)
    {
        (void)m;
        (void)suffix;
    };

    template <IsSpectral TSpectral>
    void bind_all_spectral_templates(py::module& m, const std::string& suffix)
    {
        bind_spectral_templates<vira::Visible_8bin>(m, suffix);
        
        bind_spectral_precision_templates<vira::Visible_8bin, float>(m, suffix + "F");
        bind_spectral_precision_templates<vira::Visible_8bin, double>(m, suffix + "D");

        bind_full_templates<vira::Visible_8bin, float, float>(m, suffix + "FF");
        bind_full_templates<vira::Visible_8bin, double, float>(m, suffix + "DF");
    };
};




PYBIND11_MODULE(virapy, m) {
    m.doc() = "Python bindings for the Vira C++ library";

    vira::bind_non_templates(m);

    vira::bind_precision_templates<float>(m, "F");
    vira::bind_precision_templates<double>(m, "D");

    vira::bind_all_spectral_templates<vira::Visible_8bin>(m, "V8");
};