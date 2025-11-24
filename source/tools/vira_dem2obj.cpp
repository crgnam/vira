#include <stdexcept>
#include <vector>
#include <filesystem>
#include <iostream>
#include <limits>
#include <cstdint>
#include <string>

#include "tclap/CmdLine.h"

#include "tools/tool_utils.hpp"
#include "vira/vira.hpp"

using TFloat = float;
using TSpectral = vira::Visible_1bin;
using TMeshFloat = float;


struct Input {
    std::string inputDEM;
    std::string outputDirectory;
    bool writeNormals = false;
    int numTileVertices = std::numeric_limits<int>::max();
    float resize = 1.f;
    float scale = 1.f;
    float heightScale = 1.f;
    bool isMaplet = false;
    bool applyTransformState = false;

    Input() = default;
    Input(int argc, char* argv[])
    {
        TCLAP::CmdLine cmd("Convert DEM to OBJ files.  Tool provided by Vira", ' ', VIRA_VERSION);

        TCLAP::UnlabeledValueArg<std::string> inputArg("input", "DEM file", true, "", "input file");
        cmd.add(inputArg);

        TCLAP::ValueArg<std::string> outputArg("o", "output", "Directory the OBJ files will be written to", false, ".", "output directory");
        cmd.add(outputArg);

        TCLAP::SwitchArg normalArg("n", "normal", "Write vertex normals", false);
        cmd.add(normalArg);

        TCLAP::SwitchArg mapletArg("m", "maplet", "Treat file as SPCMaplet", false);
        cmd.add(mapletArg);

        TCLAP::SwitchArg transformArg("a", "apply-transform", "Apply transform (only applies to SPC Maplets)", false);
        cmd.add(transformArg);

        TCLAP::ValueArg<float> scaleArg("s", "scale", "Scale vertices", false, 1.f, "float");
        cmd.add(scaleArg);

        TCLAP::ValueArg<float> heightScaleArg("z", "height-scale", "Scale height values", false, 1.f, "float");
        cmd.add(heightScaleArg);

        TCLAP::ValueArg<int> tileArg("t", "tiling", "Specify target number of Vertices for tiling the output Mesh", false, std::numeric_limits<int>::max(), "int");
        cmd.add(tileArg);

        TCLAP::ValueArg<float> resizeArg("r", "resize", "Resizing of DEM before generating Mesh", false, 1.f, "float");
        cmd.add(resizeArg);

        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the value parsed by each arg
        inputDEM = inputArg.getValue();
        outputDirectory = outputArg.getValue();
        writeNormals = normalArg.getValue();
        numTileVertices = tileArg.getValue();
        resize = resizeArg.getValue();
        scale = scaleArg.getValue();
        heightScale = heightScaleArg.getValue();
        isMaplet = mapletArg.getValue();
        applyTransformState = transformArg.getValue();
    }
};

static inline void readDEM(Input input, vira::dems::DEM<TSpectral, TFloat, TMeshFloat>& dem, vira::mat4<double>& transformation)
{
    if (input.isMaplet) {
        std::cout << "Loading SPCMaplet from: " << input.inputDEM << "\n";
        vira::dems::SPCMapletInterface<TSpectral, TFloat, TMeshFloat> spcMapletInterface;
        transformation = spcMapletInterface.loadTransformation(input.inputDEM);
        dem = spcMapletInterface.load(input.inputDEM);
    }
    else {
        std::cout << "Loading DEM from: " << input.inputDEM << "\n";
        // Load the Height Data:
        vira::dems::GeoreferenceImage<float> heightData = vira::dems::GDALInterface::load(input.inputDEM);

        // Resize the data if requsted:
        if (input.resize != 1) {
            std::cout << "Resizing height raster by factor of " << input.resize << "...\n" << std::flush;
            heightData.resize(input.resize);
        }

        // Scale the heights if requested:
        if (input.heightScale != 1) {
            std::cout << "Scaling height data by factor of " << input.heightScale << "...\n" << std::flush;
            for (size_t i = 0; i < heightData.size(); ++i) {
                heightData[i] = input.heightScale * heightData[i];
            }
        }

        // Make the Mesh:
        std::cout << "Creating DEM object...\n" << std::flush;
        dem = vira::dems::DEM<TSpectral, TFloat, TMeshFloat>(heightData);
    }
};

