#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include <array>
#include <fftw3.h>
#include <complex>
#include <memory>
#include <cstring>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/debug.hpp"
#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image_pixel.hpp"
#include "vira/images/interfaces/image_interface.hpp"
#include "vira/utils/valid_value.hpp"

namespace vira::images {
    // ======================== //
    // === Local FFT Helper === //
    // ======================== //
    
    // RAII wrapper for FFTW plans and memory

    static std::mutex fftw_plan_mutex;
    class FFTWWrapper {
    public:
        FFTWWrapper(int width, int height)
            : width_(width), height_(height), size_(static_cast<size_t>(width) * static_cast<size_t>(height)) {

            // Allocate aligned memory for FFTW
            real_in_ = fftwf_alloc_real(size_);
            complex_out_ = fftwf_alloc_complex(size_);
            complex_in_ = fftwf_alloc_complex(size_);
            real_out_ = fftwf_alloc_real(size_);

            // CRITICAL: Protect plan creation with mutex
            {
                std::lock_guard<std::mutex> lock(fftw_plan_mutex);
                forward_plan_ = fftwf_plan_dft_r2c_2d(height, width, real_in_, complex_out_, FFTW_ESTIMATE);
                inverse_plan_ = fftwf_plan_dft_c2r_2d(height, width, complex_in_, real_out_, FFTW_ESTIMATE);
            }
        }

        ~FFTWWrapper() {
            if (forward_plan_) fftwf_destroy_plan(forward_plan_);
            if (inverse_plan_) fftwf_destroy_plan(inverse_plan_);
            if (real_in_) fftwf_free(real_in_);
            if (complex_out_) fftwf_free(complex_out_);
            if (complex_in_) fftwf_free(complex_in_);
            if (real_out_) fftwf_free(real_out_);
        }

        // Non-copyable, movable
        FFTWWrapper(const FFTWWrapper&) = delete;
        FFTWWrapper& operator=(const FFTWWrapper&) = delete;
        FFTWWrapper(FFTWWrapper&&) = default;
        FFTWWrapper& operator=(FFTWWrapper&&) = default;

        float* real_in_;
        fftwf_complex* complex_out_;
        fftwf_complex* complex_in_;
        float* real_out_;
        fftwf_plan forward_plan_;
        fftwf_plan inverse_plan_;
        int width_, height_;
        size_t size_;
    };


