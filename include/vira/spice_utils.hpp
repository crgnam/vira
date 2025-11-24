#ifndef VIRA_SPICE_UTILS_HPP
#define VIRA_SPICE_UTILS_HPP

#include <string>
#include <array>
#include <filesystem>

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/constraints.hpp"

namespace fs = std::filesystem;

namespace vira {
    template <IsFloat TFloat>
    class SpiceUtils {
    public:
        static void furnsh(fs::path kernelPath);
        static void furnsh_relative_to_file(fs::path kernelPath);

        static std::string idToName(int id);

        static int nameToID(std::string naif_name);

        static double stringToET(std::string timeString);
        
        static std::string etToString(double et, std::string format = "C", int precision = 6);

        static std::array<vec3<TFloat>, 2> spkezr(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer);

        static vec3<TFloat> spkpos(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer);

        static mat3<TFloat> pxform(const std::string& fromFrame, const std::string& toFrame, double et);

        static vira::vec3<TFloat> computeAngularRate(const std::string& fromFrame, const std::string& toFrame, double et);

        static vira::vec3<TFloat> computeVelocity(const std::string& target, double et, const std::string& frame_name, const std::string& abcorr, const std::string& observer);

        static vec3<TFloat> getRadii(const std::string& bodyName);

        static TFloat getRadius(const std::string& bodyName);

        static void spkcov(fs::path SPKfile, int id, double& et_start, double& et_end);

    private:
        template<typename Func, typename... Args>
        static auto safe_spice_call(Func&& func, Args&&... args);
    };
};

#include "implementation/spice_utils.ipp"

#endif