#ifndef VIRA_IMAGE_INTERFACES_PNG_INTERFACE_HPP
#define VIRA_IMAGE_INTERFACES_PNG_INTERFACE_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

#include "vira/spectral_data.hpp"
#include "vira/images/image_pixel.hpp"
#include "vira/images/image_utils.hpp"

namespace fs = std::filesystem;

namespace vira::images {
    // Forward Declare
    template <IsPixel T>
    class Image;

    class ImageInterface {
    public:
        template <IsColorPixel T>
        static void write(const fs::path& filepath, Image<T> image, bool write_alpha = false);
        static void writeImageFile(const std::string& filepath_str, const std::string& extension, int width, int height, size_t channels, const std::vector<unsigned char>& output_buffer, bool hasAlpha);

        inline static void writeNormals(const fs::path& filepath, const Image<vec3<float>>& normals);

        inline static void writeIDs(const fs::path& filepath, const Image<size_t>& ids);

        inline static void writeVelocities(const fs::path& filepath, const Image<vec3<float>>& velocities);

        inline static void writeMap(const fs::path& filepath, Image<float> image, std::vector<vira::ColorRGB> colormap = std::vector<vira::ColorRGB>{});


        template <IsPixel T>
        inline static void writeFITS(const fs::path& filepath, Image<T> image, BufferDataType data_type = BufferDataType::FLOAT32, bool write_alpha = false);

        template <IsPixel T>
        inline static void writeTIFF(const fs::path& filepath, Image<T> image, BufferDataType data_type = BufferDataType::FLOAT32, bool write_alpha = false);

        
        inline static Image<float> createFloatImageFromBuffer(unsigned char* data, int width, int height, size_t channels, bool read_alpha);
        inline static Image<ColorRGB> createRGBImageFromBuffer(unsigned char* data, int width, int height, size_t channels, bool read_alpha);

        inline static Image<float> readImage(const fs::path& filepath, bool read_alpha = false);
        inline static Image<vira::ColorRGB> readImageRGB(const fs::path& filepath, bool read_alpha = false);

        inline static Image<float> readImageFromMemory(const unsigned char* data, size_t dataSize, std::string format, bool read_alpha = false);
        inline static Image<vira::ColorRGB> readImageRGBFromMemory(const unsigned char* data, size_t dataSize, std::string format, bool read_alpha = false);        
    };
};

#include "implementation/images/interfaces/image_interface.ipp"

#endif