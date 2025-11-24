#ifndef VIRA_UNITS_UNIT_TYPES_HPP 
#define VIRA_UNITS_UNIT_TYPES_HPP

#include "vira/units/base_unit.hpp"

namespace vira::units {
    // ======================= //
    // === Base Unit Types === //
    // ======================= //
    using TimeUnit = Unit<1, 0, 0, 0, 0, 0, 0, 0, 0>;
    template <typename T>
    concept IsTimeUnit = DerivedFrom<T, TimeUnit>;

    using LengthUnit = Unit<0, 1, 0, 0, 0, 0, 0, 0, 0>;
    template <typename T>
    concept IsLengthUnit = DerivedFrom<T, LengthUnit>;

    using MassUnit = Unit<0, 0, 1, 0, 0, 0, 0, 0, 0>;
    template <typename T>
    concept IsMassUnit = DerivedFrom<T, MassUnit>;

    using CurrentUnit = Unit<0, 0, 0, 1, 0, 0, 0, 0, 0>;
    template <typename T>
    concept IsCurrentUnit = DerivedFrom<T, CurrentUnit>;

    using TemperatureUnit = Unit<0, 0, 0, 0, 1, 0, 0, 0, 0>;
    template <typename T>
    concept IsTemperatureUnit = DerivedFrom<T, TemperatureUnit>;

    using AmountOfSubstanceUnit = Unit<0, 0, 0, 0, 0, 1, 0, 0, 0>;
    template <typename T>
    concept IsSubstanceUnit = DerivedFrom<T, AmountOfSubstanceUnit>;

    using LuminousIntensityUnit = Unit<0, 0, 0, 0, 0, 0, 1, 0, 0>;
    template <typename T>
    concept IsLuminosityUnit = DerivedFrom<T, LuminousIntensityUnit>;

    using AngleUnit = Unit<0, 0, 0, 0, 0, 0, 0, 1, 0>;
    template <typename T>
    concept IsAngleUnit = DerivedFrom<T, AngleUnit>;

    using SolidAngleUnit = Unit<0, 0, 0, 0, 0, 0, 0, 0, 1>;
    template <typename T>
    concept IsSolidAngleUnit = DerivedFrom<T, SolidAngleUnit>;




    // ============================ //
    // === Derived Unit Helpers === //
    // ============================ //
    template <typename... Units>
    struct multiply_dimensions;

    // Base case for single unit
    template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
    struct multiply_dimensions<Unit<T, L, M, I, O, N, J, A, S>> {
        using type = Unit<T, L, M, I, O, N, J, A, S>;
    };

    // Case for two units - direct dimension addition
    template <
        int T1, int L1, int M1, int I1, int O1, int N1, int J1, int A1, int S1,
        int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2
    >
    struct multiply_dimensions<
        Unit<T1, L1, M1, I1, O1, N1, J1, A1, S1>,
        Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>
    > {
        using type = Unit<
            T1 + T2, L1 + L2, M1 + M2, I1 + I2, O1 + O2,
            N1 + N2, J1 + J2, A1 + A2, S1 + S2
        >;
    };

    // Recursive case for multiple units
    template <typename U1, typename U2, typename... Rest>
    struct multiply_dimensions<U1, U2, Rest...> {
        using intermediate = typename multiply_dimensions<U1, U2>::type;
        using type = typename multiply_dimensions<intermediate, Rest...>::type;
    };

    // Helper for dividing unit dimensions - direct dimension subtraction
    template <typename Numerator, typename Denominator>
    struct divide_dimensions;

    template <
        int T1, int L1, int M1, int I1, int O1, int N1, int J1, int A1, int S1,
        int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2
    >
    struct divide_dimensions<
        Unit<T1, L1, M1, I1, O1, N1, J1, A1, S1>,
        Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>
    > {
        using type = Unit<
            T1 - T2, L1 - L2, M1 - M2, I1 - I2, O1 - O2,
            N1 - N2, J1 - J2, A1 - A2, S1 - S2
        >;
    };

    // Public interface: Multiply any number of units
    template <typename... Units>
    using Multiply = typename multiply_dimensions<Units...>::type;

    // Public interface: Divide exactly two units
    template <typename Numerator, typename Denominator>
    using Divide = typename divide_dimensions<Numerator, Denominator>::type;

    // Helper for squared unit
    template <typename Unit>
    using Square = typename multiply_dimensions<Unit, Unit>::type;

    // Helper for cubed unit
    template <typename Unit>
    using Cube = typename multiply_dimensions<Unit, Unit, Unit>::type;




    // ================================= //
    // === Common Derived Unit Types === //
    // ================================= //
    using AreaUnit = Square<LengthUnit>;
    template <typename T>
    concept IsAreaUnit = DerivedFrom<T, AreaUnit>;

    using VolumeUnit = Cube<LengthUnit>;
    template <typename T>
    concept IsVolumeUnit = DerivedFrom<T, VolumeUnit>;

    using VelocityUnit = Divide<LengthUnit, TimeUnit>;
    template <typename T>
    concept IsVelocityUnit = DerivedFrom<T, VelocityUnit>;

    using AccelerationUnit = Divide<LengthUnit, Square<TimeUnit>>;
    template <typename T>
    concept IsAccelerationUnit = DerivedFrom<T, AccelerationUnit>;

    using FrequencyUnit = Divide<ScalarUnit, TimeUnit>;
    template <typename T>
    concept IsFrequencyUnit = DerivedFrom<T, FrequencyUnit>;

    using ForceUnit = Multiply<MassUnit, AccelerationUnit>;
    template <typename T>
    concept IsForceUnit = DerivedFrom<T, ForceUnit>;

    using EnergyUnit = Multiply<ForceUnit, LengthUnit>;
    template <typename T>
    concept IsEnergyUnit = DerivedFrom<T, EnergyUnit>;

    using PowerUnit = Divide<EnergyUnit, TimeUnit>;
    template <typename T>
    concept IsPowerUnit = DerivedFrom<T, PowerUnit>;

    using PressureUnit = Divide<ForceUnit, AreaUnit>;
    template <typename T>
    concept IsPressureUnit = DerivedFrom<T, PressureUnit>;

    using IrradianceUnit = Divide<PowerUnit, AreaUnit>;
    template <typename T>
    concept IsIrradianceUnit = DerivedFrom<T, IrradianceUnit>;

    using RadianceUnit = Divide<IrradianceUnit, SolidAngleUnit>;
    template <typename T>
    concept IsRadianceUnit = DerivedFrom<T, RadianceUnit>;

    using ChargeUnit = Multiply<CurrentUnit, TimeUnit>;
    template <typename T>
    concept IsChargeUnit = DerivedFrom<T, ChargeUnit>;

    using MomentumUnit = Multiply<MassUnit, VelocityUnit>;
    template <typename T>
    concept IsMomentumUnit = DerivedFrom<T, MomentumUnit>;
};

#endif