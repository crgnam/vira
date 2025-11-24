#include <string>
#include <array>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <filesystem>
#include <memory>
#include <vector>

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/dems/dem.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/quipu/class_ids.hpp"
#include "vira/quipu/quipu_io.hpp"

namespace fs = std::filesystem;

// Forward Declare:
namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh;
};

namespace vira::quipu {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEMQuipu<TSpectral, TFloat, TMeshFloat>::DEMQuipu(const fs::path& newFilepath, DEMQuipuReaderOptions<TSpectral> newOptions) :
        filepath{ newFilepath }, options{ newOptions }
    {
        std::ifstream file;
        open(filepath.string(), file);

        // Read the file header:
        readClassID(file, classID);
        readTransformation(file, transformation_);
        readValue(file, numberOfLoDs);
        readValue(file, _hasAlbedos);
        readVec<float, 3>(file, normal);
        readValue(file, coneAngle);

        // Read the GSD and Buffer offset table:
        gsds = std::vector<double>(numberOfLoDs);
        offsets = std::vector<size_t>(numberOfLoDs);
        for (size_t i = 0; i < numberOfLoDs; ++i) {
            readValue(file, gsds[i]);
            readValue(file, offsets[i]);
        }

        // Use current file pointer as header size to skip to later (we have cached all header data):
        this->headerSize = static_cast<size_t>(file.tellg());

        this->defaultGSD = options.defaultGSD;

        this->albedoProfile = options.albedoProfile;

        file.close();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEMQuipu<TSpectral, TFloat, TMeshFloat>::~DEMQuipu()
    {

    };

    // function to read in the DEM pyramid written out by the static ::write method
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>> DEMQuipu<TSpectral, TFloat, TMeshFloat>::readPyramid()
    {
        std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>> pyramid;
        std::ifstream file;
        open(filepath.string(), file);
        skipNumberOfBytes(file, headerSize);

        for (size_t i = 0; i < numberOfLoDs; ++i) {
            skipNumberOfBytes(file, offsets[i]);

            vira::dems::DEM<TSpectral, TFloat, TMeshFloat> dem;

            vira::dems::DEMProjection projection;
            readDEMProjection(file, projection);

            // Read the height data:
            vira::images::Image<float> heights;
            readDEMHeights(file, heights);

            // Read the albedo data:
            if (_hasAlbedos && options.readAlbedos) {
                vira::dems::AlbedoType albedoType;
                readValue(file, albedoType);

                if (albedoType == vira::dems::CONSTANT_ALBEDO) {
                    TSpectral constantAlbedo;
                    readValue(file, constantAlbedo);

                    dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, constantAlbedo, projection);
                }
                else if (albedoType == vira::dems::FLOAT_IMAGE_ALBEDO) {
                    vira::images::Image<float> albedos_f = readAlbedos_f(file);
                    if (this->albedoProfile == TSpectral{ 1.f }) {
                        dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos_f, projection);
                    }
                    else {
                        vira::images::Image<TSpectral> albedos(albedos_f.resolution());
                        for (size_t j = 0; j < albedos_f.size(); ++j) {
                            albedos[j] = this->albedoProfile * albedos_f[j];
                        }
                        dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos, projection);
                    }
                }
                else if (albedoType == vira::dems::COLOR_IMAGE_ALBEDO) {
                    vira::images::Image<TSpectral> albedos = readAlbedos<TSpectral>(file);

                    dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos, projection);
                }
            }
            else {
                dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, TSpectral{ options.defaultAlbedo }, projection);
            }

            pyramid.push_back(std::move(dem));
        }

        file.close();
        return pyramid;
    }


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEMQuipu<TSpectral, TFloat, TMeshFloat>::readBuffers(vira::geometry::VertexBuffer<TSpectral, TMeshFloat>& vertexBuffer, vira::geometry::IndexBuffer& indexBuffer)
    {
        this->readBuffers(vertexBuffer, indexBuffer, defaultGSD);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEMQuipu<TSpectral, TFloat, TMeshFloat>::readBuffers(vira::geometry::VertexBuffer<TSpectral, TMeshFloat>& vertexBuffer, vira::geometry::IndexBuffer& indexBuffer, double requiredGSD)
    {
        // Skip the identifier (constructor verified valid file):
        std::ifstream file(filepath.string(), std::ifstream::binary);

        // Search for the ideal GSD based on request:
        float oldGSD = this->currentGSD;
        size_t offset = offsets[0]; // Offset cannot possibly be 0
        this->currentGSD = static_cast<float>(gsds[0]);
        for (size_t i = 1; i < numberOfLoDs; i++) {
            if (gsds[i] <= requiredGSD) {
                this->currentGSD = static_cast<float>(gsds[i]);
                offset = offsets[i];
            }
            else {
                break;
            }
        }

        bool updateNotNeeded = this->currentGSD == oldGSD;
        bool dataAlreadyLoaded = !std::isinf(this->currentGSD);
        if (updateNotNeeded && dataAlreadyLoaded) {
            file.close();
            return;
        }

        // Update the defaultGSD with whatever is first loaded:
        if (std::isinf(this->defaultGSD)) {
            this->defaultGSD = this->currentGSD;
        }

        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);


        // Declare, but don't initialize:
        vira::dems::DEM<TSpectral, TFloat, TMeshFloat> dem;

        // Read the projection data:
        vira::dems::DEMProjection projection;
        readDEMProjection(file, projection);

        // Read the height data:
        vira::images::Image<float> heights;
        this->_compressed = readDEMHeights(file, heights);

        // Read the albedo data:
        if (_hasAlbedos && options.readAlbedos) {
            vira::dems::AlbedoType albedoType;
            readValue(file, albedoType);

            if (albedoType == vira::dems::CONSTANT_ALBEDO) {
                TSpectral constantAlbedo;
                readValue(file, constantAlbedo);

                dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, constantAlbedo, projection);
            }
            else if (albedoType == vira::dems::FLOAT_IMAGE_ALBEDO) {
                vira::images::Image<float> albedos_f = readAlbedos_f(file);
                if (this->albedoProfile == TSpectral{ 1.f }) {
                    dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos_f, projection);
                }
                else {
                    vira::images::Image<TSpectral> albedos(albedos_f.resolution());
                    for (size_t i = 0; i < albedos_f.size(); ++i) {
                        albedos[i] = this->albedoProfile * albedos_f[i];
                    }
                    dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos, projection);
                }
            }
            else if (albedoType == vira::dems::COLOR_IMAGE_ALBEDO) {
                vira::images::Image<TSpectral> albedos = readAlbedos<TSpectral>(file);

                dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos, projection);
            }
        }
        else {
            dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heights, TSpectral{ options.defaultAlbedo }, projection);
        }


        // Compute the vertex and index buffers:
        vec3<double> local_position = ReferenceFrame<double>::getPositionFromTransformation(transformation_);
        vertexBuffer = dem.makeVertexBuffer(local_position);
        indexBuffer = dem.makeIndexBuffer();

        file.close();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(fs::path filepath, const std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>>& pyramid, mat4<double> transformation, DEMQuipuWriterOptions options)
    {
        // Ensure file extension to be DEMQuipu LoD (.qld):
        filepath.replace_extension(".qld");
        utils::makePath(filepath);

        // Extract data about heights and albedos:
        uint16_t numberOfLoDs = static_cast<uint16_t>(pyramid.size());
        const vira::dems::DEM<TSpectral, TFloat, TMeshFloat>& baseDEM = pyramid[0];
        vira::dems::AlbedoType albedoType = baseDEM.getAlbedoType();

        // Open file to write to:
        std::ofstream file(filepath, std::ofstream::binary);

        // Write the header:
        uint64_t headerBufferSize = 0;
        headerBufferSize += writeIdentifier(file);

        // Write type:
        headerBufferSize += writeClassID(file, VIRA_DEM_PYRAMID);

        // Write the transformation info:
        headerBufferSize += writeTransformation(file, transformation);

        // Write options:
        headerBufferSize += writeValue(file, numberOfLoDs);
        headerBufferSize += writeValue(file, options.writeAlbedos);

        // Compute the Normal cone:
        const vira::dems::DEM<TSpectral, TFloat, TMeshFloat>& smallestLoD = pyramid[pyramid.size() - 1];
        std::unique_ptr<vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>> mesh = smallestLoD.makeMesh();
        const vira::geometry::VertexBuffer<TSpectral, TMeshFloat>& vertexBuffer = mesh->getVertexBuffer();
        Normal demNormal{ 0,0,0 };
        for (size_t i = 0; i < vertexBuffer.size(); ++i) {
            demNormal += vertexBuffer[i].normal;
        }
        demNormal = normalize(demNormal);

        float coneAngle = 0;
        for (size_t i = 0; i < vertexBuffer.size(); ++i) {
            float angle = dot(vertexBuffer[i].normal, demNormal);
            coneAngle = std::max(coneAngle, std::acos(angle));
        }
        headerBufferSize += writeVec<float, 3>(file, demNormal);
        headerBufferSize += writeValue(file, coneAngle);


        // Write height/albedo min/max values:
        std::array<float, 2> heightMinMax = baseDEM.getHeights().minmax();

        // Calculate initial offset:
        uint64_t sizeOfEntry = sizeof(double) + sizeof(uint64_t); // GSD for lookup and Offset for fetch
        uint64_t tableOfContentsBufferSize = numberOfLoDs * sizeOfEntry;
        uint64_t offset = headerBufferSize + tableOfContentsBufferSize;

        // Write the blocks of data:
        skipNumberOfBytes(file, static_cast<size_t>(offset)); 
        std::vector<uint64_t> bufferSizes(numberOfLoDs);
        for (size_t i = 0; i < pyramid.size(); i++) {
            // Obtain the current LoD:
            auto& lod = pyramid[i];
            uint64_t bufferSize = 0;

            // Write the projection:
            bufferSize += writeDEMProjection(file, lod.getProjection());

            // Convert output height data:
            vira::images::Image<float> heights_f = lod.getHeights();
            std::array<double, 2> gsds = lod.getGSD();
            float gsd = static_cast<float>(std::min(gsds[0], gsds[1]));
            float gsdScale = gsd / 50.f; // Heuristic seems reasonable?

            // Write the height data:
            bufferSize += writeDEMHeights(file, heights_f, gsdScale, heightMinMax, options.compress);

            // Write the albedo data:
            if (options.writeAlbedos) {
                bufferSize += writeValue(file, albedoType);
                if (albedoType == vira::dems::CONSTANT_ALBEDO) {
                    auto constantAlbedo = lod.getConstantAlbedo();
                    bufferSize += writeValue(file, constantAlbedo);
                }
                else if (albedoType == vira::dems::FLOAT_IMAGE_ALBEDO) {
                    // Convert albedo to 8-bit unsigned integer:
                    vira::images::Image<float> albedos_f = lod.getAlbedos_f();
                    bufferSize += writeAlbedos_f(file, albedos_f, options.compress);
                }
                else if (albedoType == vira::dems::COLOR_IMAGE_ALBEDO) {
                    vira::images::Image<TSpectral> albedos = lod.getAlbedos();
                    bufferSize += writeAlbedos<TSpectral>(file, albedos, options.compress);

                }
            }

            bufferSizes[i] = bufferSize;
        }

        // Write the Table of Contents:
        skipNumberOfBytes(file, headerBufferSize);
        for (size_t i = 0; i < pyramid.size(); i++) {
            std::array<double, 2> gsd = pyramid[i].getGSD();

            writeValue(file, gsd[0]);
            writeValue(file, offset);
            offset = offset + bufferSizes[i];
        }
    };
};