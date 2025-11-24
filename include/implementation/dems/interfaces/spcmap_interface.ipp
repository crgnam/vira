#include <fstream>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <filesystem>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/reference_frame.hpp"
#include "vira/dems/dem.hpp"
#include "vira/dems/dem_projection.hpp"
#include "vira/utils/utils.hpp"

namespace fs = std::filesystem;

namespace vira::dems {

    // Helper function to read binary data from ifstream
    template<typename T>
    void readBinary(std::ifstream& file, T& value) {
        if (!file.read(reinterpret_cast<char*>(&value), sizeof(T))) {
            throw std::runtime_error("Failed to read from file");
        }
    }

    // Helper function to read multiple values
    template<typename T>
    void readBinary(std::ifstream& file, T* values, size_t count) {
        const auto bytes_to_read = static_cast<std::streamsize>(sizeof(T) * count);
        if (!file.read(reinterpret_cast<char*>(values), bytes_to_read)) {
            throw std::runtime_error("Failed to read from file");
        }
    }

    /**
     * @brief Loads a Digital Elevation Model from an SPC maplet file
     * @param filepath Path to the SPC maplet file
     * @param options Loading options for SPC maplet format (default: empty)
     * @return DEM object containing elevation data and spatial reference information
     * @throws std::runtime_error if file cannot be read or has invalid format
     *
     * @details Reads SPC (Small Body Mapping Tool) maplet files containing elevation
     *          and albedo data for planetary surfaces. The format uses 72-byte records
     *          with the first record containing metadata (scale, position, orientation)
     *          and subsequent records containing height/albedo data in 3-byte chunks.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat> SPCMapletInterface<TSpectral, TFloat, TMeshFloat>::load(const fs::path& filepath, SPCMapletOptions options)
    {
        // Description of SPCMaplet record.  (Taken from READ_MAP.f)
        // This routine will read the header information :
        //    Map size = (2 * QSZ + 1) x(2 * QSZ + 1) pixels
        //        Map scale(SCALE in KM)
        //        Body fixed vector from body center to landmark center(V)
        //        Landmark reference plane orientation unit vectors in body fixed
        //        coordinates system UX, UY, UZ
        //     And the height and albedo information for each point on the map
        //     from file <LMRKFILE>.
        //
        //     The mapfile is made up of 72 byte records.The first record contains
        //     information describing the size, scale, orientation and position of the
        //     map :
        //
        //     bytes 1 - 6   Unused
        //     bytes 7 - 10  Scale in km / pixel(real * 4 msb)
        //     bytes 11 - 12 qsz where map is 2 * qsz + 1 x 2 * qsz + 1 pixels(unsigned short lsb)
        //     bytes 16 - 27 map center body fixed position vector in km 3 x(real * 4 msb)
        //     bytes 28 - 39 Ux body fixed unit map axis vector 3 x(real * 4 msb)
        //     bytes 40 - 51 Uy body fixed unit map axis vector 3 x(real * 4 msb)
        //     bytes 52 - 63 Uz body fixed unit map normal vector 3 x(real * 4 msb)
        //     bytes 64 - 67 Hscale = maximum abs(height) / 30000 (real * 4 msb) *
        //     byte 13     255 * X position uncertainty unit vector component(byte) +
        //     byte 14     255 * Y position uncertainty unit vector component(byte) +
        //     byte 15     255 * Z position uncertainty unit vector component(byte) +
        //     bytes 68 - 71 magnitude of position uncertainty(real * 4 msb) +
        //     byte 72     Unused
        //
        // * heights are in units of map scale
        // + these are pretty much unused as far as I can see.
        //
        //     The remaining records are made up of 3 byte chunks :
        //
        //     bytes 1 - 2   height / hscale(integer * 2 msb)
        //     byte 3      relative "albedo" (1 - 199) (byte)
        //
        //     If there is missing data at any point, both height and albedo
        //     are set to zero.

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Could not read file: " + filepath.string());
        }

        // Skip 6 bytes (Unused):
        file.seekg(6, std::ios::beg);

        // Read the scale:
        float scaleBuffer;
        readBinary(file, scaleBuffer);
        utils::reverseFloat(scaleBuffer);

        // Read the size:
        uint16_t sizeBuffer;
        readBinary(file, sizeBuffer);

        // Skip the transformState:  
        size_t transformSize = 3 + 3 * sizeof(float) + 9 * sizeof(float);
        file.seekg(static_cast<std::streamoff>(transformSize), std::ios::cur);

        // Read the height scale:
        float heightScale;
        readBinary(file, heightScale);
        utils::reverseFloat(heightScale);

        // Skip 5 bytes (4 uncertainties + unused byte):
        file.seekg(5, std::ios::cur);

        // Read the height and albedo data:
        vira::images::Resolution resolution{ static_cast<int>(sizeBuffer) * 2 + 1 };
        int numHeights = resolution.x * resolution.y;
        vira::images::Image<float> heights(resolution);
        vira::images::Image<float> albedos(resolution);

        int idx = 0;
        while (file.good() && idx < numHeights) {

            // Read the height from the file:
            int16_t heightBuffer;
            readBinary(file, heightBuffer);
            utils::reverseInt16(heightBuffer);
            bool validPixel = (heightBuffer != 0);

            // Read the albedo from the file:
            uint8_t albedoImage;
            readBinary(file, albedoImage);
            validPixel = validPixel && (albedoImage != 0);

            // Compute image index:
            int col = idx / resolution.x;
            int row = (resolution.y - 1) - (idx % resolution.y); // Flip y-axis for proper face orientation later

            // Initialize as if invalid:
            float currentHeight = std::numeric_limits<float>::infinity();
            float currentAlbedo = std::numeric_limits<float>::infinity();

            // Assign correct data if the pixel is valid:
            if (validPixel) {
                // Obtain the height value:
                currentHeight = scaleBuffer * heightScale * static_cast<float>(heightBuffer);

                // Obtain the albedo value:
                if (options.load_albedos) {
                    currentAlbedo = options.global_albedo * 0.01f * static_cast<float>(albedoImage);
                }
                else {
                    currentAlbedo = options.global_albedo;
                }
            }

            albedos(col, row) = currentAlbedo;
            heights(col, row) = 1000 * currentHeight; // Convert km to m

            idx++;
        }

        if (idx != numHeights) {
            throw std::runtime_error("Unexpected end of file or read error");
        }

        DEMProjection projection;
        projection.resolution = heights.resolution();
        projection.pixelScale[0] = 1000. * static_cast<double>(scaleBuffer); // Convert km to m
        projection.pixelScale[1] = 1000. * static_cast<double>(scaleBuffer); // Convert km to m

        return DEM<TSpectral, TFloat, TMeshFloat>(heights, albedos, projection);
    };

    /**
    * @brief Loads the spatial transformation matrix from an SPC maplet file
    * @param filepath Path to the SPC maplet file
    * @return 4x4 transformation matrix from maplet coordinates to body-fixed coordinates
    * @throws std::runtime_error if file cannot be read or has invalid header format
    *
    * @details Extracts the spatial reference information from the SPC maplet header
    *          including position vector and orientation unit vectors (UX, UY, UZ).
    *          The resulting matrix transforms from local maplet pixel coordinates
    *          to the planetary body-fixed coordinate system.
    */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    mat4<float> SPCMapletInterface<TSpectral, TFloat, TMeshFloat>::loadTransformation(const fs::path& filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Could not read file: " + filepath.string());
        }

