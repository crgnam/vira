#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include "stb_image_write.h"

#include "tiffio.h"

#include "vira/platform/windows_compat.hpp" // MUST CALL BEFORE FITSIO.H
#include "fitsio.h"

#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"
#include "vira/utils/utils.hpp"
#include "vira/images/color_map.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/images/image_pixel.hpp"

namespace fs = std::filesystem;

namespace vira::images {
    template <IsColorPixel T>
    void ImageInterface::write(const fs::path& filepath, Image<T> image, bool write_alpha)
    {
        utils::makePath(filepath);

        Resolution resolution = image.resolution();
        if ((resolution.x <= 0) || (resolution.y <= 0)) {
            std::cout << "Image has resolution of (" << std::to_string(resolution.x) << ", " << std::to_string(resolution.y) << "); not writing to file\n";
            return;
        }

        // Get file extension to determine format
        std::string extension = filepath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

        // Detect if invalid format and throw error:
        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" && extension != ".tga" && extension != ".bmp") {
            throw std::runtime_error("Unsupported file format: " + extension + ". Supported formats: .png, .jpg, .tga, .bmp");
        }

        // Convert image to 8-bit buffer (stb only supports 8-bit)
        std::vector<unsigned char> output_buffer = image.toBuffer(write_alpha, BufferDataType::UINT8);
        size_t channels = image.getOutputChannels(write_alpha);

