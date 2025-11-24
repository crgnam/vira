#include <stdexcept>
#include <array>
#include <cstdint>
#include <limits>
#include <algorithm>

#include "glm/gtc/color_space.hpp"

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"
#include "vira/utils/hash_utils.hpp"
#include "vira/images/color_map.hpp"

namespace vira::images {
    // ======================================== //
    // === Channel and Colorspace Modifiers === //
    // ======================================== //
    ColorRGB linearToSRGB_val(ColorRGB linearRGB)
    {
        // Extract RGB values from ColorRGB
        glm::vec3 linear_color(linearRGB[0], linearRGB[1], linearRGB[2]);

        // Convert using GLM
        glm::vec3 srgb_color = glm::convertLinearToSRGB(linear_color);

        // Create new ColorRGB with converted values
        ColorRGB result;
        result[0] = srgb_color.r;
        result[1] = srgb_color.g;
        result[2] = srgb_color.b;

        return result;
    };

    Image<ColorRGB> linearToSRGB(Image<ColorRGB> linearImage)
    {
        Image<ColorRGB> sRGBImage = linearImage; // Copy the image

        Resolution resolution = linearImage.resolution();
        const size_t pixel_count = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);

        // Convert each pixel
        for (size_t i = 0; i < pixel_count; ++i) {
            const ColorRGB& linear_pixel = linearImage[i];

            // Extract RGB values
            glm::vec3 linear_color(linear_pixel[0], linear_pixel[1], linear_pixel[2]);

            // Convert using GLM
            glm::vec3 srgb_color = glm::convertLinearToSRGB(linear_color);

            // Store converted values
            ColorRGB srgb_pixel;
            srgb_pixel[0] = srgb_color.r;
            srgb_pixel[1] = srgb_color.g;
            srgb_pixel[2] = srgb_color.b;

            sRGBImage[i] = srgb_pixel;
        }

        // Handle alpha channel if present
        if (linearImage.hasAlpha()) {
            sRGBImage.setAlpha(linearImage.getAlpha()); // Alpha doesn't need conversion
        }

