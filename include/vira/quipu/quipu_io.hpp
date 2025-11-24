#ifndef PRIVATE_VIRA_QUIPU_QUIPU_IO_HPP
#define PRIVATE_VIRA_QUIPU_QUIPU_IO_HPP

#include <fstream>
#include <cstdint>
#include <string>
#include <iostream>
#include <filesystem>
#include <string_view>

// TODO REMOVE THIRD PARTY HEADERS:
#include "glm/glm.hpp"

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/images/image.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/dems/dem_projection.hpp"
#include "vira/quipu/class_ids.hpp"

namespace fs = std::filesystem;

namespace vira::quipu {
    // ================================ //
    // === Quipu Open and Validator === //
    // ================================ //
    inline void open(const fs::path& filepath, std::ifstream& file);
    

    // ============================= //
    // === Quipu File Identifier === //
    // ============================= //
    constexpr std::string_view identifier = "QUIPU";
    constexpr size_t sizeOfIdentifier = identifier.size();
    inline size_t writeIdentifier(std::ofstream& file);
    
    
    // ======================== //
    // === Basic Buffer I/O === //
    // ======================== //
    inline size_t writeBuffer(std::ofstream& file, char* buffer, size_t bufferSize);
    inline size_t writeBuffer(std::ofstream& file, const char* buffer, size_t bufferSize);
    inline void readBuffer(std::ifstream& file, char* buffer, size_t bufferSize);
    
    
    // ======================= //
    // === Basic value I/O === //
    // ======================= //
    template <typename T>
    size_t writeValue(std::ofstream& file, T value);
    template <typename T>
    void readValue(std::ifstream& file, T& value);

    template <typename T>
    size_t writeVector(std::ofstream& file, const std::vector<T>& vec);
    template <typename T>
    void readVector(std::ifstream& file, std::vector<T>& vec);

    
    // ====================== //
    // === Resolution I/O === //
    // ====================== //
    inline size_t writeResolution(std::ofstream& file, const vira::images::Resolution& resolution);
    inline void readResolution(std::ifstream& file, vira::images::Resolution& resolution);
    
    
    
    // =================== //
    // === ClassID I/O === //
    // =================== //
    template <typename T>
    size_t writeClassID(std::ofstream& file);
    inline size_t writeClassID(std::ofstream& file, ViraClassID classID);
    inline void readClassID(std::ifstream& file, ViraClassID& classID);
    
    
    
    // ========================================= //
    // === Lz4 Compression and Decompression === //
    // ========================================= //
    inline size_t compressData(std::ofstream& file, void* uncompressedBuffer, size_t uncompressedBufferSize);
    inline void decompressData(std::ifstream& file, char* uncompressedBuffer, size_t uncompressedBufferSize);
    
    
    // ======================= //
    // === Typed Value I/O === //
    // ======================= //
    template <typename T>
    size_t writeTypedValue(std::ofstream& file, T value);
    template <typename T, typename Tload>
    void _readValue(std::ifstream& file, T& value);
    template <typename T>
    void readTypedValue(std::ifstream& file, T& value);
    
    
    // =================== //
    // === GLM Vec I/O === //
    // =================== //
    template <IsFloat TFloat, int N>
    size_t writeVec(std::ofstream& file, glm::vec<N, TFloat, glm::highp>& vector);
    template <IsFloat TFloat, int N>
    size_t writeVec(std::ofstream& file, const glm::vec<N, TFloat, glm::highp>& vector);
    template <IsFloat TFloat, int N, typename TFloatLoad>
    void _readVec(std::ifstream& file, glm::vec<N, TFloat, glm::highp>& vector);
    template <IsFloat TFloat, int N>
    void readVec(std::ifstream& file, glm::vec<N, TFloat, glm::highp>& vector);
    
    
    
    // =================== //
    // === GLM Mat I/O === //
    // =================== //
    template <IsFloat TFloat, int N, int M>
    size_t writeMat(std::ofstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix);
    template <IsFloat TFloat, int N, int M>
    size_t writeMat(std::ofstream& file, const glm::mat<N, M, TFloat, glm::highp>& matrix);
    template <typename TFloat, int N, int M, typename TFloatLoad>
    void _readMat(std::ifstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix);
    template <IsFloat TFloat, int N, int M >
    void readMat(std::ifstream& file, glm::mat<N, M, TFloat, glm::highp>& matrix);
    
    
    
    // ===================== //
    // === Vira TransformState I/O === //
    // ===================== //
    inline size_t writeTransformation(std::ofstream& file, const mat4<double>& transformation);
    inline void readTransformation(std::ifstream& file, mat4<double>& transformation);
    
    
    // ================ //
    // === AABB I/O === //
    // ================ //
    template <IsSpectral TSpectral, IsFloat TFloat>
    size_t writeAABB(std::ofstream& file, vira::rendering::AABB<TSpectral, TFloat> aabb);
    template <IsSpectral TSpectral, IsFloat TFloat>
    void readAABB(std::ifstream& file, vira::rendering::AABB<TSpectral, TFloat>& aabb);
    
    
    // ================== //
    // === String I/O === //
    // ================== //
    inline size_t writeString(std::ofstream& file, const std::string& str);
    inline void readString(std::ifstream& file, std::string& str);
    
    
    // ========================== //
    // === DEM Projection I/O === //
    // ========================== //
    inline size_t writeDEMProjection(std::ofstream& file, const vira::dems::DEMProjection& projection);
    inline void readDEMProjection(std::ifstream& file, vira::dems::DEMProjection& projection);


    // ==================================//
    // === DEM Height and Albedo I/O === //
    // ==================================//
    inline size_t writeDEMHeights(std::ofstream& file, vira::images::Image<float>& heights_f, float gsdScale, const std::array<float, 2>& heightMinMax, bool compress);
    inline bool readDEMHeights(std::ifstream& file, vira::images::Image<float>& heights_f);

    inline size_t writeAlbedos_f(std::ofstream& file, vira::images::Image<float>& albedos_f, bool compress);
    inline vira::images::Image<float> readAlbedos_f(std::ifstream& file);
    
    template <IsSpectral TSpectral>
    size_t writeAlbedos(std::ofstream& file, vira::images::Image<TSpectral>& albedos, bool compress);

    template <IsSpectral TSpectral>
    vira::images::Image<TSpectral> readAlbedos(std::ifstream& file);

    // ================= //
    // === Image I/O === //
    // ================= //
    template <typename T>
    size_t writeImage(std::ofstream& file, vira::images::Image<T>& image, bool compressed);
    template <typename T>
    bool readImage(std::ifstream& file, vira::images::Image<T>& image);
    
    
    // ========================== //
    // === Skip Field Methods === //
    // ========================== //
    inline void skipNumberOfBytes(std::istream& file, size_t numberOfBytes);
    inline void skipNumberOfBytes(std::ostream& file, size_t numberOfBytes);
    inline void skipNumberOfBytesFrom(std::istream& file, size_t numberOfBytes, std::ios_base::seekdir dir);
    inline void skipNumberOfBytesFrom(std::ostream& file, size_t numberOfBytes, std::ios_base::seekdir dir);
    inline void skipVec(std::ifstream& file);
    inline void skipMat(std::ifstream& file);
    inline void skipTransformState(std::ifstream& file);
    inline void skipVec3d(std::ifstream& file);
    inline void skipMat3d(std::ifstream& file);
};

#include "implementation/quipu/quipu_io.ipp"

#endif