#include <fstream>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <array>

#include "glm/glm.hpp"
#include "lz4.h"

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/images/image.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/dems/dem_projection.hpp"
#include "vira/quipu/class_ids.hpp"
#include "vira/images/image_utils.hpp"

namespace vira::quipu {
    // ================================ //
    // === Quipu Open and Validator === //
    // ================================ //
    void open(const fs::path& filepath, std::ifstream& file)
    {
        // Check if file exists:
        if (!std::filesystem::exists(filepath)) {
            std::string errorMessage = "There is no file with path: " + filepath.string();
            std::cout << errorMessage << std::endl;
            throw std::invalid_argument(errorMessage);
        }

        // Check if path is a directory:
        if (std::filesystem::is_directory(filepath)) {
            std::string errorMessage = "Path is a directory, not a file: " + filepath.string();
            std::cout << errorMessage << std::endl;
            throw std::invalid_argument(errorMessage);
        }

        // Check if existing file is a valid Quipu:
        file = std::ifstream(filepath, std::ifstream::binary);
        std::array<char, sizeOfIdentifier> identifierIn;
        file.read(identifierIn.data(), sizeOfIdentifier);

        for (uint8_t i = 0; i < sizeOfIdentifier; i++) {
            if (identifierIn[i] != identifier[i]) {
                file.close();
                std::string errorMessage = filepath.string() + " exists, but is not a valid Quipu!";
                std::cout << errorMessage << std::endl;
                throw std::runtime_error(errorMessage);
            }
        }
    }



    // ============================= //
    // === Quipu File Identifier === //
    // ============================= //
    size_t writeIdentifier(std::ofstream& file)
    {
        return writeBuffer(file, identifier.data(), sizeOfIdentifier);
    };


    // ======================== //
    // === Basic Buffer I/O === //
    // ======================== //
    size_t writeBuffer(std::ofstream& file, char* buffer, size_t bufferSize)
    {
        file.write(buffer, static_cast<std::streamsize>(bufferSize));
        return bufferSize;
    };
    size_t writeBuffer(std::ofstream& file, const char* buffer, size_t bufferSize)
    {
        file.write(buffer, static_cast<std::streamsize>(bufferSize));
        return bufferSize;
    };
    void readBuffer(std::ifstream& file, char* buffer, size_t bufferSize)
    {
        if (bufferSize > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("Buffer size too large for stream operation");
        }
        file.read(buffer, static_cast<std::streamsize>(bufferSize));
    };


    // ======================= //
    // === Basic value I/O === //
    // ======================= //
    template <typename T>
    size_t writeValue(std::ofstream& file, T value)
    {
        file.write(reinterpret_cast<char*>(&value), sizeof(T));
        return sizeof(T);
    };
    template <typename T>
    void readValue(std::ifstream& file, T& value)
    {
        file.read(reinterpret_cast<char*>(&value), sizeof(T));
    };



    template <typename T>
    size_t writeVector(std::ofstream& file, const std::vector<T>& vec)
    {
        size_t buffer = 0;
        uint32_t size = static_cast<uint32_t>(vec.size());
        buffer += writeValue(file, size);
        buffer += writeValue(file, static_cast<uint32_t>(size * sizeof(T)));
        size_t bufferSize = size * sizeof(T);
        buffer += writeBuffer(file, reinterpret_cast<const char*>(vec.data()), bufferSize);

        return buffer;
    };

    template <typename T>
    void readVector(std::ifstream& file, std::vector<T>& vec)
    {
        uint32_t size;
        readValue(file, size);
        vec.resize(size);

        uint32_t bufferSize;
        readValue(file, bufferSize);
        readBuffer(file, reinterpret_cast<char*>(vec.data()), bufferSize);
    };


    // ====================== //
    // === Resolution I/O === //
    // ====================== //
    size_t writeResolution(std::ofstream& file, const vira::images::Resolution& resolution)
    {
        size_t bufferSize = 0;

        bufferSize += writeValue<uint32_t>(file, static_cast<uint32_t>(resolution.x));
        bufferSize += writeValue<uint32_t>(file, static_cast<uint32_t>(resolution.y));

        return bufferSize;
    };
    void readResolution(std::ifstream& file, vira::images::Resolution& resolution)
    {
        uint32_t x, y;

        readValue<uint32_t>(file, x);
        readValue<uint32_t>(file, y);

        resolution = vira::images::Resolution(x, y);
    };