    // ========================== //
    // === Image Constructors === //
    // ========================== //
    template <IsPixel T>
    Image<T>::Image(Resolution resolution, T defaultValue) :
        resolution_{ resolution }, defaultValue_{ defaultValue }
    {
        this->data_ = std::vector<T>(static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y), defaultValue_);
    };

    template <IsPixel T>
    Image<T>::Image(Resolution resolution, std::vector<T> data) :
        resolution_{ resolution }, data_{ data }
    {
        size_t specifiedSize = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);
        if (data_.size() != specifiedSize) {
            throw std::runtime_error(std::to_string(data_.size()) + " pixel values were given, but the image was specified to contain " + std::to_string(specifiedSize));
        }
    };

    template <IsPixel T>
    Image<T>::Image(Resolution resolution, std::vector<T> data, std::vector<float> alpha) :
        resolution_{ resolution }, data_{ data }, alpha_{ alpha }
    {
        size_t specifiedSize = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);
        if (data_.size() != specifiedSize) {
            throw std::runtime_error(std::to_string(data_.size()) + " pixel values were given, but the image was specified to contain " + std::to_string(specifiedSize));
        }

        if (data_.size() != alpha_.size()) {
            throw std::runtime_error("The provided pixel and alpha data do not have the same size ( " +
                std::to_string(data_.size()) + " and " + std::to_string(alpha.size()) + " pixels given respectively)");
        }
        if (alpha_.size() != 0) {
            this->hasAlpha_ = true;
        }
    };


    template <IsPixel T>
    Image<T>::Image(const Image<T>& original) :
        resolution_{ original.resolution_ }, defaultValue_{ original.defaultValue_ }, data_{ original.data_ },
        alpha_{ original.alpha_ }, hasAlpha_{ original.hasAlpha_ }
    {

    };


    // =================================== //
    // === Indexing operator overloads === //
    // =================================== //
    template <IsPixel T>
    const T& Image<T>::operator[] (size_t idx) const
    {
        vira::debug::check_1d_bounds(idx, data_.size());
        return this->data_[idx];
    };

    template <IsPixel T>
    const T& Image<T>::operator() (int i, int j) const
    {
        vira::debug::check_2d_bounds(i, j, resolution_.x, resolution_.y);
        size_t linearIndex = static_cast<size_t>(i) + (static_cast<size_t>(j) * static_cast<size_t>(resolution_.x));

        return this->data_[linearIndex];
    };

    template <IsPixel T>
    const T& Image<T>::operator() (Pixel pixel) const
    {
        return this->operator()(static_cast<int>(pixel.x), static_cast<int>(pixel.y));
    };


    // ======================== //
    // === Channel Accessor === //
    // ======================== //
    template <IsPixel T>
    std::vector<float> Image<T>::extractChannel(size_t channelIndex) const {
        constexpr size_t numChannels = []() {
            if constexpr (IsVec<T>) {
                return static_cast<size_t>(T::length());
            }
            else if constexpr (IsSpectral<T>) {
                return T::size();
            }
            else {
                return static_cast<size_t>(1);
            }
            }();

        std::vector<float> channel(data_.size());

        if constexpr (numChannels == 1) {
            // Scalar types - direct copy with cast
            for (size_t i = 0; i < data_.size(); ++i) {
                if constexpr (IsSpectral<T>) {
                    channel[i] = data_[i][0];
                }
                else if constexpr (IsFloatingVec<T>) {
                    channel[i] = data_[i][0];
                }
                else {
                    channel[i] = static_cast<float>(data_[i]);
                }
            }
        }
        else {
            // Multi-channel types - extract specific channel
            for (size_t i = 0; i < data_.size(); ++i) {
                channel[i] = static_cast<float>(data_[i][channelIndex]);
            }
        }

        return channel;
    }

    template <IsPixel T>
    Image<float> Image<T>::extractChannelImage(size_t channelIndex) const {
        std::vector<float> channelData = extractChannel(channelIndex);
        Image<float> output(resolution_, channelData);
        return output;
    }

    template <IsPixel T>
    void Image<T>::setChannel(size_t channelIndex, const std::vector<float>& channelData) {
        constexpr size_t numChannels = []() {
            if constexpr (IsVec<T>) {
                return static_cast<size_t>(T::length());
            }
            else if constexpr (IsSpectral<T>) {
                return T::size();
            }
            else {
                return static_cast<size_t>(1);
            }
            }();

        if constexpr (numChannels == 1) {
            // Scalar types - direct assignment with cast
            for (size_t i = 0; i < data_.size(); ++i) {
                data_[i] = static_cast<T>(channelData[i]);
            }
        }
        else {
            // Multi-channel types - set specific channel
            for (size_t i = 0; i < data_.size(); ++i) {
                data_[i][channelIndex] = channelData[i];
            }
        }
    }


    // =================================== //
    // === Addition operator overloads === //
    // =================================== //
    template <IsPixel T>
    template <typename T2>
    Image<T>& Image<T>::operator+= (const T2& rhs)
    {
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] += static_cast<T>(rhs);
        }
        return *this;
    };

    template <IsPixel T>
    Image<T>& Image<T>::operator+= (const Image<T>& rhs)
    {
        if (rhs.resolution() != this->resolution()) {
            throw std::runtime_error("Images must have the same resolution to perform arithmetic");
        }
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] += static_cast<T>(rhs[i]);
        }
        return *this;
    }

    template <IsPixel T>
    template <typename T2>
    Image<T> Image<T>::operator+ (const T2& rhs) const
    {
        Image<T> output = *this;
        output += rhs;
        return output;
    };

    template <IsPixel T>
    Image<T> Image<T>::operator+ (const Image<T>& rhs) const
    {
        Image<T> output = *this;
        output += rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator+ (T2 lhs, const Image<T>& rhs)
    {
        return rhs + lhs;
    }


    // ====================================== //
    // === Subtraction operator overloads === //
    // ====================================== //
    template <IsPixel T>
    template <typename T2>
    Image<T>& Image<T>::operator-= (const T2& rhs)
    {
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] -= static_cast<T>(rhs);
        }
        return *this;
    };

    template <IsPixel T>
    Image<T>& Image<T>::operator-= (const Image<T>& rhs)
    {
        if (rhs.resolution() != this->resolution()) {
            throw std::runtime_error("Images must have the same resolution to perform arithmetic");
        }
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] -= static_cast<T>(rhs[i]);
        }
        return *this;
    }

    template <IsPixel T>
    template <typename T2>
    Image<T> Image<T>::operator- (const T2& rhs) const
    {
        Image<T> output = *this;
        output -= rhs;
        return output;
    };

    template <IsPixel T>
    Image<T> Image<T>::operator- (const Image<T>& rhs) const
    {
        Image<T> output = *this;
        output -= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator- (T2 lhs, const Image<T>& rhs)
    {
        Image<T> result(rhs.resolution());
        for (size_t i = 0; i < rhs.size(); ++i) {
            result[i] = static_cast<T>(lhs) - rhs[i];
        }
        return result;
    }


    // ========================================= //
    // === Multiplication operator overloads === //
    // ========================================= //
    template <IsPixel T>
    template <typename T2>
    Image<T>& Image<T>::operator*= (const T2& rhs)
    {
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] *= static_cast<T>(rhs);
        }
        return *this;
    };

    template <IsPixel T>
    Image<T>& Image<T>::operator*= (const Image<T>& rhs)
    {
        if (rhs.resolution() != this->resolution()) {
            throw std::runtime_error("Images must have the same resolution to perform arithmetic");
        }
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] *= static_cast<T>(rhs[i]);
        }
        return *this;
    }

    template <IsPixel T>
    template <typename T2>
    Image<T> Image<T>::operator* (const T2& rhs) const
    {
        Image<T> output = *this;
        output *= rhs;
        return output;
    };

    template <IsPixel T>
    Image<T> Image<T>::operator* (const Image<T>& rhs) const
    {
        Image<T> output = *this;
        output *= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator* (T2 lhs, const Image<T>& rhs)
    {
        return rhs * lhs;  // Delegate to member function
    }


    // =================================== //
    // === Division operator overloads === //
    // =================================== //
    template <IsPixel T>
    template <typename T2>
    Image<T>& Image<T>::operator/= (const T2& rhs)
    {
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] /= static_cast<T>(rhs);
        }
        return *this;
    };

    template <IsPixel T>
    Image<T>& Image<T>::operator/= (const Image<T>& rhs)
    {
        if (rhs.resolution() != this->resolution()) {
            throw std::runtime_error("Images must have the same resolution to perform arithmetic");
        }
        for (size_t i = 0; i < this->size(); ++i) {
            this->data_[i] /= static_cast<T>(rhs[i]);
        }
        return *this;
    }

    template <IsPixel T>
    template <typename T2>
    Image<T> Image<T>::operator/ (const T2& rhs) const
    {
        Image<T> output = *this;
        output /= rhs;
        return output;
    };

    template <IsPixel T>
    Image<T> Image<T>::operator/ (const Image<T>& rhs) const
    {
        Image<T> output = *this;
        output /= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <typename T, typename T2>
    std::enable_if_t<!std::is_same_v<std::decay_t<T2>, Image<T>>, Image<T>>
        operator/ (T2 lhs, const Image<T>& rhs)
    {
        Image<T> result(rhs.resolution());
        for (size_t i = 0; i < rhs.size(); ++i) {
            result[i] = static_cast<T>(lhs) / rhs[i];
        }
        return result;
    }


    // ================= //
    // === To Buffer === //
    // ================= //
    template <IsPixel T>
    size_t Image<T>::getOutputChannels(bool includeAlpha) const
    {
        size_t base_channels = this->numChannels();
        return (this->hasAlpha() && includeAlpha) ? (base_channels + 1) : base_channels;
    }


    static void storeValueInBuffer(unsigned char* buffer_ptr, float value, BufferDataType data_type) {
        // Clamp value to [0,1] range
        value = std::clamp(value, 0.0f, 1.0f);

        switch (data_type) {
        case BufferDataType::UINT8: {
            uint8_t int_val = static_cast<uint8_t>(value * 255.0f);
            *buffer_ptr = int_val;
            break;
        }
        case BufferDataType::UINT16: {
            uint16_t int_val = static_cast<uint16_t>(value * 65535.0f);
            std::memcpy(buffer_ptr, &int_val, sizeof(uint16_t));
            break;
        }
        case BufferDataType::UINT32: {
            uint32_t int_val = static_cast<uint32_t>(value * 4294967295.0f);
            std::memcpy(buffer_ptr, &int_val, sizeof(uint32_t));
            break;
        }
        case BufferDataType::UINT64: {
            uint64_t int_val = static_cast<uint64_t>(value * static_cast<double>(std::numeric_limits<uint64_t>::max()));
            std::memcpy(buffer_ptr, &int_val, sizeof(uint64_t));
            break;
        }
        case BufferDataType::FLOAT32: {
            std::memcpy(buffer_ptr, &value, sizeof(float));
            break;
        }
        case BufferDataType::FLOAT64: {
            double double_val = static_cast<double>(value);
            std::memcpy(buffer_ptr, &double_val, sizeof(double));
            break;
        }
        default:
            throw std::runtime_error("Unsupported buffer data type");
        }
    }

    template <IsPixel T>
    std::vector<unsigned char> Image<T>::toBuffer(bool includeAlpha, BufferDataType data_type) const
    {
        const size_t pixel_count = static_cast<size_t>(resolution_.x) * static_cast<size_t>(resolution_.y);

        size_t num_channels = this->numChannels();
        bool will_include_alpha = this->hasAlpha() && includeAlpha;
        size_t output_channels = will_include_alpha ? (num_channels + 1) : num_channels;

        // Determine bytes per sample based on data type
        size_t bytes_per_sample;
        switch (data_type) {
        case BufferDataType::UINT8:
            bytes_per_sample = 1;
            break;
        case BufferDataType::UINT16:
            bytes_per_sample = 2;
            break;
        case BufferDataType::UINT32:
        case BufferDataType::FLOAT32:
            bytes_per_sample = 4;
            break;
        case BufferDataType::UINT64:
        case BufferDataType::FLOAT64:
            bytes_per_sample = 8;
            break;
        default:
            throw std::invalid_argument("Unsupported buffer data type");
        }

        std::vector<unsigned char> buffer(pixel_count * output_channels * bytes_per_sample);

        if (will_include_alpha) {
            const std::vector<float>& alphas = this->getAlpha();

            for (size_t i = 0; i < pixel_count; ++i) {
                const T& pixel = data_[i];

                // Process all color channels
                for (size_t j = 0; j < num_channels; ++j) {
                    float channel_value;

                    if constexpr (IsColorRGB<T>) {
                        channel_value = pixel[j];
                    }
                    else {
                        if (j == 0) {
                            channel_value = pixel;
                        }
                        else {
                            channel_value = 0.0f;
                        }
                    }

                    // Store value based on data type
                    size_t buffer_idx = (i * output_channels + j) * bytes_per_sample;
                    storeValueInBuffer(buffer.data() + buffer_idx, channel_value, data_type);
                }

                // Process alpha channel
                size_t alpha_buffer_idx = (i * output_channels + num_channels) * bytes_per_sample;
                storeValueInBuffer(buffer.data() + alpha_buffer_idx, alphas[i], data_type);
            }
        }
        else {
            for (size_t i = 0; i < pixel_count; ++i) {
                const T& pixel = data_[i];

                for (size_t j = 0; j < num_channels; ++j) {
                    float channel_value;

                    if constexpr (IsColorRGB<T>) {
                        channel_value = pixel[j];
                    }
                    else {
                        if (j == 0) {
                            channel_value = pixel;
                        }
                        else {
                            channel_value = 0.0f;
                        }
                    }

                    // Store value based on data type
                    size_t buffer_idx = (i * num_channels + j) * bytes_per_sample;
                    storeValueInBuffer(buffer.data() + buffer_idx, channel_value, data_type);
                }
            }
        }

        return buffer;
    }



    // =================== //
    // === UV-sampling === //
    // =================== //
    template <IsPixel T>
    T Image<T>::sampleUVs(const UV& uv) const
    {
        Pixel pix;
        Pixel rx{ resolution_.x, resolution_.y };
        pix.x = std::fmod(uv.x * rx.x, rx.x);
        pix.y = std::fmod(uv.y * rx.y, rx.y);

        if (pix.x < 0) {
            pix.x += rx.x;
        }

        if (pix.y < 0) {
            pix.y += rx.y;
        }

        return interpolatePixel(pix);
    };

    template <IsPixel T>
    T Image<T>::sampleUVsNoWrap(const UV& uv) const
    {
        if (uv.x >= 1.f || uv.y >= 1.f || uv.x < 0.f || uv.y < 0.f) {
            return vira::utils::INVALID_VALUE<T>();
        }
        else {
            return sampleUVs(uv);
        }
    };

    template <IsPixel T>
    T Image<T>::interpolatePixel(const vira::Pixel& pixel) const
    {
        int x0 = static_cast<int>(std::floor(pixel.x));
        int y0 = static_cast<int>(std::floor(pixel.y));

        x0 = std::min(x0, static_cast<int>(resolution_.x - 1));
        y0 = std::min(y0, static_cast<int>(resolution_.y - 1));

        int x1 = std::min(x0 + 1, static_cast<int>(resolution_.x - 1));
        int y1 = std::min(y0 + 1, static_cast<int>(resolution_.y - 1));

        T p00 = (*this)(x0, y0);
        T p10 = (*this)(x1, y0);
        T p01 = (*this)(x0, y1);
        T p11 = (*this)(x1, y1);

        if (!vira::utils::IS_VALID(p00) || !vira::utils::IS_VALID(p11) || !vira::utils::IS_VALID(p01) || !vira::utils::IS_VALID(p11)) {
            return vira::utils::INVALID_VALUE<T>();
        }

        if constexpr (vira::IsDoubleVec<T>) {
            double dx = static_cast<double>(pixel.x) - static_cast<double>(x0);
            double dy = static_cast<double>(pixel.y) - static_cast<double>(y0);

            T pval = (1. - dx) * (1. - dy) * p00 +
                dx * (1. - dy) * p10 +
                (1. - dx) * dy * p01 +
                dx * dy * p11;

            return pval;
        }
        else if constexpr (vira::IsFloatVec<T> || vira::IsSpectral<T>) {
            float dx = pixel.x - static_cast<float>(x0);
            float dy = pixel.y - static_cast<float>(y0);

            T pval = (1.f - dx) * (1.f - dy) * p00 +
                dx * (1.f - dy) * p10 +
                (1.f - dx) * dy * p01 +
                dx * dy * p11;

            return pval;
        }
        else {
            float dx = pixel.x - static_cast<float>(x0);
            float dy = pixel.y - static_cast<float>(y0);

            float pval = (1.f - dx) * (1.f - dy) * static_cast<float>(p00) +
                dx * (1.f - dy) * static_cast<float>(p10) +
                (1.f - dx) * dy * static_cast<float>(p01) +
                dx * dy * static_cast<float>(p11);

            return static_cast<T>(pval);
        }
    };


    // ========================= //
    // === Meta-data methods === //
    // ========================= //
    template <IsPixel T>
    size_t Image<T>::numChannels() const
    {
        if constexpr (IsVec<T>) {
            return T::length();
        }
        else if constexpr (IsSpectral<T>) {
            return T::size();
        }
        else {
            return 1;
        }
    };

    template <IsPixel T>
    float Image<T>::min() const
    {
        float min = std::numeric_limits<float>::max();

        for (size_t i = 0; i < data_.size(); i++) {
            T val = data_[i];
            if (vira::utils::IS_VALID<T>(val)) {

                float valMagnitude = getMagnitude(val);

                if (min > valMagnitude) {
                    min = valMagnitude;
                }
            }
        }

        return min;
    };

    template <IsPixel T>
    float Image<T>::max() const
    {
        float maxValue = std::numeric_limits<float>::lowest();

        for (size_t i = 0; i < data_.size(); i++) {
            T val = data_[i];
            if (vira::utils::IS_VALID<T>(val)) {

                float valMagnitude = getMagnitude(val);

                if (maxValue < valMagnitude) {
                    maxValue = valMagnitude;
                }
            }
        }

        return maxValue;
    };

    template <IsPixel T>
    std::array<float, 2> Image<T>::minmax() const
    {
        std::array<float, 2> imageMinMax = { this->min(), this->max() };
        return imageMinMax;
    };


    // ===================== //
    // === Alpha methods === //
    // ===================== //
    template <IsPixel T>
    const float& Image<T>::alpha(size_t idx) const
    {
        vira::debug::check_1d_bounds(idx, alpha_.size());
        return this->alpha_[idx];
    };

    template <IsPixel T>
    const float& Image<T>::alpha(int i, int j) const
    {
        vira::debug::check_2d_bounds(i, j, resolution_.x, resolution_.y);
        size_t linearIndex = static_cast<size_t>(i) + (static_cast<size_t>(j) * static_cast<size_t>(resolution_.x));
        return this->alpha_[linearIndex];
    };

    template <IsPixel T>
    void Image<T>::setAlpha(Image<float> newAlpha)
    {
        if (newAlpha.resolution() != resolution_ && newAlpha.size() != 0) {
            throw std::runtime_error("Provided alpha channel does not have the same resolution as the Image being set to");
        }

        this->alpha_ = newAlpha.getVector();
        if (alpha_.size() != 0) {
            this->hasAlpha_ = true;
        }
    };

    template <IsPixel T>
    void Image<T>::setAlpha(std::vector<float> newAlpha)
    {
        if (newAlpha.size() != data_.size() && newAlpha.size() != 0) {
            throw std::runtime_error("Provided alpha channel does not have the same resolution as the Image being set to");
        }

        this->alpha_ = newAlpha;
        if (alpha_.size() != 0) {
            this->hasAlpha_ = true;
        }
    }

    template <IsPixel T>
    const std::vector<float>& Image<T>::getAlpha() const
    {
        return alpha_;
    };

    template <IsPixel T>
    void Image<T>::reset(T resetValue)
    {
        this->alpha_.clear();
        for (size_t i = 0; i < this->data_.size(); ++i) {
            this->data_[i] = resetValue;
        }
    };

    template <IsPixel T>
    void Image<T>::clear()
    {
        this->data_.clear();
        this->alpha_.clear();
    };




    // ======================= //
    // === Modifer methods === //
    // ======================= //
    template <IsPixel T>
    void Image<T>::fillMissing(T fillValue)
    {
        for (size_t i = 0; i < (*this).size(); ++i) {
            if (!vira::utils::IS_VALID((*this)[i])) {
                (*this)[i] = fillValue;
            }
        }
    };

    template <IsPixel T>
    void Image<T>::fillMissing()
    {
        const int maxStep = std::min(resolution_.x / 2, resolution_.y / 2);
        for (int i = 0; i < resolution_.x; i++) {
            for (int j = 0; j < resolution_.y; j++) {
                T value = (*this)(i, j);

                // Check if value is invalid:
                if (vira::utils::IS_VALID(value)) {
                    continue;
                }

                // If it is invalid, obtain the nearest valid pixel:
                // Done inside lambda to immediately escape inner loops:
                [&] {
                    for (int step = 1; step < static_cast<int>(maxStep); step++) {
                        int i_start = std::max(i - step, 0);
                        int i_stop = std::min(i + step, resolution_.x);
                        int j_start = std::max(j - step, 0);
                        int j_stop = std::min(j + step, resolution_.y);

                        for (int ii = i_start; ii < i_stop; ii++) {
                            for (int jj = j_start; jj < j_stop; jj++) {
                                T newValue = (*this)(ii, jj);
                                if (vira::utils::IS_VALID(newValue)) {
                                    (*this)(i, j) = newValue;
                                    return; // Escape the lambda, return to main loop
                                }
                            }
                        }
                    }
                    }();
            }
        }
    };

    template <IsPixel T>
    void Image<T>::crop(const ROI& roi)
    {
        if (roi.x0 >= (resolution_.x - 1) || roi.y0 >= (resolution_.y - 1)) {
            throw std::runtime_error("Cropped ROI is outside of original image");
        }

        int width = roi.width();
        int height = roi.height();

        width = std::min(resolution_.x - roi.x0, width);
        height = std::min(resolution_.y - roi.y0, height);

        Resolution croppedResolution{ width, height };
        Image<T> croppedImage(croppedResolution);

        Image<float> outAlpha;
        if (hasAlpha()) {
            outAlpha = Image<float>(croppedResolution);
        }

        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                croppedImage(i, j) = (*this)(i + roi.x0, j + roi.y0);

                if (hasAlpha()) {
                    outAlpha(i, j) = alpha(i + roi.x0, j + roi.y0);
                }
            }
        }
        croppedImage.setAlpha(outAlpha);
        *this = croppedImage;
    };

    template <IsPixel T>
    void Image<T>::padBounds(int xpad, int ypad)
    {
        // TODO modify the existing buffer, without making a duplicate allocation:
        Resolution paddedResolution = resolution_;
        paddedResolution.x += 2 * xpad;
        paddedResolution.y += 2 * ypad;
        Image<T> paddedImage(paddedResolution);

        for (int i = 0; i < resolution_.x; ++i) {
            for (int j = 0; j < resolution_.y; ++j) {
                paddedImage(i + xpad, j + ypad) = (*this)(i, j);
            }
        }
        *this = paddedImage;
    };

    template <IsPixel T>
    void Image<T>::resize(float scale)
    {
        this->resize(scale * resolution_);
    };

    template <IsPixel T>
    void Image<T>::resize(Resolution newResolution)
    {
        if (newResolution == resolution_) {
            return;
        }

        float stepSizeX = static_cast<float>(resolution_.x - 1) / static_cast<float>(newResolution.x - 1);
        float stepSizeY = static_cast<float>(resolution_.y - 1) / static_cast<float>(newResolution.y - 1);

        // TODO modify the existing buffer, without making a duplicate allocation:
        Image<float> currentAlpha;
        Image<float> outAlpha;
        if (hasAlpha()) {
            outAlpha = Image<float>(newResolution);
            currentAlpha = getAlphaImage();
        }

        // Perform subsampling:
        Image<T> resizedImage(newResolution);
        for (int i = 0; i < newResolution.x; i++) {
            for (int j = 0; j < newResolution.y; j++) {
                float I = static_cast<float>(i) * stepSizeX;
                float J = static_cast<float>(j) * stepSizeY;
                resizedImage(i, j) = interpolatePixel(Pixel{ I,J });
                if (hasAlpha()) {
                    outAlpha(i, j) = currentAlpha.interpolatePixel(Pixel{ I,J });
                }
            }
        }
        resizedImage.setAlpha(outAlpha);
        *this = resizedImage;
    };

    template <IsPixel T>
    void Image<T>::stretch(float newMin, float newMax)
    {
        Image<T> outImage(resolution_);
        outImage.setAlpha(alpha_);

        if (newMax > 1) {
            newMax = 1;
        }

        if (newMin < 0) {
            newMin = 0;
        }

        // Find the extrema:
        float oldMin = this->min();
        float oldMax = this->max();

        float oldRange = oldMax - oldMin;
        float newRange = newMax - newMin;

        // Stretch the data:
        for (size_t i = 0; i < data_.size(); ++i) {
            const T& value = data_[i];
            if (!vira::utils::IS_VALID<T>(value)) {
                outImage[i] = value;
            }
            else {
                if constexpr (IsNumeric<T>) {
                    // other numeric types convert to `float` to be mapped to the new scaling:
                    float value_f = static_cast<float>(value);
                    outImage[i] = static_cast<T>((newRange * (value_f - oldMin) / oldRange) + newMin);
                }
                else {
                    // vec2, vec3, and SpectralData can be multiplied with `float` directly
                    outImage[i] = (newRange * (value - oldMin) / oldRange) + newMin;
                }
            }
        }
        *this = outImage;
    };

    template <IsPixel T>
    void Image<T>::convolve(Image<T>& kernel, bool applyToAlpha) {
        constexpr size_t numChannels = []() {
            if constexpr (IsVec<T>) {
                return static_cast<size_t>(T::length());
            }
            else if constexpr (IsSpectral<T>) {
                return T::size();
            }
            else {
                return static_cast<size_t>(1);
            }
            }();

        // Choose algorithm based on kernel size
        size_t kernelSize = static_cast<size_t>(kernel.resolution_.x) * static_cast<size_t>(kernel.resolution_.y);
        const size_t FFT_THRESHOLD = 256; // Tune this based on performance testing

        if (kernelSize < FFT_THRESHOLD) {
            convolveSpatial(kernel, applyToAlpha);
            return;
        }

        // FFT convolution for large kernels
        Image<T> result(resolution_);

        // Pre-extract all channels (avoid race conditions in extractChannel)
        std::vector<std::vector<float>> imageChannels(numChannels);
        std::vector<std::vector<float>> kernelChannels(numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            imageChannels[ch] = extractChannel(ch);
            kernelChannels[ch] = kernel.extractChannel(ch);
        }

        // Parallel process channels
        std::vector<std::vector<float>> resultChannels(numChannels);

        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, numChannels),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t ch = range.begin(); ch != range.end(); ++ch) {
                    resultChannels[ch] = convolveChannelFFT(
                        imageChannels[ch],
                        kernelChannels[ch],
                        kernel.resolution_
                    );
                }
            }
        );

        // Set all channels in result
        for (size_t ch = 0; ch < numChannels; ++ch) {
            result.setChannel(ch, resultChannels[ch]);
        }

        // Handle alpha channel if requested
        if (applyToAlpha && hasAlpha()) {
            float maxPower = this->max();

            std::vector<float> newAlpha(data_.size());
            for (size_t i = 0; i < data_.size(); ++i) {
                
                float power = getMagnitude(result.data_[i]);

                newAlpha[i] = std::min(1.0f, power / maxPower);
            }

            result.setAlpha(newAlpha);
        }

        *this = result;
    }

    template <IsPixel T>
    void Image<T>::addImage(const Image<T>& addingImage, Pixel center)
    {
        ROI boundingROI(0, 0, resolution_.x, resolution_.y);
        this->addImage(addingImage, center, boundingROI);
    };

    template <IsPixel T>
    void Image<T>::addImage(const Image<T>& addingImage, Pixel center, const ROI& boundingROI)
    {
        // If either image is empty, return early
        if (data_.empty() || addingImage.data_.empty()) {
            return;
        }

        // Extract the fractional part of the center coordinates
        float X = center.x;
        float Y = center.y;
        int ix = static_cast<int>(std::floor(X)); // Integer part of x
        int iy = static_cast<int>(std::floor(Y)); // Integer part of y

        float rx = static_cast<float>(addingImage.resolution_.x) / 2.0f;
        float ry = static_cast<float>(addingImage.resolution_.y) / 2.0f;

        int startX = std::max(boundingROI.x0, ix - static_cast<int>(rx));
        int startY = std::max(boundingROI.y0, iy - static_cast<int>(ry));

        int stopX = std::min(boundingROI.x1, ix + static_cast<int>(rx) + 1);
        int stopY = std::min(boundingROI.y1, iy + static_cast<int>(ry) + 1);

        // If there's no overlap, return early
        if (startX >= stopX || startY >= stopY) {
            return;
        }

        // Loop through all pixels in the target image that might be affected
        for (int i = startX; i < stopX; ++i) {
            for (int j = startY; j < stopY; ++j) {
                // Calculate the position in the kernel image with sub-pixel offset
                float kx = static_cast<float>(i) - (X - rx);
                float ky = static_cast<float>(j) - (Y - ry);

                // We need to sample the kernel with bilinear interpolation
                int kx0 = static_cast<int>(std::floor(kx));
                int ky0 = static_cast<int>(std::floor(ky));
                int kx1 = kx0 + 1;
                int ky1 = ky0 + 1;

                // Calculate interpolation weights
                float wx1 = kx - static_cast<float>(kx0);
                float wy1 = ky - static_cast<float>(ky0);
                float wx0 = 1.0f - wx1;
                float wy0 = 1.0f - wy1;

                // Make sure kernel indices are in bounds
                if (kx0 >= 0 && kx0 < static_cast<int>(addingImage.resolution_.x) &&
                    ky0 >= 0 && ky0 < static_cast<int>(addingImage.resolution_.y)) {

                    T value = T(0);

                    // Perform bilinear interpolation of the kernel
                    if (kx1 < static_cast<int>(addingImage.resolution_.x) &&
                        ky1 < static_cast<int>(addingImage.resolution_.y)) {
                        // Full bilinear interpolation
                        value += addingImage(kx0, ky0) * wx0 * wy0;
                        value += addingImage(kx1, ky0) * wx1 * wy0;
                        value += addingImage(kx0, ky1) * wx0 * wy1;
                        value += addingImage(kx1, ky1) * wx1 * wy1;
                    }
                    else if (kx1 < static_cast<int>(addingImage.resolution_.x)) {
                        // Just interpolate in x
                        value += addingImage(kx0, ky0) * wx0;
                        value += addingImage(kx1, ky0) * wx1;
                    }
                    else if (ky1 < static_cast<int>(addingImage.resolution_.y)) {
                        // Just interpolate in y
                        value += addingImage(kx0, ky0) * wy0;
                        value += addingImage(kx0, ky1) * wy1;
                    }
                    else {
                        // No interpolation needed
                        value = addingImage(kx0, ky0);
                    }

                    this->operator()(i, j) += value;
                }
            }
        }
    };

    // =========================== //
    // === Convolution Helpers === //
    // =========================== //
    template <IsPixel T>
    std::vector<float> Image<T>::convolveChannelFFT(
        const std::vector<float>& imageChannel,
        const std::vector<float>& kernelChannel,
        const Resolution& kernelRes) const {

        // Calculate minimum size for linear convolution
        Resolution minSize{
            resolution_.x + kernelRes.x - 1,
            resolution_.y + kernelRes.y - 1
        };

        // Add padding to prevent wraparound artifacts
        Resolution paddedSize{
            minSize.x + kernelRes.x,
            minSize.y + kernelRes.y
        };

        // Round to power of 2
        auto nextPowerOf2 = [](int x) {
            int power = 1;
            while (power < x) power <<= 1;
            return power;
            };

        paddedSize.x = nextPowerOf2(paddedSize.x);
        paddedSize.y = nextPowerOf2(paddedSize.y);

        FFTWWrapper fft(paddedSize.x, paddedSize.y);

        // === IMAGE: Place at top-left (0,0) ===
        std::fill_n(fft.real_in_, fft.size_, 0.0f);
        for (int j = 0; j < resolution_.y; ++j) {
            for (int i = 0; i < resolution_.x; ++i) {
                size_t srcIdx = static_cast<size_t>(j) * static_cast<size_t>(resolution_.x) + static_cast<size_t>(i);
                size_t dstIdx = static_cast<size_t>(j) * static_cast<size_t>(paddedSize.x) + static_cast<size_t>(i);
                fft.real_in_[dstIdx] = imageChannel[srcIdx];
            }
        }

        fftwf_execute(fft.forward_plan_);

        size_t complexSize = static_cast<size_t>(paddedSize.x) * (static_cast<size_t>(paddedSize.y) / 2 + 1);
        std::unique_ptr<fftwf_complex[]> imageFFT(new fftwf_complex[complexSize]);
        std::memcpy(imageFFT.get(), fft.complex_out_, complexSize * sizeof(fftwf_complex));

        // === KERNEL: SIMPLE PLACEMENT ===
        std::fill_n(fft.real_in_, fft.size_, 0.0f);

        // Create flipped kernel
        std::vector<float> flippedKernel(kernelChannel.size());
        for (int j = 0; j < kernelRes.y; ++j) {
            for (int i = 0; i < kernelRes.x; ++i) {
                size_t srcIdx = static_cast<size_t>(j) * static_cast<size_t>(kernelRes.x) + static_cast<size_t>(i);
                // Flip both dimensions
                int flipped_i = kernelRes.x - 1 - i;
                int flipped_j = kernelRes.y - 1 - j;
                size_t dstIdx = static_cast<size_t>(flipped_j) * static_cast<size_t>(kernelRes.x) + static_cast<size_t>(flipped_i);
                flippedKernel[dstIdx] = kernelChannel[srcIdx];
            }
        }

        // Just place kernel at top-left for now (we'll fix the convolution vs correlation later)
        for (int j = 0; j < kernelRes.y; ++j) {
            for (int i = 0; i < kernelRes.x; ++i) {
                size_t srcIdx = static_cast<size_t>(j) * static_cast<size_t>(kernelRes.x) + static_cast<size_t>(i);
                size_t dstIdx = static_cast<size_t>(j) * static_cast<size_t>(paddedSize.x) + static_cast<size_t>(i);
                fft.real_in_[dstIdx] = flippedKernel[srcIdx];
            }
        }

        fftwf_execute(fft.forward_plan_);

        // === MULTIPLY ===
        for (size_t i = 0; i < complexSize; ++i) {
            float realPart = imageFFT[i][0] * fft.complex_out_[i][0] -
                imageFFT[i][1] * fft.complex_out_[i][1];
            float imagPart = imageFFT[i][0] * fft.complex_out_[i][1] +
                imageFFT[i][1] * fft.complex_out_[i][0];

            fft.complex_in_[i][0] = realPart;
            fft.complex_in_[i][1] = imagPart;
        }

        // === INVERSE FFT ===
        fftwf_execute(fft.inverse_plan_);

        // === EXTRACT "SAME" SIZE RESULT ===
        std::vector<float> result(data_.size());
        float normalization = 1.0f / static_cast<float>(fft.size_);

        // Extract the top-left region (same size as original image)
        int kw_half = kernelRes.x / 2;
        int kh_half = kernelRes.y / 2;

        for (int j = 0; j < resolution_.y; ++j) {
            for (int i = 0; i < resolution_.x; ++i) {
                size_t srcIdx = static_cast<size_t>(j + kh_half) * static_cast<size_t>(paddedSize.x) +
                    static_cast<size_t>(i + kw_half);
                size_t dstIdx = static_cast<size_t>(j) * static_cast<size_t>(resolution_.x) + static_cast<size_t>(i);
                result[dstIdx] = fft.real_out_[srcIdx] * normalization;
            }
        }

        return result;
    }

    template <IsPixel T>
    void Image<T>::convolveSpatial(const Image<T>& kernel, bool applyToAlpha) {
        constexpr size_t numChannels = []() {
            if constexpr (IsVec<T>) {
                return static_cast<size_t>(T::length());
            }
            else if constexpr (IsSpectral<T>) {
                return T::size();
            }
            else {
                return static_cast<size_t>(1);
            }
            }();

        Image<T> result(resolution_);

        int kw = kernel.resolution_.x;
        int kh = kernel.resolution_.y;
        int kw_half = kw / 2;
        int kh_half = kh / 2;

        // Parallel convolution using TBB
        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)
        tbb::parallel_for(
            tbb::blocked_range2d<int>(0, resolution_.y, 0, resolution_.x),
            [&](const tbb::blocked_range2d<int>& range) {
                for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
                    for (int x = range.cols().begin(); x != range.cols().end(); ++x) {

                        if constexpr (numChannels == 1) {
                            float sum = 0.0f;

                            for (int ky = 0; ky < kh; ++ky) {
                                for (int kx = 0; kx < kw; ++kx) {
                                    int ix = x + kx - kw_half;
                                    int iy = y + ky - kh_half;

                                    // Handle boundaries with zero-padding
                                    if (ix >= 0 && ix < resolution_.x && iy >= 0 && iy < resolution_.y) {
                                        if constexpr (IsSpectral<T>) {
                                            sum += (*this)(ix, iy)[0] * kernel(kx, ky)[0];
                                        }
                                        else if constexpr (IsFloatingVec<T>) {
                                            sum += (*this)(ix, iy)[0] * kernel(kx, ky)[0];
                                        }
                                        else {
                                            sum += static_cast<float>((*this)(ix, iy)) *
                                                static_cast<float>(kernel(kx, ky));
                                        }
                                    }
                                }
                            }
                            result(x, y) = static_cast<T>(sum);

                        }
                        else {
                            T sum = T(0);

                            for (int ky = 0; ky < kh; ++ky) {
                                for (int kx = 0; kx < kw; ++kx) {
                                    int ix = x + kx - kw_half;
                                    int iy = y + ky - kh_half;

                                    // Handle boundaries with zero-padding
                                    if (ix >= 0 && ix < resolution_.x && iy >= 0 && iy < resolution_.y) {
                                        sum += (*this)(ix, iy) * kernel(kx, ky);
                                    }
                                }
                            }
                            result(x, y) = sum;
                        }
                    }
                }
            }
        );

        // Handle alpha channel if requested
        if (applyToAlpha && hasAlpha()) {
            // Apply same convolution to alpha channel
            Image<float> alphaImage(resolution_, alpha_);
            Image<float> kernelAlpha(kernel.resolution_);

            // Create a scalar kernel for alpha (average of all channels or use magnitude)
            for (int i = 0; i < kernel.resolution_.x; ++i) {
                for (int j = 0; j < kernel.resolution_.y; ++j) {
                    if constexpr (numChannels == 1) {
                        if constexpr (IsSpectral<T>) {
                            kernelAlpha(i, j) = kernel(i, j)[0];
                        }
                        else if constexpr (IsFloatingVec<T>) {
                            kernelAlpha(i, j) = kernel(i, j)[0];
                        }
                        else {
                            kernelAlpha(i, j) = static_cast<float>(kernel(i, j));
                        }
                        
                    }
                    else if constexpr (IsSpectral<T>) {
                        kernelAlpha(i, j) = kernel(i, j).magnitude();
                    }
                    else if constexpr (IsFloatingVec<T>) {
                        kernelAlpha(i, j) = length(kernel(i, j));
                    }
                }
            }

            alphaImage.convolveSpatial(kernelAlpha, false);
            result.setAlpha(alphaImage.getVector());
        }

        *this = result;
    }

    template <IsPixel T>
    float Image<T>::getMagnitude(T val) const
    {
        float magnitude = 0.f;

        if constexpr (IsSpectral<T>) {
            magnitude = val.magnitude();
        }
        else if constexpr (IsFloatingVec<T>) {
            magnitude = length(val);
        }
        else {
            magnitude = static_cast<float>(val);
        }

        return magnitude;
    };
};