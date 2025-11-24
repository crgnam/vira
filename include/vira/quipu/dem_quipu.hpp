#ifndef VIRA_DEMQuipu_DEM_QUIPU_HPP
#define VIRA_DEMQuipu_DEM_QUIPU_HPP

#include <cstdint>
#include <limits>
#include <filesystem>
#include <vector>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/quipu/class_ids.hpp"
#include "vira/dems/dem.hpp"

namespace fs = std::filesystem;

namespace vira::quipu {
    struct DEMQuipuWriterOptions {
        bool compress = false;
        bool writeAlbedos = true;
    };

    template <IsSpectral TSpectral>
    struct DEMQuipuReaderOptions {
        bool readAlbedos = true;
        float defaultGSD = std::numeric_limits<float>::infinity();
        float defaultAlbedo = 0.06f;
        TSpectral albedoProfile{ 1.f };
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class DEMQuipu {
    public:
        DEMQuipu() = default;
        DEMQuipu(const fs::path& newFilepath, DEMQuipuReaderOptions<TSpectral> newOptions = DEMQuipuReaderOptions<TSpectral>{});
        ~DEMQuipu();

        DEMQuipu(const DEMQuipu&) = default;
        DEMQuipu& operator=(const DEMQuipu&) = default;

        DEMQuipu(DEMQuipu&&) = default;
        DEMQuipu& operator=(DEMQuipu&&) = default;

        std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>> readPyramid();

        void readBuffers(vira::geometry::VertexBuffer<TSpectral, TMeshFloat>& vertexBuffer, vira::geometry::IndexBuffer& indexBuffer, double requiredGSD);
        void readBuffers(vira::geometry::VertexBuffer<TSpectral, TMeshFloat>& vertexBuffer, vira::geometry::IndexBuffer& indexBuffer);
        mat4<double> getTransformation() { return transformation_; }
        float getCurrentGSD() { return currentGSD; }
        float getDefaultGSD() { return defaultGSD; }

        bool isCompressed() { return _compressed; }
        bool hasAlbedos() { return _hasAlbedos; }

        Normal getNormal() { return normal; }
        float getConeAngle() { return coneAngle; }

        const fs::path& getFilepath() const { return filepath; }

        // Write methods:
        static void write(fs::path file, const std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>>& pyramid, mat4<double> transformation, DEMQuipuWriterOptions options = DEMQuipuWriterOptions{});

    private:
        fs::path filepath = "";
        DEMQuipuReaderOptions<TSpectral> options;

        // Header details read from file:
        float defaultGSD = std::numeric_limits<float>::infinity();
        float currentGSD = std::numeric_limits<float>::infinity();
        ViraClassID classID;
        mat4<double> transformation_;
        uint16_t numberOfLoDs;

        bool _compressed;
        bool _hasAlbedos;

        Normal normal;
        float coneAngle;
        std::vector<double> gsds;
        std::vector<size_t> offsets;
        size_t headerSize;

        TSpectral albedoProfile;
    };
};

#include "implementation/quipu/dem_quipu.ipp"

#endif