        // Skip the header (6 Unused bytes + scaleBuffer + sizeBuffer):
        size_t headerSize = 6 + sizeof(float) + sizeof(uint16_t);
        file.seekg(static_cast<std::streamoff>(headerSize), std::ios::beg);

        // Skip 3 bytes (uncertainty vector):
        file.seekg(3, std::ios::cur);

        // Read the position:
        float positionBuffer[3];
        readBinary(file, positionBuffer, 3);
        for (size_t i = 0; i < 3; i++) {
            utils::reverseFloat(positionBuffer[i]);
        }

        // Read the rotation:
        float rotationBuffer[9];
        readBinary(file, rotationBuffer, 9);
        for (size_t i = 0; i < 9; i++) {
            utils::reverseFloat(rotationBuffer[i]);
        }

        // Format the transformState:
        vec3<float> position{ 1000 * positionBuffer[0], 1000 * positionBuffer[1], 1000 * positionBuffer[2] }; // Convert from km to m

        size_t idx = 0;
        mat3<float> rotmat{ 0. };
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rotmat[j][i] = rotationBuffer[idx];
                idx++;
            }
        }
        Rotation<float> rotation(rotmat);

        return ReferenceFrame<float>::makeTransformationMatrix(position, rotation, vec3<float>{1, 1, 1});
    };
};