#include <cmath>
#include <vector>

#include "vira/math.hpp"

namespace vira::unresolved {
    template <IsFloat TFloat>
    TFloat normcdf(TFloat x, TFloat mean, TFloat stddev) {
        TFloat z = (x - mean) / stddev;
        TFloat invSqrt2 = 1 / std::sqrt(2);
        return 0.5 * std::erfc(-z * invSqrt2);
    }
    
    template <IsFloat TFloat>
    std::vector<TFloat> johnsonVBandApproximation(size_t N, std::vector<TFloat>& lambda)
    {
        // This was a heuristically derived approximation:
        std::vector<TFloat> vband(N);
    
        TFloat sigma = 63e-9;
        TFloat mu = 505e-9;
        TFloat alpha = 6e7;
        TFloat scale = 5.4729e6;
    
        lambda = linspace(450e-9, 725e-9, N);
        for (size_t i = 0; i < N; ++i) {
            TFloat x = lambda[i] - mu;
            TFloat s2 = sigma * sigma;
    
            TFloat a = 1 / std::sqrt(2 * PI<TFloat>() * s2);
            TFloat b = std::exp(-(x * x) / (2 * s2));
    
            vband[i] = a * b * normcdf<TFloat>(alpha * x) / scale;
        }
    
        return vband;
    };
}