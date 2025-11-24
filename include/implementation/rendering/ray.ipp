#include <cmath>
#include <type_traits>
#include <cstdint>
#include <cstring>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"

namespace vira::rendering {
    // Allow for casting interactions between type instantiations:
    template <IsSpectral TSpectral, IsFloat TFloat>
    template <IsFloat TFloat2>
    Interaction<TSpectral, TFloat>::operator Interaction<TSpectral, TFloat2>()
    {

        Interaction<TSpectral, TFloat2> cast_interaction;
        cast_interaction.t = static_cast<TFloat2>(this->t);

        cast_interaction.face_normal = this->face_normal;

        cast_interaction.vert[0] = this->vert[0];
        cast_interaction.vert[1] = this->vert[1];
        cast_interaction.vert[2] = this->vert[2];

        cast_interaction.w[0] = static_cast<TFloat2>(this->w[0]);
        cast_interaction.w[1] = static_cast<TFloat2>(this->w[1]);
        cast_interaction.w[2] = static_cast<TFloat2>(this->w[2]);

        cast_interaction.tri_id = this->tri_id;
        cast_interaction.material_cache_index = this->material_cache_index;

        cast_interaction.mesh_ptr = this->mesh_ptr;
        cast_interaction.instance_ptr = this->instance_ptr;
        return cast_interaction;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    Ray<TSpectral, TFloat>::Ray(vec3<TFloat> new_origin, vec3<TFloat> new_direction) : 
        origin{ new_origin }, direction{ new_direction }
    {
        reciprocal_direction = TFloat{ 1 } / direction;
    };





    // Adapted from 6.2.2.4 from: https://link.springer.com/content/pdf/10.1007/978-1-4842-4427-2_6.pdf
    template <IsFloat T>
    constexpr T origin() {
        if constexpr (std::same_as<T, float>) {
            return 1.f / 32.f;
        }
        else {
            return 1. / 64.;
        }
    };

    template <IsFloat T>
    constexpr T float_scale() {
        if constexpr (std::same_as<T, float>) {
            return 1.f / 65536.f;
        }
        else {
            return 1. / 4294967296.;
        }
    };

    template <IsFloat T>
    constexpr T int_scale() {
        if constexpr (std::same_as<T, float>) {
            return 256.0f;
        }
        else {
            return 65536.;
        }
    };

    inline int32_t float_as_int(float val)
    {
        int32_t out;
        std::memcpy(&out, &val, sizeof(float));
        return out;
    };

    inline int64_t float_as_int(double val)
    {
        int64_t out;
        std::memcpy(&out, &val, sizeof(double));
        return out;
    }

    inline float int_as_float(int32_t val)
    {
        float out;
        std::memcpy(&out, &val, sizeof(int32_t));
        return out;
    };

    inline double int_as_float(int64_t val)
    {
        double out;
        std::memcpy(&out, &val, sizeof(int64_t));
        return out;
    };

    template <IsFloat TMeshFloat>
    inline vec3<TMeshFloat> offsetIntersection(vec3<TMeshFloat> intersection, const vec3<float>& N)
    {
        vec3<TMeshFloat> newIntersection;
        if constexpr (std::same_as<TMeshFloat, float>) {
            vec3<int32_t> of_i{ int_scale<TMeshFloat>() * N.x, int_scale<TMeshFloat>() * N.y, int_scale<TMeshFloat>() * N.z };

            vec3<TMeshFloat> p_i{
                int_as_float(float_as_int(intersection.x) + ((intersection.x < 0) ? -of_i.x : of_i.x)),
                int_as_float(float_as_int(intersection.y) + ((intersection.y < 0) ? -of_i.y : of_i.y)),
                int_as_float(float_as_int(intersection.z) + ((intersection.z < 0) ? -of_i.z : of_i.z))
            };

            newIntersection.x = (fabsf(intersection.x) < origin<TMeshFloat>() ? intersection.x + float_scale<TMeshFloat>() * N.x : p_i.x);
            newIntersection.y = (fabsf(intersection.y) < origin<TMeshFloat>() ? intersection.y + float_scale<TMeshFloat>() * N.y : p_i.y);
            newIntersection.z = (fabsf(intersection.z) < origin<TMeshFloat>() ? intersection.z + float_scale<TMeshFloat>() * N.z : p_i.z);
        }
        else {
            vec3<int64_t> of_i{ int_scale<TMeshFloat>() * N.x, int_scale<TMeshFloat>() * N.y, int_scale<TMeshFloat>() * N.z };

            vec3<TMeshFloat> p_i{
                int_as_float(float_as_int(intersection.x) + ((intersection.x < 0) ? -of_i.x : of_i.x)),
                int_as_float(float_as_int(intersection.y) + ((intersection.y < 0) ? -of_i.y : of_i.y)),
                int_as_float(float_as_int(intersection.z) + ((intersection.z < 0) ? -of_i.z : of_i.z))
            };

            newIntersection.x = (fabs(intersection.x) < origin<TMeshFloat>() ? intersection.x + float_scale<TMeshFloat>() * N.x : p_i.x);
            newIntersection.y = (fabs(intersection.y) < origin<TMeshFloat>() ? intersection.y + float_scale<TMeshFloat>() * N.y : p_i.y);
            newIntersection.z = (fabs(intersection.z) < origin<TMeshFloat>() ? intersection.z + float_scale<TMeshFloat>() * N.z : p_i.z);
        }

        return newIntersection;
    };
};