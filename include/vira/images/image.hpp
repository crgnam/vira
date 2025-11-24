#ifndef VIRA_IMAGES_IMAGE_HPP
#define VIRA_IMAGES_IMAGE_HPP

#include <cstddef>
#include <stdexcept>
#include <vector>
#include <array>

#include "vira/vec.hpp"
#include "vira/images/image_pixel.hpp"
#include "vira/images/resolution.hpp"

namespace vira::images {
    enum ROIType {
        ROI_CORNERS,
        ROI_CORNER_DIM,
        ROI_CENTER_DIM
    };

    class ROI {
    public:
        ROI(int x, int y, int xx, int yy, ROIType inputType = ROI_CORNERS)
        {
            switch (inputType) {
            case ROIType::ROI_CORNERS:
                x0 = x;
                y0 = y;

                x1 = xx;
                y1 = yy;
                break;

            case ROIType::ROI_CORNER_DIM:
                x0 = x;
                y0 = y;

                x1 = x0 + xx;
                y1 = y0 + yy;
                break;

            case ROIType::ROI_CENTER_DIM:
                x0 = x - xx / 2;
                y0 = y - yy / 2;

                x1 = x + xx / 2;
                y1 = y + yy / 2;
                break;

            default:
                throw std::invalid_argument("Invalid ROI type");
            }
        }

        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;

        int width() const { return x1 - x0; }
        int height() const { return y1 - y0; }

        int centerX() const { return (x0 + x1) / 2; }
        int centerY() const { return (y0 + y1) / 2; }
    };

    template <IsPixel T>
    class Image {
    public:
        Image() = default;
        Image(Resolution resolution, T defaultValue = T{ 0 });
        Image(Resolution resolution, std::vector<T> data);
        Image(Resolution resolution, std::vector<T> data, std::vector<float> alpha);
        Image(const Image& original);

        // Default assignment operator required explicitly by "Rule of Three"
        Image& operator=(const Image& other) = default;

        // Getters:
        void* data() { return data_.data(); }
        size_t size() const { return data_.size(); }
        Resolution resolution() { return resolution_; }
        Resolution resolution() const { return resolution_; }
        const std::vector<T>& getVector() const { return data_; }
        std::vector<T>& getVector() { return const_cast<std::vector<T>&>(const_cast<const Image<T>*>(this)->getVector()); }

        // Indexing operator overloads:
        const T& operator[] (size_t idx) const;
        T& operator[] (size_t idx) { return const_cast<T&>(const_cast<const Image<T>*>(this)->operator[](idx)); }

        const T& operator() (int i, int j) const;
        T& operator() (int i, int j) { return const_cast<T&>(const_cast<const Image<T>*>(this)->operator()(i, j)); }

        const T& operator() (Pixel pixel) const;
        T& operator() (Pixel pixel) { return const_cast<T&>(const_cast<const Image<T>*>(this)->operator()(pixel)); }

        const T& operator() (size_t i, size_t j) const { return this->operator()(static_cast<int>(i), static_cast<int>(j)); }
        T& operator() (size_t i, size_t j) { return const_cast<T&>(const_cast<const Image<T>*>(this)->operator()(i, j)); }


        // Channel accessors:
        std::vector<float> extractChannel(size_t channelIndex) const;
        Image<float> extractChannelImage(size_t channelIndex) const;
        void setChannel(size_t channelIndex, const std::vector<float>& channelData);

        // Addition operator overloads:
        template <typename T2>
        Image<T>& operator+= (const T2& rhs);
        Image<T>& operator+= (const Image<T>& rhs);
        template <typename T2>
        Image<T> operator+ (const T2& rhs) const;
        Image<T> operator+ (const Image<T>& rhs) const;

        // Subtraction operator overloads:
        template <typename T2>
        Image<T>& operator-= (const T2& rhs);
        Image<T>& operator-= (const Image<T>& rhs);
        template <typename T2>
        Image<T> operator- (const T2& rhs) const;
        Image<T> operator- (const Image<T>& rhs) const;

        // Multiplication operator overloads:
        template <typename T2>
        Image<T>& operator*= (const T2& rhs);
        Image<T>& operator*= (const Image<T>& rhs);
        template <typename T2>
        Image<T> operator* (const T2& rhs) const;
        Image<T> operator* (const Image<T>& rhs) const;

        // Division operator overloads:
        template <typename T2>
        Image<T>& operator/= (const T2& rhs);
        Image<T>& operator/= (const Image<T>& rhs);
        template <typename T2>
        Image<T> operator/ (const T2& rhs) const;
        Image<T> operator/ (const Image<T>& rhs) const;

        size_t getOutputChannels(bool includeAlpha = true) const;
        std::vector<unsigned char> toBuffer(bool includeAlpha, BufferDataType data_type) const;

        // UV-sampling:
        T sampleUVs(const UV& uv) const;
        T sampleUVsNoWrap(const UV& uv) const;
        T interpolatePixel(const vira::Pixel& pixel) const;

        // Meta-data methods:
        size_t numChannels() const;
        float min() const;
        float max() const;
        std::array<float, 2> minmax() const;

        // Alpha-methods:
        const float& alpha(int i, int j) const;
        float& alpha(int i, int j) { return const_cast<float&>(const_cast<const Image<T>*>(this)->alpha(i, j)); }
        const float& alpha(size_t idx) const;
        float& alpha(size_t idx) { return const_cast<float&>(const_cast<const Image<T>*>(this)->alpha(idx)); }

        void setAlpha(Image<float> newAlpha);
        void setAlpha(std::vector<float> newAlpha);
        const std::vector<float>& getAlpha() const;
        Image<float> getAlphaImage() { return Image<float>(resolution_, alpha_); }
        std::vector<float>& getAlpha() { return const_cast<std::vector<float>&>(const_cast<const Image<T>*>(this)->getAlpha()); }
        bool hasAlpha() const { return hasAlpha_; }

        // Modifer methods:
        void fillMissing(T fillValue);
        void fillMissing();
        void crop(const ROI& roi);
        void padBounds(int xpad, int ypad);
        void resize(float scale);
        void resize(Resolution newResolution);
        void stretch(float newMin = 0.f, float newMax = 1.f);
        void convolve(Image<T>& kernel, bool applyToAlpha = true);

        void addImage(const Image<T>& addingImage, Pixel center);
        void addImage(const Image<T>& addingImage, Pixel center, const ROI& boundingROI);

        void reset(T resetValue = T{ 0 });
        void clear();

    private:
        Resolution resolution_{ 0, 0 };
        T defaultValue_{ 0 };

        std::vector<T> data_;
        std::vector<float> alpha_;
        bool hasAlpha_ = false;

        
        std::vector<float> convolveChannelFFT(
            const std::vector<float>& imageChannel,
            const std::vector<float>& kernelChannel,
            const Resolution& kernelRes) const;
        
        void convolveSpatial(const Image<T>& kernel, bool applyToAlpha);

        float getMagnitude(T val) const;

        template <IsPixel T2>
        friend class Image;
    };

    // Right-hand-side operator overloads:
    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator+ (T2 lhs, const Image<T>& rhs);

    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator- (T2 lhs, const Image<T>& rhs);

    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator* (T2 lhs, const Image<T>& rhs);

    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator/ (T2 lhs, const Image<T>& rhs);
};

#include "implementation/images/image.ipp"

#endif