    // =================== //
    // === ClassID I/O === //
    // =================== //
    template <typename T>
    size_t writeClassID(std::ofstream& file)
    {
        return writeValue(file, getClassID<T>());
    };

    size_t writeClassID(std::ofstream& file, ViraClassID classID)
    {
        file.write(reinterpret_cast<char*>(&classID), sizeof(ViraClassID));
        return static_cast<size_t>(sizeof(ViraClassID));
    };
    void readClassID(std::ifstream& file, ViraClassID& classID)
    {
        file.read(reinterpret_cast<char*>(&classID), sizeof(ViraClassID));
    };


    // ========================================= //
    // === Lz4 Compression and Decompression === //
    // ========================================= //
    size_t compressData(std::ofstream& file, void* uncompressedBuffer, size_t uncompressedBufferSize)
    {
        size_t bufferSize = 0;

        // Compress the buffer:
        const int maxDstSize = LZ4_compressBound(static_cast<int>(uncompressedBufferSize));

        char* compressedBuffer = static_cast<char*>(malloc(static_cast<size_t>(maxDstSize)));
        const uint64_t compressedBufferSize = static_cast<uint64_t>(LZ4_compress_default(
            reinterpret_cast<char*>(uncompressedBuffer),
            compressedBuffer,
            static_cast<int>(uncompressedBufferSize),
            maxDstSize));

        char* compressedBufferTMP = static_cast<char*>(realloc(
            compressedBuffer,
            static_cast<size_t>(compressedBufferSize)));

        if (compressedBufferTMP == nullptr) {
            free(compressedBuffer);
            throw std::runtime_error("Failed to allocate memory for compressedBuffer");
        }
        compressedBuffer = compressedBufferTMP;

        // Write compressed buffer to file:
        try {
            bufferSize += writeValue(file, compressedBufferSize);
            bufferSize += writeBuffer(file, compressedBuffer, compressedBufferSize);
        }
        catch (...) {
            free(compressedBuffer); // Free heap allocations:
            throw;
        }

        // Free heap allocations:
        free(compressedBuffer);

        return bufferSize;
    };

    void decompressData(std::ifstream& file, char* uncompressedBuffer, size_t uncompressedBufferSize)
    {
        // Read the compressed data from file:
        uint64_t compressedBufferSize;
        readValue<uint64_t>(file, compressedBufferSize);

        char* compressedBuffer = new char[compressedBufferSize];
        readBuffer(file, compressedBuffer, compressedBufferSize);

        // Decompress the data:
        const int decompressedSize = LZ4_decompress_safe(compressedBuffer, uncompressedBuffer,
            static_cast<int>(compressedBufferSize), static_cast<int>(uncompressedBufferSize));

        delete[] compressedBuffer; // Free heap allocation
        if (decompressedSize < 0) {
            throw std::runtime_error("The provided data failed to be decompressed\n");
        }
    };