        return sRGBImage;
    };

    ColorRGB sRGBToLinear_val(ColorRGB sRGB)
    {
        // Extract RGB values from ColorRGB
        glm::vec3 srgb_color(sRGB[0], sRGB[1], sRGB[2]);

        // Convert using GLM
        glm::vec3 linear_color = glm::convertSRGBToLinear(srgb_color);

        // Create new ColorRGB with converted values
        ColorRGB result;
        result[0] = linear_color.r;
        result[1] = linear_color.g;
        result[2] = linear_color.b;

        return result;
    };

    Image<ColorRGB> sRGBToLinear(Image<ColorRGB> sRGBImage)
    {
        Image<ColorRGB> linearImage = sRGBImage; // Copy the image

        Resolution resolution = sRGBImage.resolution();
        const size_t pixel_count = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);

        // Convert each pixel
        for (size_t i = 0; i < pixel_count; ++i) {
            const ColorRGB& srgb_pixel = sRGBImage[i];

            // Extract RGB values
            glm::vec3 srgb_color(srgb_pixel[0], srgb_pixel[1], srgb_pixel[2]);

            // Convert using GLM
            glm::vec3 linear_color = glm::convertSRGBToLinear(srgb_color);

            // Store converted values
            ColorRGB linear_pixel;
            linear_pixel[0] = linear_color.r;
            linear_pixel[1] = linear_color.g;
            linear_pixel[2] = linear_color.b;

            linearImage[i] = linear_pixel;
        }

        // Handle alpha channel if present
        if (sRGBImage.hasAlpha()) {
            linearImage.setAlpha(sRGBImage.getAlpha()); // Alpha doesn't need conversion
        }

        return linearImage;
    };


    Image<ColorRGB> monoToRGB(const Image<float>& image)
    {
        Image<ColorRGB> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (int X = 0; X < image.resolution().x; X++) {
            for (int Y = 0; Y < image.resolution().y; Y++) {
                float val = image(X, Y);
                ColorRGB pixel{ val, val, val };
                outImage(X, Y) = pixel;
            }
        }

        return outImage;
    };


    Image<ColorRGB> idToRGB(const Image<size_t>& image)
    {
        Image<ColorRGB> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            size_t id = image[i];

            if (id == std::numeric_limits<size_t>::max()) {
                ColorRGB pixel{ 0 };
                outImage[i] = pixel;
            }
            else {
                ColorRGB pixel = vira::utils::idToColor(id);
                outImage[i] = pixel;
            }
        }
        return outImage;
    };

    Image<ColorRGB> velocityToRGB(const Image<vec3<float>>& velocityImage)
    {
        Image<ColorRGB> outImage(velocityImage.resolution());
        outImage.setAlpha(velocityImage.getAlpha());

        // Get magnitude range using your Image class functions
        //float minMagnitude = velocityImage.min();
        float maxMagnitude = velocityImage.max();

        for (size_t i = 0; i < velocityImage.size(); ++i) {
            vec3<float> velocity = velocityImage[i];

            ColorRGB color;
            if (maxMagnitude > 0.0f) {
                // This preserves direction while ensuring values fit in [-1, 1]
                color[0] = (velocity[0] / maxMagnitude) * 0.5f + 0.5f;
                color[1] = (velocity[1] / maxMagnitude) * 0.5f + 0.5f;
                color[2] = (velocity[2] / maxMagnitude) * 0.5f + 0.5f;
            }
            else {
                color = { 0.5f, 0.5f, 0.5f };
            }

            outImage[i] = color;
        }

        return outImage;
    };

    Image<ColorRGB> triangleSizeToRGB(const Image<float>& image, float triangleSize)
    {
        Image<ColorRGB> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            float size = image[i];
            if (std::isinf(size)) {
                outImage[i] = ColorRGB{ 0.f };
            }
            else {
                if (size <= triangleSize) {
                    outImage[i] = ColorRGB{ 0.,0.,1. };
                }
                else if (size > triangleSize) {
                    outImage[i] = ColorRGB{ 1.,0.,0. };
                }

                if (size <= (triangleSize / 2.f)) {
                    outImage[i] = ColorRGB{ 0.,1.,0. };
                }

            }
        }
        return outImage;
    };

    Image<ColorRGB> colorMap(Image<float> image, std::vector<ColorRGB> colormap)
    {
        image.stretch(0.f, 1.f);

        if (colormap.size() == 0) {
            Image<ColorRGB> outputImage = monoToRGB(image);
            return outputImage;
        }
        else {
            Image<ColorRGB> outputImage(image.resolution());
            std::vector<float> key = vira::linspace<float>(0.f, 1.f, colormap.size());

            for (size_t i = 0; i < image.size(); ++i) {
                outputImage[i] = colorMap(image[i], colormap, key);
            }

            return outputImage;
        }
    };

    Image<ColorRGB> formatNormals(const Image<vec3<float>>& image)
    {
        Image<ColorRGB> output(image.resolution());
        for (size_t i = 0; i < image.size(); ++i) {
            output[i] = ColorRGB{
                (image[i][0] + 1.f) * 0.5f,
                (image[i][1] + 1.f) * 0.5f,
                (image[i][2] + 1.f) * 0.5f
            };
        }
        return output;
    };


    // ========================= //
    // === Channel Modifiers === //
    // ========================= //
    template <IsSpectral TSpectral>
    std::vector<Image<float>> channelSplit(const Image<TSpectral>& image)
    {
        std::vector<Image<float>> output(TSpectral::size(), Image<float>(image.resolution()));

        for (size_t i = 0; i < image.size(); ++i) {
            for (size_t j = 0; j < output.size(); ++j) {
                output[j][i] = image[i][j];
            }
        }

        return output;
    };

    template <IsSpectral TSpectral>
    Image<TSpectral> channelMerge(const std::vector<Image<float>>& channels)
    {
        if (channels.size() != TSpectral::size() && channels.size() != 1) {
            throw std::runtime_error("Must provide all channels or 1 channel to be duplicated for Image<TSpectral>");
        }

        Resolution resolution = channels[0].resolution();
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i].resolution() != resolution) {
                throw std::runtime_error("All provided channels must have the same resolution!");
            }
        }

        Image<TSpectral> output(resolution);
        if (channels.size() == 1) {
            for (size_t i = 0; i < output.size(); ++i) {
                output[i] = TSpectral{ channels[0][i] };
            }
        }
        else {
            for (size_t i = 0; i < output.size(); ++i) {
                for (size_t j = 0; j < channels.size(); ++j) {
                    output[i][j] = channels[j][i];
                }
            }
        }

        return output;
    };

    template <IsSpectral TSpectral, IsFloatImage... Channels> requires ValidChannels<TSpectral, Channels...>
    Image<TSpectral> channelMerge(Channels... channels)
    {
        std::vector<Image<float>> channelsVector{ channels... };
        return channelMerge<TSpectral>(channelsVector);
    };


    template <IsSpectral TSpectral>
    Image<float> spectralToMono(const Image<TSpectral>& image)
    {
        Image<float> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            outImage[i] = image[i].magnitude();
        }

        return outImage;
    };

    template <IsSpectral TSpectral>
    Image<ColorRGB> spectralToRGB(const Image<TSpectral>& image, std::function<ColorRGB(const TSpectral&)>specToRGB)
    {
        Image<ColorRGB> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            outImage[i] = specToRGB(image[i]);
        }

        return outImage;
    };


    template <IsSpectral TSpectral>
    Image<TSpectral> rgbToSpectral(const Image<TSpectral>& image, std::function<TSpectral(const ColorRGB&)>colorConvert)
    {
        Image<TSpectral> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            outImage[i] = colorConvert(image[i]);
        }

        return outImage;
    };

    template <IsSpectral TSpectral1, IsSpectral TSpectral2>
    Image<TSpectral2> spectralConvert(const Image<TSpectral1>& image, std::function<TSpectral2(const TSpectral1&)>colorConvert)
    {
        Image<TSpectral2> outImage(image.resolution());
        outImage.setAlpha(image.getAlpha());

        for (size_t i = 0; i < image.size(); ++i) {
            outImage[i] = colorConvert(image[i]);
        }

        return outImage;
    };



    // ========================= //
    // === Generic Utilities === //
    // ========================= //
    template <IsPixel T>
    vira::geometry::IndexBuffer imageToIndexBuffer(const Image<T>& image)
    {
        Resolution resolution = image.resolution();
        size_t numVertices = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);
        vira::geometry::IndexBuffer indexBuffer(3 * 2 * numVertices);

        size_t idx = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(resolution.x - 1); i++) {
            for (uint32_t j = 0; j < static_cast<uint32_t>(resolution.y - 1); j++) {
                // Define the first triangle of the quad:
                uint32_t f0 = i + (j + 1) * static_cast<uint32_t>(resolution.x);
                uint32_t f1 = i + 1 + j * static_cast<uint32_t>(resolution.x);
                uint32_t f2 = i + j * static_cast<uint32_t>(resolution.x);

                // Only include new faces if vertexBuffer are valid:
                if (!std::isinf(image[f0]) &&
                    !std::isinf(image[f1]) &&
                    !std::isinf(image[f2]))
                {
                    indexBuffer[idx + 0] = f0;
                    indexBuffer[idx + 1] = f1;
                    indexBuffer[idx + 2] = f2;
                    idx = idx + 3;
                }

                // Define the second triangle of the quad:
                f0 = i + 1 + j * static_cast<uint32_t>(resolution.x);
                f1 = i + (j + 1) * static_cast<uint32_t>(resolution.x);
                f2 = i + 1 + (j + 1) * static_cast<uint32_t>(resolution.x);

                // Only include new faces if vertexBuffer are valid:
                if (!std::isinf(image[f0]) &&
                    !std::isinf(image[f1]) &&
                    !std::isinf(image[f2]))
                {
                    indexBuffer[idx + 0] = f0;
                    indexBuffer[idx + 1] = f1;
                    indexBuffer[idx + 2] = f2;
                    idx = idx + 3;
                }

            }
        }

        // Remove the invalid faces:
        indexBuffer.erase(indexBuffer.begin() + static_cast<std::ptrdiff_t>(idx), indexBuffer.end());

        return indexBuffer;
    };

    template <IsPixel T>
    std::vector<vira::vec2<size_t>> getValidPerimeter(const Image<T>& image)
    {
        Resolution resolution = image.resolution();

        Image<uint8_t> edges(resolution, false);

        // Search for first valid point in each row:
        size_t numberOfEdgeVertices = 0;
        for (int i = 0; i < resolution.x; ++i) {
            // Look forward in the row:
            for (int j = 0; j < resolution.y; ++j) {
                if (!std::isinf(image(i, j))) {
                    edges(i, j) = true;
                    numberOfEdgeVertices++;
                    break;
                }
            }

            // Look backward in the row:
            for (int j = resolution.y - 1; j >= 0; --j) {
                if (!std::isinf(image(i, j))) {
                    edges(i, j) = true;
                    numberOfEdgeVertices++;
                    break;
                }
            }
        }

        // Search for the first valid point in each column:
        for (int j = 0; j < resolution.y; ++j) {
            // Look down in the column:
            for (int i = 0; i < resolution.x; ++i) {
                if (!std::isinf(image(i, j))) {
                    edges(i, j) = true;
                    numberOfEdgeVertices++;
                    break;
                }
            }

            // Look up in the column:
            for (int i = resolution.x - 1; i >= 0; --i) {
                if (!std::isinf(image(i, j))) {
                    edges(i, j) = true;
                    numberOfEdgeVertices++;
                    break;
                }
            }
        }

        if (numberOfEdgeVertices == 0) {
            throw std::runtime_error("The processed RasterMap is invalid");
        }
        std::vector<vec2<size_t>> edgeVertices(numberOfEdgeVertices);

        // Obtain edge vertices:
        size_t idx = 0;
        for (int i = 0; i < resolution.x; i++) {
            for (int j = 0; j < resolution.y; j++) {
                if (edges(i, j)) {
                    edgeVertices[idx] = vec2<size_t>{ i,j };
                    idx++;
                }
            }
        }
        edgeVertices.erase(edgeVertices.begin() + static_cast<std::ptrdiff_t>(idx), edgeVertices.end());

        return edgeVertices;
    };

    template <IsInteger T>
    Image<T> floatToFixed(const Image<float>& image, std::array<float, 2> minMax, bool clamp)
    {
        Resolution resolution = image.resolution();
        Image<T> newImage(resolution);

        for (size_t i = 0; i < image.size(); i++) {
            float value = image[i];
            if (std::isinf(value)) {
                newImage[i] = 0;
            }
            else {
                if (clamp) {
                    value = std::clamp(value, 0.f, 1.f);
                }
                value = (value - minMax[0]) / (minMax[1] - minMax[0]);
                constexpr float m = static_cast<float>(std::numeric_limits<T>::max());
                newImage[i] = static_cast<T>(((m - 1.f) * value) + 1.f);
            }
        }

        return newImage;
    };

    template <IsInteger T>
    Image<float> fixedToFloat(const Image<T>& image, std::array<float, 2> minMax)
    {
        Resolution resolution = image.resolution();
        Image<float> newImage(resolution);

        for (size_t i = 0; i < newImage.size(); i++) {
            T value = image[i];
            if (value == 0) {
                newImage[i] = std::numeric_limits<float>::infinity();
            }
            else {
                constexpr float m = static_cast<float>(std::numeric_limits<T>::max());
                newImage[i] = static_cast<float>(value) / (m - 1.f);
                newImage[i] = ((minMax[1] - minMax[0]) * newImage[i]) + minMax[0];
            }
        }

        return newImage;
    };

    std::vector<std::array<int, 4>> computeChunks(vira::images::Resolution resolution, int64_t maxAllowedPixels)
    {
        int64_t numPixels = static_cast<int64_t>(resolution.x * resolution.y);
        std::vector<std::array<int, 4>> chunks;
        if (numPixels > maxAllowedPixels) {
            // Calculate the total number of chunks needed
            int numChunks = static_cast<int>((numPixels + maxAllowedPixels - 1) / maxAllowedPixels);

            // Calculate the aspect ratio of the original image
            double aspectRatio = static_cast<double>(resolution.x) / static_cast<double>(resolution.y);

            // Calculate the number of chunks in each dimension
            int numChunksX = static_cast<int>(std::sqrt(static_cast<double>(numChunks) * aspectRatio));
            int numChunksY = (numChunks + numChunksX - 1) / numChunksX;

            // Adjust numChunksX if necessary
            if (numChunksX * numChunksY < numChunks) {
                numChunksX++;
            }

            // Calculate the size of each chunk
            int chunkWidth = (resolution.x + numChunksX - 1) / numChunksX;
            int chunkHeight = (resolution.y + numChunksY - 1) / numChunksY;

            // Generate crop bounds with overlap
            for (int y = 0; y < numChunksY; ++y) {
                for (int x = 0; x < numChunksX; ++x) {
                    int startX = x * chunkWidth;
                    int startY = y * chunkHeight;

                    // Calculate width and height with overlap
                    int width = std::min(chunkWidth, resolution.x - startX);
                    int height = std::min(chunkHeight, resolution.y - startY);

                    // Add 1-pixel overlap on the right edge (except for the last column)
                    if (x < numChunksX - 1 && startX + width < resolution.x) {
                        width += 1;
                    }

                    // Add 1-pixel overlap on the bottom edge (except for the last row)
                    if (y < numChunksY - 1 && startY + height < resolution.y) {
                        height += 1;
                    }

                    chunks.push_back({ startX, startY, width, height });
                }
            }
        }
        else {
            chunks.push_back(std::array<int, 4>{0, 0, resolution.x, resolution.y});
        }

        return chunks;
    };
};