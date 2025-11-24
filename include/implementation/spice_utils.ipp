#include <string>
#include <stdexcept>
#include <filesystem>

#include "cspice/SpiceUsr.h"

#include "vira/vec.hpp"
#include "vira/constraints.hpp"

namespace fs = std::filesystem;

namespace vira {
    template <IsFloat TFloat>
    template<typename Func, typename... Args>
    auto SpiceUtils<TFloat>::safe_spice_call(Func&& func, Args&&... args) {
        if (failed_c()) {
            reset_c();
        }

        if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
            func(std::forward<Args>(args)...);

            if (failed_c()) {
                constexpr int MSG_LEN = 1840;
                SpiceChar msg[MSG_LEN];
                getmsg_c("SHORT", MSG_LEN, msg);

                std::string error_msg = "Spice error: " + std::string(msg);
                reset_c();

                throw std::runtime_error(error_msg);
            }
        }
        else {
            auto result = func(std::forward<Args>(args)...);

            if (failed_c()) {
                constexpr int MSG_LEN = 1840;
                SpiceChar msg[MSG_LEN];
                getmsg_c("SHORT", MSG_LEN, msg);

                std::string error_msg = "Spice error: " + std::string(msg);
                reset_c();

                throw std::runtime_error(error_msg);
            }

            return result;
        }
    }

    template <IsFloat TFloat>
    void SpiceUtils<TFloat>::furnsh(fs::path kernelPath)
    {
        safe_spice_call(furnsh_c, kernelPath.string().c_str());
    }

    template <IsFloat TFloat>
    void SpiceUtils<TFloat>::furnsh_relative_to_file(fs::path kernelPath)
    {
        if (!kernelPath.has_parent_path()) {
            furnsh(kernelPath);
            return;
        }

        auto original_dir = std::filesystem::current_path();
        try {
            std::filesystem::current_path(kernelPath.parent_path());
            safe_spice_call([&]() { furnsh_c(kernelPath.filename().string().c_str()); });
            std::filesystem::current_path(original_dir);

        }
        catch (const std::filesystem::filesystem_error&) {
            std::filesystem::current_path(original_dir);
            throw;
        }
    }

    template <IsFloat TFloat>
    std::string SpiceUtils<TFloat>::idToName(int id)
    {
        constexpr const int MAXLEN = 32;
        SpiceBoolean found;
        SpiceChar NAME[MAXLEN];
        safe_spice_call(bodc2n_c, id, MAXLEN, NAME, &found);

        if (!found) {
            throw std::runtime_error("The supplied Spice ID (" + std::to_string(id) + ") is unknown, and no corresponding NAIF naif_name_ could be found.");
        }

        return std::string(NAME);
    }

    template <IsFloat TFloat>
    int SpiceUtils<TFloat>::nameToID(std::string naif_name_)
    {
        SpiceBoolean found;
        SpiceInt code;
        safe_spice_call(bodn2c_c, naif_name_.c_str(), &code, &found);

        if (!found) {
            throw std::runtime_error("The supplied Spice naif_name_ (" + naif_name_ + ") is unknown, and no corresponding NAIF ID could be found.");
        }

        return static_cast<int>(code);
    }

    template <IsFloat TFloat>
    double SpiceUtils<TFloat>::stringToET(std::string timeString)
    {
        SpiceDouble et;
        safe_spice_call(str2et_c, timeString.c_str(), &et);
        return static_cast<double>(et);
    }

    template <IsFloat TFloat>
    std::string SpiceUtils<TFloat>::etToString(double et, std::string format, int precision)
    {
        constexpr const int TIMLEN = 35;
        SpiceChar utcstr[TIMLEN];
        safe_spice_call(et2utc_c, et, format.c_str(), precision, TIMLEN, utcstr);
        return std::string(utcstr);
    }

    template <IsFloat TFloat>
    std::array<vec3<TFloat>, 2> SpiceUtils<TFloat>::spkezr(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer)
    {
        SpiceDouble lt;
        SpiceDouble state[6];
        safe_spice_call(spkezr_c, target.c_str(), et, frame_name.c_str(), abcorr.c_str(), observer.c_str(), state, &lt);

        vec3<TFloat> position{ 1000 * state[0], 1000 * state[1], 1000 * state[2] };
        vec3<TFloat> velocity{ 1000 * state[3], 1000 * state[4], 1000 * state[5] };

        return std::array<vec3<TFloat>, 2>{ position, velocity };
    }

    template <IsFloat TFloat>
    vec3<TFloat> SpiceUtils<TFloat>::spkpos(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer)
    {
        SpiceDouble lt;
        SpiceDouble pos[3];
        safe_spice_call(spkpos_c, target.c_str(), et, frame_name.c_str(), abcorr.c_str(), observer.c_str(), pos, &lt);

        return vec3<TFloat>{ 1000 * pos[0], 1000 * pos[1], 1000 * pos[2] };
    }

    template <IsFloat TFloat>
    mat3<TFloat> SpiceUtils<TFloat>::pxform(const std::string& fromFrame, const std::string& toFrame, double et)
    {
        SpiceDouble rotation_matrix[3][3];
        safe_spice_call(pxform_c, fromFrame.c_str(), toFrame.c_str(), et, rotation_matrix);

        mat3<TFloat> outputRotation;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                // Remember, GLM uses column-major:
                outputRotation[j][i] = static_cast<TFloat>(rotation_matrix[i][j]);
            }
        }

        return outputRotation;
    }

    template <IsFloat TFloat>
    vec3<TFloat> SpiceUtils<TFloat>::computeAngularRate(const std::string& fromFrame, const std::string& toFrame, double et)
    {
        SpiceDouble state_matrix[6][6];
        safe_spice_call(sxform_c, fromFrame.c_str(), toFrame.c_str(), et, state_matrix);

        SpiceDouble rotation_matrix[3][3];
        SpiceDouble angular_velocity[3];
        safe_spice_call(xf2rav_c, state_matrix, rotation_matrix, angular_velocity);

        return vira::vec3<TFloat>{
            static_cast<TFloat>(angular_velocity[0]),
                static_cast<TFloat>(angular_velocity[1]),
                static_cast<TFloat>(angular_velocity[2])
        };
    };

    template <IsFloat TFloat>
    vec3<TFloat> SpiceUtils<TFloat>::computeVelocity(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer)
    {
        auto state = spkezr(target, et, frame_name, abcorr, observer);
        return state[1];
    };

    template <IsFloat TFloat>
    vec3<TFloat> SpiceUtils<TFloat>::getRadii(const std::string& bodyName)
    {
        SpiceDouble values[3];
        SpiceInt dim;
        safe_spice_call(bodvrd_c, bodyName.c_str(), "RADII", 3, &dim, values);

        return vec3<TFloat>{1000 * values[0], 1000 * values[1], 1000 * values[2]};
    }

    template <IsFloat TFloat>
    TFloat SpiceUtils<TFloat>::getRadius(const std::string& bodyName)
    {
        SpiceDouble values[3];
        SpiceInt dim;
        safe_spice_call(bodvrd_c, bodyName.c_str(), "RADII", 3, &dim, values);

        return 1000 * static_cast<TFloat>(values[0] + values[1] + values[2]) / TFloat{ 3 };
    }

    template <IsFloat TFloat>
    void SpiceUtils<TFloat>::spkcov(fs::path SPKfile, int id, double& et_start, double& et_end)
    {
        safe_spice_call([&]() {
#define  MAXIV  1000
#define  WINSIZ ( 2 * MAXIV )
            SPICEDOUBLE_CELL(cover, WINSIZ);
            spkcov_c(SPKfile.string().c_str(), id, &cover);
            wnfetd_c(&cover, 0, &et_start, &et_end);
            });
    }
};