        // Write the file
        writeImageFile(filepath.string(), extension, resolution.x, resolution.y, channels, output_buffer, write_alpha);
    }

    void ImageInterface::writeImageFile(const std::string& filepath_str, const std::string& extension, int width, int height, size_t channels, const std::vector<unsigned char>& output_buffer, bool hasAlpha)
    {
        int result = 0;

        if (extension == ".png") {
            result = stbi_write_png(filepath_str.c_str(), width, height, static_cast<int>(channels),
                output_buffer.data(), width * static_cast<int>(channels));
        }
        else if (extension == ".jpg" || extension == ".jpeg") {
            if (hasAlpha) {
                std::cout << "Warning: JPEG doesn't support alpha channel, writing without alpha\n";
                // Note: For JPEG, you'd need to call image.toBuffer(false, 8) to get RGB-only buffer
                // This is a limitation of this approach - we'd need access to the original image
                // Alternative: strip alpha from the existing buffer
                size_t rgb_channels = (channels == 4) ? 3 : (channels == 2) ? 1 : channels;
                std::vector<unsigned char> rgb_buffer;
                rgb_buffer.reserve(output_buffer.size() * rgb_channels / channels);

                for (size_t i = 0; i < output_buffer.size() / channels; ++i) {
                    for (size_t c = 0; c < rgb_channels; ++c) {
                        rgb_buffer.push_back(output_buffer[i * channels + c]);
                    }
                }

                result = stbi_write_jpg(filepath_str.c_str(), width, height, static_cast<int>(rgb_channels),
                    rgb_buffer.data(), 90);
            }
            else {
                result = stbi_write_jpg(filepath_str.c_str(), width, height, static_cast<int>(channels),
                    output_buffer.data(), 90);
            }
        }
        else if (extension == ".tga") {
            result = stbi_write_tga(filepath_str.c_str(), width, height, static_cast<int>(channels),
                output_buffer.data());
        }
        else if (extension == ".bmp") {
            if (hasAlpha) {
                std::cout << "Warning: BMP doesn't reliably support alpha channel, writing without alpha\n";
                // Same alpha-stripping logic as JPEG
                size_t rgb_channels = (channels == 4) ? 3 : (channels == 2) ? 1 : channels;
                std::vector<unsigned char> rgb_buffer;
                rgb_buffer.reserve(output_buffer.size() * rgb_channels / channels);

                for (size_t i = 0; i < output_buffer.size() / channels; ++i) {
                    for (size_t c = 0; c < rgb_channels; ++c) {
                        rgb_buffer.push_back(output_buffer[i * channels + c]);
                    }
                }

                result = stbi_write_bmp(filepath_str.c_str(), width, height, static_cast<int>(rgb_channels),
                    rgb_buffer.data());
            }
            else {
                result = stbi_write_bmp(filepath_str.c_str(), width, height, static_cast<int>(channels),
                    output_buffer.data());
            }
        }

        if (!result) {
            std::cout << "Failed to write image to: " << filepath_str << "\n";
        }
    }



    template <IsPixel T>
    void ImageInterface::writeFITS(const fs::path& filepath, Image<T> image, BufferDataType data_type, bool write_alpha)
    {
        utils::makePath(filepath);

        Resolution resolution = image.resolution();
        if ((resolution.x <= 0) || (resolution.y <= 0)) {
            std::cout << "Image has resolution of (" << std::to_string(resolution.x) << ", "
                << std::to_string(resolution.y) << "); not writing to file\n";
            return;
        }

        // Get the number of channels and prepare dimensions
        size_t channels = image.getOutputChannels(write_alpha);

        // FITS uses NAXIS convention: NAXIS1=width, NAXIS2=height, NAXIS3=channels (if >1)
        long naxes[3];
        int naxis;

        if (channels == 1) {
            naxis = 2;
            naxes[0] = resolution.x;  // NAXIS1 = width
            naxes[1] = resolution.y;  // NAXIS2 = height
        }
        else {
            naxis = 3;
            naxes[0] = resolution.x;  // NAXIS1 = width  
            naxes[1] = resolution.y;  // NAXIS2 = height
            if (channels > static_cast<size_t>(std::numeric_limits<long>::max())) {
                throw std::runtime_error("Too many channels for FITS format");
            }
            naxes[2] = static_cast<long>(channels);      // NAXIS3 = channels
        }

        // Convert image to buffer with specified data type
        std::vector<unsigned char> byte_buffer = image.toBuffer(write_alpha, data_type);
        size_t total_pixels = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y) * channels;

        // Convert from interleaved to plane-separated buffer:
        if (channels > 1) {
            size_t bytes_per_sample;
            switch (data_type) {
            case BufferDataType::UINT8: bytes_per_sample = 1; break;
            case BufferDataType::UINT16: bytes_per_sample = 2; break;
            case BufferDataType::UINT32:
            case BufferDataType::FLOAT32: bytes_per_sample = 4; break;
            case BufferDataType::UINT64:
            case BufferDataType::FLOAT64: bytes_per_sample = 8; break;
            default: bytes_per_sample = 1; break;
            }

            // Convert from interleaved (RGBRGBRGB...) to plane-separated (RRR...GGG...BBB...)
            std::vector<unsigned char> plane_separated_buffer(byte_buffer.size());
            size_t pixels_per_plane = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);

            for (size_t channel = 0; channel < channels; ++channel) {
                for (size_t pixel = 0; pixel < pixels_per_plane; ++pixel) {
                    // Source: interleaved position
                    size_t src_offset = (pixel * channels + channel) * bytes_per_sample;
                    // Destination: plane-separated position  
                    size_t dst_offset = (channel * pixels_per_plane + pixel) * bytes_per_sample;

                    std::memcpy(&plane_separated_buffer[dst_offset],
                        &byte_buffer[src_offset],
                        bytes_per_sample);
                }
            }

            byte_buffer = std::move(plane_separated_buffer);
        }

        // Create FITS file
        fitsfile* fptr;
        int status = 0;

        // Remove existing file if it exists
        std::remove(filepath.string().c_str());

        // Create new FITS file
        if (fits_create_file(&fptr, filepath.string().c_str(), &status)) {
            fits_report_error(stderr, status);
            throw std::runtime_error("Failed to create FITS file: " + filepath.string());
        }

        // Determine FITS image type and data type based on BufferDataType
        int bitpix;
        int datatype;

        switch (data_type) {
        case BufferDataType::UINT8:
            bitpix = BYTE_IMG;
            datatype = TBYTE;
            break;
        case BufferDataType::UINT16:
            bitpix = USHORT_IMG;
            datatype = TUSHORT;
            break;
        case BufferDataType::UINT32:
            bitpix = ULONG_IMG;
            datatype = TUINT;
            break;
        case BufferDataType::UINT64:
            bitpix = LONGLONG_IMG;
            datatype = TULONGLONG;
            break;
        case BufferDataType::FLOAT32:
            bitpix = FLOAT_IMG;
            datatype = TFLOAT;
            break;
        case BufferDataType::FLOAT64:
            bitpix = DOUBLE_IMG;
            datatype = TDOUBLE;
            break;
        default:
            fits_close_file(fptr, &status);
            throw std::invalid_argument("Unsupported buffer data type for FITS");
        }

        // Create primary image HDU
        if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) {
            fits_close_file(fptr, &status);
            fits_report_error(stderr, status);
            throw std::runtime_error("Failed to create FITS image HDU");
        }

        // Write the image data
        long fpixel[3] = { 1, 1, 1 }; // Start at pixel (1,1,1) - FITS uses 1-based indexing

        if (fits_write_pix(fptr, datatype, fpixel, total_pixels, byte_buffer.data(), &status)) {
            fits_close_file(fptr, &status);
            fits_report_error(stderr, status);
            throw std::runtime_error("Failed to write FITS image data");
        }

        // Add some basic header keywords
        if (fits_write_key(fptr, TSTRING, "CREATOR", const_cast<void*>(static_cast<const void*>("VIRA Image Library")),
            "Software that created this file", &status)) {
            // Non-fatal, just report
            fits_report_error(stderr, status);
            status = 0; // Reset status
        }

        // Add data type information
        std::string data_type_str;
        switch (data_type) {
        case BufferDataType::UINT8: data_type_str = "8-bit unsigned integer"; break;
        case BufferDataType::UINT16: data_type_str = "16-bit unsigned integer"; break;
        case BufferDataType::UINT32: data_type_str = "32-bit unsigned integer"; break;
        case BufferDataType::UINT64: data_type_str = "64-bit unsigned integer"; break;
        case BufferDataType::FLOAT32: data_type_str = "32-bit float"; break;
        case BufferDataType::FLOAT64: data_type_str = "64-bit double"; break;
        default:
            fits_close_file(fptr, &status);
            fits_report_error(stderr, status);
            throw std::invalid_argument("Unsupported buffer data type for FITS");
        }

        if (fits_write_key(fptr, TSTRING, "DATATYPE", const_cast<void*>(static_cast<const void*>(data_type_str.c_str())),
            "Data type of pixel values", &status)) {
            fits_report_error(stderr, status);
            status = 0;
        }

        // Add channel information if multi-channel
        if (channels > 1) {
            char comment[80];
            snprintf(comment, sizeof(comment), "Number of channels: %zu", channels);
            long channels_long = static_cast<long>(channels);
            if (fits_write_key(fptr, TLONG, "NCHANS", &channels_long, comment, &status)) {
                fits_report_error(stderr, status);
                status = 0;
            }

            if (write_alpha && image.hasAlpha()) {
                int has_alpha = 1;
                if (fits_write_key(fptr, TLOGICAL, "HASALPHA", &has_alpha,
                    "Image contains alpha channel", &status)) {
                    fits_report_error(stderr, status);
                    status = 0;
                }
            }
        }

        // Close the file
        if (fits_close_file(fptr, &status)) {
            fits_report_error(stderr, status);
            throw std::runtime_error("Failed to close FITS file");
        }
    }


    template <IsPixel T>
    void ImageInterface::writeTIFF(const fs::path& filepath, Image<T> image, BufferDataType data_type, bool write_alpha)
    {
        utils::makePath(filepath);

        Resolution resolution = image.resolution();
        if ((resolution.x <= 0) || (resolution.y <= 0)) {
            std::cout << "Image has resolution of (" << std::to_string(resolution.x) << ", "
                << std::to_string(resolution.y) << "); not writing to file\n";
            return;
        }

        // Get the number of channels
        size_t channels = image.getOutputChannels(write_alpha);

        // Convert image to buffer with specified data type
        std::vector<unsigned char> byte_buffer = image.toBuffer(write_alpha, data_type);

        // Open TIFF file for writing
        TIFF* tiff = TIFFOpen(filepath.string().c_str(), "w");
        if (!tiff) {
            throw std::runtime_error("Failed to create TIFF file: " + filepath.string());
        }

        // Set basic TIFF tags
        TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(resolution.x));
        TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(resolution.y));
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, static_cast<uint16_t>(channels));
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG); // Interleaved samples
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW); // Use LZW compression

        // Set bits per sample and sample format based on data type
        uint16_t bits_per_sample;
        uint16_t sample_format;

        switch (data_type) {
        case BufferDataType::UINT8:
            bits_per_sample = 8;
            sample_format = SAMPLEFORMAT_UINT;
            break;
        case BufferDataType::UINT16:
            bits_per_sample = 16;
            sample_format = SAMPLEFORMAT_UINT;
            break;
        case BufferDataType::UINT32:
            bits_per_sample = 32;
            sample_format = SAMPLEFORMAT_UINT;
            break;
        case BufferDataType::UINT64:
            bits_per_sample = 64;
            sample_format = SAMPLEFORMAT_UINT;
            break;
        case BufferDataType::FLOAT32:
            bits_per_sample = 32;
            sample_format = SAMPLEFORMAT_IEEEFP;
            break;
        case BufferDataType::FLOAT64:
            bits_per_sample = 64;
            sample_format = SAMPLEFORMAT_IEEEFP;
            break;
        default:
            TIFFClose(tiff);
            throw std::invalid_argument("Unsupported buffer data type for TIFF");
        }

        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
        TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, sample_format);

        // Set photometric interpretation based on number of channels
        uint16_t photometric;
        if (channels == 1) {
            photometric = PHOTOMETRIC_MINISBLACK; // Grayscale
        }
        else if (channels == 3 || channels == 4) {
            photometric = PHOTOMETRIC_RGB; // RGB or RGBA
        }
        else {
            photometric = PHOTOMETRIC_MINISBLACK; // Default for multi-channel
        }
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric);

        // Set alpha channel information if present
        if (write_alpha && image.hasAlpha()) {
            if (channels == 2) {
                // Grayscale + Alpha
                uint16_t extra_samples = EXTRASAMPLE_ASSOCALPHA;
                TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extra_samples);
            }
            else if (channels == 4) {
                // RGB + Alpha
                uint16_t extra_samples = EXTRASAMPLE_ASSOCALPHA;
                TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extra_samples);
            }
            else if (channels > 4) {
                // Multi-channel with alpha - mark the last channel as alpha
                std::vector<uint16_t> extra_samples(channels - 3, EXTRASAMPLE_UNSPECIFIED);
                extra_samples.back() = EXTRASAMPLE_ASSOCALPHA; // Last channel is alpha
                TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, static_cast<uint16_t>(extra_samples.size()), extra_samples.data());
            }
        }
        else if (channels > 3) {
            // Multi-channel without alpha - all extra channels are unspecified
            std::vector<uint16_t> extra_samples(channels - 3, EXTRASAMPLE_UNSPECIFIED);
            TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, static_cast<uint16_t>(extra_samples.size()), extra_samples.data());
        }

        // Set resolution (optional - defaults to 72 DPI)
        TIFFSetField(tiff, TIFFTAG_XRESOLUTION, 72.0);
        TIFFSetField(tiff, TIFFTAG_YRESOLUTION, 72.0);
        TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

        // Add software tag
        TIFFSetField(tiff, TIFFTAG_SOFTWARE, "VIRA Image Library");

        // Calculate bytes per sample and row
        size_t bytes_per_sample = (bits_per_sample + 7) / 8; // Round up to nearest byte
        size_t bytes_per_pixel = channels * bytes_per_sample;
        size_t bytes_per_row = static_cast<size_t>(resolution.x) * bytes_per_pixel;

        // Set row per strip (write entire image as one strip for simplicity)
        TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, static_cast<uint32_t>(resolution.y));

        // Write the image data
        const unsigned char* buffer_ptr = byte_buffer.data();

        for (uint32_t row = 0; row < static_cast<uint32_t>(resolution.y); ++row) {
            if (TIFFWriteScanline(tiff, const_cast<unsigned char*>(buffer_ptr + static_cast<size_t>(row) * bytes_per_row), row, static_cast<uint16_t>(0)) < 0) {
                TIFFClose(tiff);
                throw std::runtime_error("Failed to write TIFF scanline at row " + std::to_string(row));
            }
        }

        // Add custom fields for additional metadata
        std::string description = "Channels: " + std::to_string(channels) +
            ", Data type: ";

        switch (data_type) {
        case BufferDataType::UINT8: description += "8-bit unsigned integer"; break;
        case BufferDataType::UINT16: description += "16-bit unsigned integer"; break;
        case BufferDataType::UINT32: description += "32-bit unsigned integer"; break;
        case BufferDataType::UINT64: description += "64-bit unsigned integer"; break;
        case BufferDataType::FLOAT32: description += "32-bit float"; break;
        case BufferDataType::FLOAT64: description += "64-bit double"; break;
        default:
            TIFFClose(tiff);
            throw std::invalid_argument("Unsupported buffer data type for TIFF");
        }

        if (write_alpha && image.hasAlpha()) {
            description += ", Has alpha channel";
        }
        TIFFSetField(tiff, TIFFTAG_IMAGEDESCRIPTION, description.c_str());

        // Close the file
        TIFFClose(tiff);
    }

    void ImageInterface::writeMap(const fs::path& filepath, Image<float> image, std::vector<ColorRGB> colormap)
    {
        image.stretch();

        if (colormap.size() == 0) {
            vira::images::ImageInterface::write(filepath, image);
        }
        else {
            vira::images::Image<vira::ColorRGB> imageRGB = vira::images::colorMap(image, colormap);
            vira::images::ImageInterface::write(filepath, imageRGB, false);
        }
    };

    void ImageInterface::writeNormals(const fs::path& filepath, const Image<vec3<float>>& normals)
    {
        ImageInterface::write(filepath, formatNormals(normals));
    }

    void ImageInterface::writeIDs(const fs::path& filepath, const Image<size_t>& ids)
    {
        ImageInterface::write(filepath, idToRGB(ids));
    }

    void ImageInterface::writeVelocities(const fs::path& filepath, const Image<vec3<float>>& velocities)
    {
        ImageInterface::write(filepath, velocityToRGB(velocities));
    }
    

    // Helper function to create Image<float> from stb data
    Image<float> ImageInterface::createFloatImageFromBuffer(unsigned char* data, int width, int height, size_t channels, bool read_alpha)
    {
        Resolution resolution{ width, height };
        Image<float> image(resolution);

        const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);

        if (channels == 1) {
            // Grayscale image
            for (size_t i = 0; i < pixel_count; ++i) {
                float gray_value = static_cast<float>(data[i]) / 255.0f;
                image[i] = gray_value;
            }
        }
        else if (channels >= 3) {
            // RGB or RGBA image - convert to grayscale using luminance formula
            for (size_t i = 0; i < pixel_count; ++i) {
                float r = static_cast<float>(data[i * channels + 0]) / 255.0f;
                float g = static_cast<float>(data[i * channels + 1]) / 255.0f;
                float b = static_cast<float>(data[i * channels + 2]) / 255.0f;

                // Use standard luminance conversion: 0.299*R + 0.587*G + 0.114*B
                float gray_value = 0.299f * r + 0.587f * g + 0.114f * b;
                image[i] = gray_value;
            }
        }
        else if (channels == 2) {
            // Grayscale + Alpha
            for (size_t i = 0; i < pixel_count; ++i) {
                float gray_value = static_cast<float>(data[i * 2]) / 255.0f;
                image[i] = gray_value;
            }
        }

        // Handle alpha channel if requested and available
        if (read_alpha && (channels == 2 || channels == 4)) {
            std::vector<float> alphas(pixel_count);
            size_t alpha_channel = (channels == 2) ? 1 : 3;

            for (size_t i = 0; i < pixel_count; ++i) {
                alphas[i] = static_cast<float>(data[i * channels + alpha_channel]) / 255.0f;
            }

            image.setAlpha(alphas);
        }

        return image;
    }

    // Helper function to create Image<ColorRGB> from stb data
    Image<ColorRGB> ImageInterface::createRGBImageFromBuffer(unsigned char* data, int width, int height, size_t channels, bool read_alpha)
    {
        Resolution resolution{ width, height };
        Image<ColorRGB> image(resolution);

        const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);

        if (channels == 1) {
            // Grayscale - replicate to RGB
            for (size_t i = 0; i < pixel_count; ++i) {
                float gray_value = static_cast<float>(data[i]) / 255.0f;
                ColorRGB pixel;
                pixel[0] = gray_value;  // R
                pixel[1] = gray_value;  // G  
                pixel[2] = gray_value;  // B
                image[i] = pixel;
            }
        }
        else if (channels == 2) {
            // Grayscale + Alpha - replicate gray to RGB
            for (size_t i = 0; i < pixel_count; ++i) {
                float gray_value = static_cast<float>(data[i * 2]) / 255.0f;
                ColorRGB pixel;
                pixel[0] = gray_value;  // R
                pixel[1] = gray_value;  // G
                pixel[2] = gray_value;  // B
                image[i] = pixel;
            }
        }
        else if (channels >= 3) {
            // RGB or RGBA
            for (size_t i = 0; i < pixel_count; ++i) {
                ColorRGB pixel;
                pixel[0] = static_cast<float>(data[i * channels + 0]) / 255.0f;  // R
                pixel[1] = static_cast<float>(data[i * channels + 1]) / 255.0f;  // G
                pixel[2] = static_cast<float>(data[i * channels + 2]) / 255.0f;  // B
                image[i] = pixel;
            }
        }

        // Handle alpha channel if requested and available
        if (read_alpha && (channels == 2 || channels == 4)) {
            std::vector<float> alphas(pixel_count);
            size_t alpha_channel = (channels == 2) ? 1 : 3;

            for (size_t i = 0; i < pixel_count; ++i) {
                alphas[i] = static_cast<float>(data[i * channels + alpha_channel]) / 255.0f;
            }

            image.setAlpha(alphas);
        }

        return image;
    }

    // Read grayscale image from file
    Image<float> ImageInterface::readImage(const fs::path& filepath, bool read_alpha)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filepath.string().c_str(), &width, &height, &channels, 0);

        if (!data) {
            std::cout << "Failed to load image: " << filepath.string() << " - " << stbi_failure_reason() << "\n";
            // Return empty 1x1 image as fallback
            Resolution resolution{ 1, 1 };
            return Image<float>(resolution);
        }

        Image<float> result = createFloatImageFromBuffer(data, width, height, static_cast<size_t>(channels), read_alpha);
        stbi_image_free(data);

        return result;
    }

    // Read RGB image from file
    Image<ColorRGB> ImageInterface::readImageRGB(const fs::path& filepath, bool read_alpha)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filepath.string().c_str(), &width, &height, &channels, 0);

        if (!data) {
            std::cout << "Failed to load image: " << filepath.string() << " - " << stbi_failure_reason() << "\n";
            // Return empty 1x1 image as fallback
            Resolution resolution{ 1, 1 };
            return Image<ColorRGB>(resolution);
        }

        Image<ColorRGB> result = createRGBImageFromBuffer(data, width, height, static_cast<size_t>(channels), read_alpha);
        stbi_image_free(data);

        return result;
    }

    // Read grayscale image from memory buffer
    Image<float> ImageInterface::readImageFromMemory(const unsigned char* data, size_t dataSize, std::string format, bool read_alpha)
    {
        (void)format; // Unused as STB automatically detects format
        int width, height, channels;
        unsigned char* image_data = stbi_load_from_memory(data, static_cast<int>(dataSize), &width, &height, &channels, 0);

        if (!image_data) {
            std::cout << "Failed to load image from memory buffer - " << stbi_failure_reason() << "\n";
            // Return empty 1x1 image as fallback
            Resolution resolution{ 1, 1 };
            return Image<float>(resolution);
        }

        Image<float> result = createFloatImageFromBuffer(image_data, width, height, static_cast<size_t>(channels), read_alpha);
        stbi_image_free(image_data);

        return result;
    }

    // Read RGB image from memory buffer
    Image<ColorRGB> ImageInterface::readImageRGBFromMemory(const unsigned char* data, size_t dataSize, std::string format, bool read_alpha)
    {
        (void)format; // Unused as STB automatically detects format
        int width, height, channels;
        unsigned char* image_data = stbi_load_from_memory(data, static_cast<int>(dataSize), &width, &height, &channels, 0);

        if (!image_data) {
            std::cout << "Failed to load image from memory buffer - " << stbi_failure_reason() << "\n";
            // Return empty 1x1 image as fallback
            Resolution resolution{ 1, 1 };
            return Image<ColorRGB>(resolution);
        }

        Image<ColorRGB> result = createRGBImageFromBuffer(image_data, width, height, static_cast<size_t>(channels), read_alpha);
        stbi_image_free(image_data);

        return result;
    }
};