    // ======================= //
    // === Typed Value I/O === //
    // ======================= //
    template <typename T>
    size_t writeTypedValue(std::ofstream& file, T value)
    {
        size_t bufferSize = 0;

        bufferSize += writeClassID<T>(file);
        bufferSize += writeValue(file, value);

        return bufferSize;
    };
    template <typename T, typename Tload>
    void _readValue(std::ifstream& file, T& value)
    {
        Tload _value;
        file.read(reinterpret_cast<char*>(&_value), sizeof(Tload));
        value = static_cast<T>(_value);
    };
    template <typename T>
    void readTypedValue(std::ifstream& file, T& value)
    {
        ViraClassID classID;
        readClassID(file, classID);

        if (classID == VIRA_FLOAT) {
            _readValue<T, float>(file, value);
        }
        else if (classID == VIRA_DOUBLE) {
            _readValue<T, double>(file, value);
        }

        else if (classID == VIRA_UINT8) {
            _readValue<T, uint8_t>(file, value);
        }
        else if (classID == VIRA_UINT16) {
            _readValue<T, uint16_t>(file, value);
        }
        else if (classID == VIRA_UINT32) {
            _readValue<T, size_t>(file, value);
        }
        else if (classID == VIRA_UINT64) {
            _readValue<T, uint64_t>(file, value);
        }

        else if (classID == VIRA_INT8) {
            _readValue<T, int8_t>(file, value);
        }
        else if (classID == VIRA_INT16) {
            _readValue<T, int16_t>(file, value);
        }
        else if (classID == VIRA_INT32) {
            _readValue<T, int32_t>(file, value);
        }
        else if (classID == VIRA_INT64) {
            _readValue<T, int64_t>(file, value);
        }
        else if (classID == VIRA_CLASS_ID) {
            _readValue<T, ViraClassID>(file, value);
        }
        else {
            std::string errorMessage = "Provided Quipu file contains a value with invalid classID of " + std::to_string(classID);
            throw std::runtime_error(errorMessage);
        }
    };



    // =================== //
    // === GLM Vec I/O === //
    // =================== //
    template <IsFloat TFloat, int N>
    size_t writeVec(std::ofstream& file, glm::vec<N, TFloat, glm::highp>& vector)
    {
        size_t bufferSize = 0;
        bufferSize += writeClassID<TFloat>(file);
        bufferSize += writeValue(file, static_cast<uint8_t>(N));
        for (int i = 0; i < N; i++) {
            file.write(reinterpret_cast<char*>(&vector[i]), sizeof(TFloat));
            bufferSize += sizeof(TFloat);
        }

        return bufferSize;
    };
    template <IsFloat TFloat, int N>
    size_t writeVec(std::ofstream& file, const glm::vec<N, TFloat, glm::highp>& vector)
    {
        size_t bufferSize = 0;
        bufferSize += writeClassID<TFloat>(file);
        bufferSize += writeValue(file, static_cast<uint8_t>(N));
        for (int i = 0; i < N; i++) {
            file.write(reinterpret_cast<const char*>(&vector[i]), sizeof(TFloat));
            bufferSize += sizeof(TFloat);
        }

        return bufferSize;
    };
    template <IsFloat TFloat, int N, typename TFloatLoad>
    void _readVec(std::ifstream& file, glm::vec<N, TFloat, glm::highp>& vector)
    {
        vector = glm::vec<N, TFloat, glm::highp>{ 0 };
        for (int i = 0; i < N; i++) {
            TFloatLoad value;
            file.read(reinterpret_cast<char*>(&value), sizeof(TFloatLoad));
            vector[i] = static_cast<TFloat>(value);
        }
    };
    template <IsFloat TFloat, int N>
    void readVec(std::ifstream& file, glm::vec<N, TFloat, glm::highp>& vector)
    {
        ViraClassID classID;
        uint8_t N_load;

        readClassID(file, classID);
        readValue(file, N_load);

        if (N != N_load) {
            std::string errorMessage = "The specified vector length (" + std::to_string(N) + ") do not match the length " +
                "in the supplied Quipu file (" + std::to_string(N_load) + ")";
            std::cout << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }

        if (classID == VIRA_FLOAT) {
            _readVec<TFloat, N, float>(file, vector);
        }
        else if (classID == VIRA_DOUBLE) {
            _readVec<TFloat, N, double>(file, vector);
        }
        else if (classID == VIRA_UINT8) {
            _readVec<TFloat, N, uint8_t>(file, vector);
        }
        else if (classID == VIRA_UINT16) {
            _readVec<TFloat, N, uint16_t>(file, vector);
        }
        else if (classID == VIRA_UINT32) {
            _readVec<TFloat, N, size_t>(file, vector);
        }
        else if (classID == VIRA_UINT64) {
            _readVec<TFloat, N, uint64_t>(file, vector);
        }
        else {
            std::string errorMessage = "Provided Quipu file contains a Vec with invalid classID of " + std::to_string(classID);
            std::cout << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }
    };