int main(int argc, char* argv[])
{
    // Parse the inputs:
    Input input;
    try {
        vira::print::initializePrinting();
        input = Input(argc, argv);
    }
    catch (TCLAP::ArgException& e)
    {
        vira::print::printError(e.error() + " for arg " + e.argId());
        return 1;
    }
    catch (std::exception& e)
    {
        vira::print::printError(std::string(e.what()));
        return 1;
    }


    // Read the input DEM:
    vira::dems::DEM<TSpectral, TFloat, TMeshFloat> dem;
    vira::mat4<double> transformation;
    if (!input.isMaplet) {
        vira::dems::setProjDataSearchPaths();
    }
    try {
        readDEM(input, dem, transformation);
    }
    catch (std::exception& e) {
        vira::print::printError(e.what());
        return 1;
    }

    // Provide output summary:
    std::cout << vira::print::printGreen("Writing outputs to: " + input.outputDirectory + "\n");
    if (input.applyTransformState && input.isMaplet) {
        std::cout << vira::print::printYellow("Applying Transformation:\n");
    }
    if (input.scale != 1) {
        std::cout << vira::print::printYellow("Scaling vertex data by: " + std::to_string(input.scale) + "\n");
    }

    // Begin writing outputs:
    //try {
    //    // Make the desired output directory if it doesn't already exist:
    //    vira::utils::makePath(input.outputDirectory);
    //
    //    vira::geometry::OBJInterface<TSpectral, TFloat, TMeshFloat> objInterface;
    //    objInterface.writeNormal = input.writeNormals;
    //
    //
    //    int numVertices = dem.resolution().x * dem.resolution().y;
    //    if (input.numTileVertices >= numVertices) {
    //        // Create the Mesh object:
    //        auto mesh = dem.makeMesh();
    //
    //        if (input.applyTransformState && input.isMaplet) {
    //            mesh->applyTransformState(transformState);
    //        }
    //
    //        // Scale the vertex data:
    //        if (input.scale != 1) {
    //            mesh->applyScale(input.scale);
    //        }
    //
    //        // Write to OBJ files:
    //        std::filesystem::path fullPath = input.outputDirectory;
    //        fullPath /= vira::utils::getFileName(input.inputDEM);
    //        std::string objFile = fullPath.generic_string() + ".obj";
    //        std::cout << "Writing to: " << objFile << "\n" << std::flush;
    //        objInterface.write(objFile, mesh);
    //    }
    //    else {
    //        std::cout << "Tiling..." << std::flush;
    //        auto tiles = dem.tile(input.numTileVertices);
    //        std::cout << " " << tiles.size() << " tiles created\n" << std::flush;
    //
    //        auto bar = vira::print::makeProgressBar("Writing tiles");
    //        size_t tileID = 0;
    //        for (auto& tile : tiles) {
    //            float completion = 100 * static_cast<float>(tileID) / static_cast<float>(tiles.size());
    //            vira::print::updateProgressBar(bar, "Writing tile " + std::to_string(tileID + 1) + "/" + std::to_string(tiles.size()), completion);
    //            auto mesh = tile.makeMesh();
    //
    //            if (input.applyTransformState && input.isMaplet) {
    //                mesh->applyTransformState(transformState);
    //            }
    //
    //            // Scale the vertex data:
    //            if (input.scale != 1) {
    //                mesh->applyScale(input.scale);
    //            }
    //
    //            // Write to OBJ files:
    //            std::filesystem::path fullPath = input.outputDirectory;
    //            fullPath /= vira::utils::getFileName(input.inputDEM);
    //            std::string objFile = fullPath.generic_string() + "_tile_" + tile.getProjection().getTileString() + ".obj";
    //            objInterface.write(objFile, mesh);
    //
    //            tileID++;
    //        }
    //        vira::print::updateProgressBar(bar, 100.f);
    //    }
    //}
    //catch (std::exception& err) {
    //    vira::print::printError(err.what());
    //    return 1;
    //}

    return 0;
};