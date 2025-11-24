#ifndef VIRA_UNRESOLVED_MAGNITUDES_HPP
#define VIRA_UNRESOLVED_MAGNITUDES_HPP

#include <cmath>

#include "vira/math.hpp"

namespace vira::unresolved {
    class Band {
    public:
        constexpr Band(double lambda, double newFWHM, double newJY) :
            fwhm{ newFWHM }, jy{ newJY }
        { 
            constexpr const double janksy = 1e-23; // erg / (s cm^2 Hz)

            double janksy_m = janksy * 10000; // erg / (s m^2 Hz)
            double freq = SPEED_OF_LIGHT<double>() / lambda;
            double janksy_m_hz = janksy_m * freq; // erg / (s m^2)

            double energy = PhotonEnergyFreq<double>(freq); // J
            double energy_erg = energy / 1e-7; // erg

            this->jy2photon = janksy_m_hz / energy_erg; // photon / (s m^2)
        }

        double fluxFromMagnitude(double apparentMagnitude) const
        {
            // Calculate flux diminishing factor:
            double d = std::pow(10., apparentMagnitude / -2.5);
            double flux = d * jy * jy2photon * fwhm;
            return flux;
        }

    private:
        double fwhm;
        double jy;
        double jy2photon;
    };

    // Pre-define apparent magnitude bands:
    constexpr Band Uband = Band(360.,  0.15, 1810.);
    constexpr Band Bband = Band(440.,  0.22, 4260.);
    constexpr Band Vband = Band(550.,  0.16, 3640.);
    constexpr Band Rband = Band(640.,  0.23, 3080.);
    constexpr Band Iband = Band(790.,  0.19, 2550.);
    constexpr Band Jband = Band(1260., 0.16, 1600.);
    constexpr Band Hband = Band(1600., 0.23, 1080.);
    constexpr Band Kband = Band(2200., 0.23,  670.);
    constexpr Band gband = Band(520.,  0.14, 3730.);
    constexpr Band rband = Band(670.,  0.14, 4490.);
    constexpr Band iband = Band(790.,  0.16, 4760.);
    constexpr Band zband = Band(910.,  0.13, 4810.);

    template <IsFloat TFloat>
    TFloat normcdf(TFloat x, TFloat mean = 0, TFloat stddev = 1);

    template <IsFloat TFloat>
    std::vector<TFloat> johnsonVBandApproximation(size_t N, std::vector<TFloat>& lambda);
};

#include "implementation/unresolved/magnitudes.ipp"

#endif