    // =================== //
    // === GLM Mat I/O === //
    // =================== //
    template <IsFloat TFloat, int N, int M>
    size_t writeMat(std::ofstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix)
    {
        size_t bufferSize = 0;
        bufferSize += writeClassID<TFloat>(file);
        bufferSize += writeValue(file, static_cast<uint8_t>(N));
        bufferSize += writeValue(file, static_cast<uint8_t>(M));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                file.write(reinterpret_cast<char*>(&matrix[i][j]), sizeof(TFloat));
                bufferSize += sizeof(TFloat);
            }
        }

        return bufferSize;
    };
    template <IsFloat TFloat, int N, int M>
    size_t writeMat(std::ofstream& file, const glm::mat<N, M, TFloat, glm::highp>& matrix)
    {
        size_t bufferSize = 0;
        bufferSize += writeClassID<TFloat>(file);
        bufferSize += writeValue(file, static_cast<uint8_t>(N));
        bufferSize += writeValue(file, static_cast<uint8_t>(M));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                file.write(reinterpret_cast<const char*>(&matrix[i][j]), sizeof(TFloat));
                bufferSize += sizeof(TFloat);
            }
        }

        return bufferSize;
    };
    template <typename TFloat, int N, int M, typename TFloatLoad>
    void _readMat(std::ifstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix)
    {
        matrix = glm::mat<N, M, TFloat, glm::highp>{ 0 };
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                TFloatLoad value;
                file.read(reinterpret_cast<char*>(&value), sizeof(TFloatLoad));
                matrix[i][j] = static_cast<TFloat>(value);
            }
        }
    };
    template <IsFloat TFloat, int N, int M >
    void readMat(std::ifstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix)
    {
        ViraClassID classID;
        readValue(file, classID);

        uint8_t N_load, M_load;
        readValue(file, N_load);
        readValue(file, M_load);

        if (N != N_load || M != M_load) {
            std::string errorMessage = "The specified matrix dimensions (" + std::to_string(N) + "x" + std::to_string(M) + ") do not match the dimensions " +
                "in the supplied Quipu file (" + std::to_string(N_load) + "x" + std::to_string(M_load) + ")";
            std::cout << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }


        if (classID == VIRA_FLOAT) {
            _readMat<TFloat, N, M, float>(file, matrix);
        }
        else if (classID == VIRA_DOUBLE) {
            _readMat<TFloat, N, M, double>(file, matrix);
        }
        else {
            std::string errorMessage = "Provided Quipu file contains a Mat with invalid classID of " + std::to_string(classID);
            std::cout << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }
    };


    // =============================== //
    // === Vira TransformState I/O === //
    // =============================== //
    size_t writeTransformation(std::ofstream& file, const mat4<double>& transformation)
    {
        size_t bufferSize = 0;

        vec3<double> position = ReferenceFrame<double>::getPositionFromTransformation(transformation);
        vec3<double> scale = ReferenceFrame<double>::getScaleFromTransformation(transformation);
        Rotation<double> rotation = ReferenceFrame<double>::getRotationFromTransformation(transformation, scale);

        bufferSize += writeVec(file, position);
        bufferSize += writeMat(file, rotation.getMatrix());
        bufferSize += writeVec(file, scale);

        return bufferSize;
    };
    void readTransformation(std::ifstream& file, mat4<double>& transformation)
    {
        vec3<double> position;
        mat3<double> matrix;
        vec3<double> scale;

        readVec(file, position);
        readMat(file, matrix);
        readVec(file, scale);

        Rotation<double> rotation(matrix);
        transformation = ReferenceFrame<double>::makeTransformationMatrix(position, rotation, scale);
    };



    // ================ //
    // === AABB I/O === //
    // ================ //
    template <IsSpectral TSpectral, IsFloat TFloat>
    size_t writeAABB(std::ofstream& file, vira::rendering::AABB<TSpectral, TFloat> aabb)
    {
        size_t bufferSize = 0;

        bufferSize += writeVec(file, aabb.bmin);
        bufferSize += writeVec(file, aabb.bmax);

        return bufferSize;
    };
    template <IsSpectral TSpectral, IsFloat TFloat>
    void readAABB(std::ifstream& file, vira::rendering::AABB<TSpectral, TFloat>& aabb)
    {
        readVec(file, aabb.bmin);
        readVec(file, aabb.bmax);
    };


    // ================== //
    // === String I/O === //
    // ================== //
    size_t writeString(std::ofstream& file, const std::string& str)
    {
        size_t buffer = 0;
        uint32_t size = static_cast<uint32_t>(str.size());
        buffer += writeValue<uint32_t>(file, size);
        buffer += writeBuffer(file, str.c_str(), size);
        return buffer;
    };

    void readString(std::ifstream& file, std::string& str)
    {
        uint32_t size;
        readValue<uint32_t>(file, size);

        str.resize(size);
        readBuffer(file, &str[0], size);
    };


    // ========================== //
    // === DEM Projection I/O === //
    // ========================== //
    size_t writeDEMProjection(std::ofstream& file, const vira::dems::DEMProjection& projection)
    {
        size_t buffer = 0;
        buffer += writeResolution(file, projection.resolution);
        buffer += writeValue(file, projection.pixelScale);

        buffer += writeResolution(file, projection.tileDefinitions);
        buffer += writeValue(file, projection.tileRow);
        buffer += writeValue(file, projection.tileCol);
        buffer += writeValue(file, projection.xoff);
        buffer += writeValue(file, projection.yoff);
        buffer += writeValue(file, projection.dx);
        buffer += writeValue(file, projection.dy);

        buffer += writeValue(file, projection.isOGR);
        if (projection.isOGR) {
            buffer += writeString(file, projection.projRef);
            buffer += writeValue(file, projection.rowColRotation);
            buffer += writeValue(file, projection.modelTiePoint);
        }

        return buffer;
    };

    void readDEMProjection(std::ifstream& file, vira::dems::DEMProjection& projection)
    {
        readResolution(file, projection.resolution);
        readValue(file, projection.pixelScale);

        readResolution(file, projection.tileDefinitions);
        readValue(file, projection.tileRow);
        readValue(file, projection.tileCol);
        readValue(file, projection.xoff);
        readValue(file, projection.yoff);
        readValue(file, projection.dx);
        readValue(file, projection.dy);

        readValue(file, projection.isOGR);
        if (projection.isOGR) {
            readString(file, projection.projRef);
            readValue(file, projection.rowColRotation);
            readValue(file, projection.modelTiePoint);
        }
    };


    // ==================================//
    // === DEM Height and Albedo I/O === //
    // ==================================//
    size_t writeDEMHeights(std::ofstream& file, vira::images::Image<float>& heights_f, float gsdScale, const std::array<float, 2>& heightMinMax, bool compress)
    {
        size_t bufferSize = 0;

        // Write the height min/max:
        bufferSize += writeValue(file, heightMinMax);

        // Determine how many bits to use to represent the height data:
        float heightDelta = heightMinMax[1] - heightMinMax[0];
        if ((heightDelta / std::numeric_limits<uint8_t>::max() < gsdScale)) {
            // Use 8-bits:
            bufferSize += writeClassID(file, VIRA_UINT8);
            auto heights_u8 = vira::images::floatToFixed<uint8_t>(heights_f, heightMinMax);
            bufferSize += writeImage(file, heights_u8, compress);
        }
        else if ((heightDelta / std::numeric_limits<uint16_t>::max() < gsdScale)) {
            // Use 16-bits:
            bufferSize += writeClassID(file, VIRA_UINT16);
            auto heights_u16 = vira::images::floatToFixed<uint16_t>(heights_f, heightMinMax);
            bufferSize += writeImage(file, heights_u16, compress);
        }
        else {
            // Use native 32-bit floating point representation:
            bufferSize += writeClassID(file, VIRA_FLOAT);
            bufferSize += writeImage(file, heights_f, compress);
        }

        return bufferSize;
    };

    bool readDEMHeights(std::ifstream& file, vira::images::Image<float>& heights_f)
    {
        std::array<float, 2> heightMinMax;
        readValue(file, heightMinMax);

        bool isCompressed = false;

        ViraClassID precision;
        readClassID(file, precision);
        if (precision == VIRA_UINT8) {
            vira::images::Image<uint8_t> heights_u8;
            isCompressed = readImage(file, heights_u8);
            heights_f = vira::images::fixedToFloat(heights_u8, heightMinMax);
        }
        else if (precision == VIRA_UINT16) {
            vira::images::Image<uint16_t> heights_u16;
            isCompressed = readImage(file, heights_u16);
            heights_f = vira::images::fixedToFloat(heights_u16, heightMinMax);
        }
        else if (precision == VIRA_FLOAT) {
            isCompressed = readImage(file, heights_f);
        }

        return isCompressed;
    };

    size_t writeAlbedos_f(std::ofstream& file, vira::images::Image<float>& albedos_f, bool compress)
    {
        size_t bufferSize = 0;

        auto albedoMinMax = albedos_f.minmax();
        auto albedos_u8 = vira::images::floatToFixed<uint8_t>(albedos_f, albedoMinMax);

        bufferSize += writeValue(file, albedoMinMax);
        bufferSize += writeImage(file, albedos_u8, compress);

        return bufferSize;
    };

    vira::images::Image<float> readAlbedos_f(std::ifstream& file)
    {
        std::array<float, 2> albedoMinMax;
        readValue(file, albedoMinMax);

        vira::images::Image<uint8_t> albedos_u8;
        readImage(file, albedos_u8);

        return vira::images::fixedToFloat<uint8_t>(albedos_u8, albedoMinMax);
    };

    template <IsSpectral TSpectral>
    size_t writeAlbedos(std::ofstream& file, vira::images::Image<TSpectral>& albedos, bool compress)
    {
        size_t bufferSize = 0;

        std::vector<vira::images::Image<float>> albedo_channels = vira::images::channelSplit(albedos);

        bufferSize += writeValue(file, static_cast<uint32_t>(albedo_channels.size()));
        bufferSize += writeValue(file, compress);
        for (auto& albedo_channel : albedo_channels) {
            auto albedoMinMax = albedo_channel.minmax();
            auto albedos_u8 = vira::images::floatToFixed<uint8_t>(albedo_channel, albedoMinMax);
            bufferSize += writeValue(file, albedoMinMax);
            bufferSize += writeImage(file, albedos_u8, compress);
        }

        return bufferSize;
    };

    template <IsSpectral TSpectral>
    vira::images::Image<TSpectral> readAlbedos(std::ifstream& file)
    {
        uint32_t numChannels;
        readValue(file, numChannels);

        if (numChannels != TSpectral::size()) {
            throw std::runtime_error("Quipu file uses a Spectral albedo with " + std::to_string(numChannels) +
                " channels, while TSpectral for the current program has " + std::to_string(TSpectral::size()) +
                " channels.  The two must match!");
        }

        // Read if the data is compressed or not:
        bool compressed;
        readValue(file, compressed);

        // Read each channel:
        std::vector<vira::images::Image<float>> albedo_channels(numChannels);
        for (uint32_t i = 0; i < numChannels; ++i) {
            std::array<float, 2> albedoMinMax;
            readValue(file, albedoMinMax);

            vira::images::Image<uint8_t> albedos_u8;
            readImage(file, albedos_u8);

            // Convert back to linear:
            albedo_channels[i] = vira::images::fixedToFloat<uint8_t>(albedos_u8, albedoMinMax);
        }

        // Merge all channels:
        return vira::images::channelMerge<TSpectral>(albedo_channels);
    };




    // ================= //
    // === Image I/O === //
    // ================= //
    template <typename T>
    size_t writeImage(std::ofstream& file, vira::images::Image<T>& image, bool compressed)
    {
        vira::images::Resolution resolution = image.resolution();

        size_t bufferSize = 0;
        bufferSize += writeClassID<T>(file);
        bufferSize += writeResolution(file, resolution);
        bufferSize += writeValue(file, compressed);

        if (compressed) {
            size_t imageBufferSize = static_cast<size_t>(image.size() * sizeof(T));
            void* uncompressedHeightsBuffer = image.data();
            bufferSize += writeValue(file, imageBufferSize);
            bufferSize += compressData(file, uncompressedHeightsBuffer, imageBufferSize);
        }
        else {
            for (int i = 0; i < resolution.x; i++) {
                for (int j = 0; j < resolution.y; j++) {
                    file.write(reinterpret_cast<const char*>(&image(i, j)), sizeof(T));
                    bufferSize += sizeof(T);
                }
            }
        }

        return bufferSize;
    };
    template <typename T>
    bool readImage(std::ifstream& file, vira::images::Image<T>& image)
    {
        ViraClassID classID;
        readValue(file, classID);

        if (classID != getClassID<T>()) {
            throw std::runtime_error("Image with classID of " + std::to_string(classID) + " cannot be read as " +
                "image with classID of " + std::to_string(getClassID<T>()));
        }

        vira::images::Resolution resolution;
        readResolution(file, resolution);
        bool compressed;
        readValue(file, compressed);

        if (compressed) {
            size_t imageBufferSize;
            readValue(file, imageBufferSize);

            image = vira::images::Image<T>(resolution);
            void* uncompressedBuffer = image.data();
            decompressData(file, reinterpret_cast<char*>(uncompressedBuffer), imageBufferSize);
        }
        else {
            image = vira::images::Image<T>(resolution);
            for (int i = 0; i < resolution.x; i++) {
                for (int j = 0; j < resolution.y; j++) {
                    T value;
                    file.read(reinterpret_cast<char*>(&value), sizeof(T));
                    image(i, j) = value;
                }
            }
        }

        return compressed;
    };


    // ========================== //
    // === Skip Field Methods === //
    // ========================== //
    void skipNumberOfBytes(std::istream& file, size_t numberOfBytes)
    {
        file.seekg(static_cast<std::streamoff>(numberOfBytes), std::ios_base::beg);
    }

    void skipNumberOfBytes(std::ostream& file, size_t numberOfBytes)
    {
        file.seekp(static_cast<std::streamoff>(numberOfBytes), std::ios_base::beg);
    }

    void skipNumberOfBytesFrom(std::istream& file, size_t numberOfBytes, std::ios_base::seekdir dir)
    {
        file.seekg(static_cast<std::streamoff>(numberOfBytes), dir);
    }

    void skipNumberOfBytesFrom(std::ostream& file, size_t numberOfBytes, std::ios_base::seekdir dir)
    {
        file.seekp(static_cast<std::streamoff>(numberOfBytes), dir);
    }

    void skipVec(std::ifstream& file)
    {
        ViraClassID classID;
        readClassID(file, classID);

        uint8_t N_load;
        readValue(file, N_load);

        size_t numberOfBytes = 0;
        if (classID == VIRA_FLOAT) {
            numberOfBytes += sizeof(float) * N_load;
        }
        else if (classID == VIRA_DOUBLE) {
            numberOfBytes += sizeof(double) * N_load;
        }

        skipNumberOfBytesFrom(file, numberOfBytes, std::ios_base::cur);
    };

    void skipMat(std::ifstream& file)
    {
        ViraClassID classID;
        readClassID(file, classID);

        uint8_t N_load, M_load;
        readValue(file, N_load);
        readValue(file, M_load);

        size_t numberOfBytes = 0;
        if (classID == VIRA_FLOAT) {
            numberOfBytes += sizeof(float) * N_load * M_load;
        }
        else if (classID == VIRA_DOUBLE) {
            numberOfBytes += sizeof(double) * N_load * M_load;
        }

        skipNumberOfBytesFrom(file, numberOfBytes, std::ios_base::cur);
    };

    void skipVec3d(std::ifstream& file)
    {
        size_t numberOfBytes = sizeof(ViraClassID) + sizeof(uint8_t) + (3 * sizeof(double));
        skipNumberOfBytesFrom(file, numberOfBytes, std::ios_base::cur);
    };
    void skipMat3d(std::ifstream& file)
    {
        size_t numberOfBytes = sizeof(ViraClassID) + (2 * sizeof(uint8_t)) + (9 * sizeof(double));
        skipNumberOfBytesFrom(file, numberOfBytes, std::ios_base::cur);
    };


    void skipTransformState(std::ifstream& file)
    {
        skipVec3d(file);
        skipMat3d(file);
        skipVec3d(file);
    };
};