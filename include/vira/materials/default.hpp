#ifndef VIRA_MATERIALS_DEFAULT_MATERIAL_HPP
#define VIRA_MATERIALS_DEFAULT_MATERIAL_HPP

#include "vira/constraints.hpp"
#include "vira/materials/lambertian.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    class DefaultMaterial : public Lambertian<TSpectral> {
    public:
        DefaultMaterial()
        {
            this->defaultValue = vira::spectralConvert_val<ColorRGB, TSpectral>(ColorRGB{ 1,0,1 });

            this->setAlbedo(defaultValue);
        }

    private:
        TSpectral defaultValue;
    };
};

#endif