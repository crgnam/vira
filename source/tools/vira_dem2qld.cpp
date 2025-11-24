#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <csignal>
#include <memory>
#include <stdio.h>
#include <atomic>
#include <unordered_map>

#include "tclap/CmdLine.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "indicators/progress_bar.hpp"
#include "indicators/cursor_control.hpp"
#include "termcolor/termcolor.hpp"
#ifdef _WIN32
#define YAML_CPP_API
#endif
#include "yaml-cpp/yaml.h"

#include "tools/tool_utils.hpp"
#include "vira/dems/dem.hpp"
#include "vira/dems/interfaces/gdal_interface.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/quipu/dem_quipu.hpp"

#include "vira/tools/geo_translate.hpp"

// Create aliases for precision and color types:
using TFloat = float;
using TSpectral = vira::Visible_8bin;
using TMeshFloat = float;
namespace fs = std::filesystem;


// YAML Input Data Format:
struct InputData {
    fs::path file;
    std::vector<fs::path> albedo_files;
    std::vector<uint8_t> albedo_files_lonwrap;
    fs::path append_right = "";
    fs::path append_below = "";
    float z_scale = 1.f;
    bool compress = false;
    fs::path output;
    int max_vertices;
    float default_albedo;
};

struct OverlapsEntry {
    std::vector<fs::path> paths;
    fs::path output;
};

struct Input {
    int maxAllowedMemory;
    fs::path configPath;
    fs::path rootPath;

    Input() = default;
    Input(int argc, char* argv[])
    {
        TCLAP::CmdLine cmd("DEM Data Pre-processing Script", ' ', VIRA_VERSION);

        TCLAP::UnlabeledValueArg<std::string> yamlArg("config", "DEM config YAML file", true, "", "DEM config YAML");
        cmd.add(yamlArg);

        TCLAP::ValueArg<std::string> rootArg("r", "root-path", "Root file path.  If none is provided, the location of the provided config.yml file is used", false, "", "string");
        cmd.add(rootArg);

        TCLAP::ValueArg<int> maxMemoryArg("m", "max-memory", "Maximum allowed memory usage in GB", false, 25, "int");
        cmd.add(maxMemoryArg);


        // Parse the inputs:
        cmd.parse(argc, argv);
        configPath = yamlArg.getValue();
        vira::utils::validateFile(configPath);

        rootPath = rootArg.getValue();
        if (rootPath.empty()) {
            rootPath = configPath.parent_path();
        }
        rootPath = fs::absolute(rootPath);
        vira::utils::validateDirectory(rootPath);

        maxAllowedMemory = maxMemoryArg.getValue();
        if (maxAllowedMemory < 2) {
            throw std::runtime_error("Input argument --max-memory must be greater than 2 (GB) (was set to: " + std::to_string(maxAllowedMemory) + ")");
        }
    }
};

static inline void parseYAML(Input input, std::vector<InputData>& inputs, std::unordered_map<std::string, OverlapsEntry>& overlaps)
{
    YAML::Node config = YAML::LoadFile(input.configPath.string());

    fs::path outputDir = config["output"].as<std::string>();
    vira::utils::makePath(outputDir);
    vira::utils::validateDirectory(outputDir);

    int max_vertices = config["max-vertices"].as<int>();

    float default_albedo = config["default-albedo"].as<float>();
    if (default_albedo < 0.f || default_albedo > 1.f) {
        throw std::runtime_error("YAML defined default-albedo must be between 0 and 1.  (was set to: " + std::to_string(default_albedo) + ")");
    }

    const YAML::Node& input_nodes = config["inputs"];
    for (const auto& input_node : input_nodes) {
        try {
            InputData data;
            data.file = vira::utils::combinePaths(input.rootPath, fs::path(input_node["file"].as<std::string>()));

            // Parse the output override:
            if (input_node["output"]) {
                data.output = vira::utils::combinePaths(input.rootPath, input_node["output"].as<std::string>(), false);
            }
            else {
                data.output = vira::utils::combinePaths(input.rootPath, outputDir, false);
            }
            data.output = fs::weakly_canonical(data.output);
            vira::utils::makePath(data.output);
            vira::utils::validateDirectory(data.output);

            // Parse the overlap files:
            if (input_node["overlap-files"]) {
                std::vector<fs::path> tmpOverlaps;
                for (const auto& overlap_file : input_node["overlap-files"]) {
                    std::string overlapStr = overlap_file.as<std::string>();
                    if (vira::utils::isGlob(overlapStr)) {
                        std::vector<std::string> expandedOverlap = vira::utils::getFiles(vira::utils::combinePaths(input.rootPath, overlapStr, false));
                        for (std::string& eo : expandedOverlap) {
                            tmpOverlaps.push_back(eo);
                        }
                    }
                    else {
                        tmpOverlaps.push_back(vira::utils::combinePaths(input.rootPath, overlapStr));
                    }
                }

                // Update the mapping of all overlap files:
                for (fs::path& overlapPath: tmpOverlaps) {
                    // This conversion back to string is being done because some compilers do not 
                    // support hashing fs::path, and thus we need to use std::string for std::unordered_map
                    std::string overlap = overlapPath.string();
                    auto it = overlaps.find(overlap);
                    if (it == overlaps.end()) {
                        // Key doesn't exist, create a new vector with the new element
                        overlaps[overlap].paths = std::vector<fs::path>{ data.file };
                        overlaps[overlap].output = data.output;
                    }
                    else {
                        // Key exists, append the new element to the existing vector
                        it->second.paths.push_back(data.file);
                    }
                }
            }

            // Parse the albedo files:
            if (input_node["albedo-files"]) {
                for (const auto& albedo : input_node["albedo-files"]) {
                    std::string albedoStr = albedo.as<std::string>();
                    if (vira::utils::isGlob(albedoStr)) {
                        auto expandedAlbedos = vira::utils::getFiles(vira::utils::combinePaths(input.rootPath, albedoStr, false));
                        for (auto& a : expandedAlbedos) {
                            data.albedo_files.push_back(vira::utils::combinePaths(input.rootPath, a));
                            data.albedo_files_lonwrap.push_back(false);
                        }
                    }
                    else {
                        data.albedo_files.push_back(vira::utils::combinePaths(input.rootPath, albedoStr));
                        data.albedo_files_lonwrap.push_back(false);
                    }
                }
            }

            if (input_node["albedo-files-lonwrap"]) {
                for (const auto& albedo : input_node["albedo-files-lonwrap"]) {
                    std::string albedoStr = albedo.as<std::string>();
                    if (vira::utils::isGlob(albedoStr)) {
                        auto expandedAlbedos = vira::utils::getFiles(vira::utils::combinePaths(input.rootPath, albedoStr, false));
                        for (auto& a : expandedAlbedos) {
                            data.albedo_files.push_back(vira::utils::combinePaths(input.rootPath, a));
                            data.albedo_files_lonwrap.push_back(true);
                        }
                    }
                    else {
                        data.albedo_files.push_back(vira::utils::combinePaths(input.rootPath, albedoStr));
                        data.albedo_files_lonwrap.push_back(true);
                    }
                }
            }

            // Parse the append-right:
            if (input_node["append-right"]) {
                data.append_right = vira::utils::combinePaths(input.rootPath, fs::path(input_node["append-right"].as<std::string>()));
            }

            // Parse the append-below:
            if (input_node["append-below"]) {
                data.append_below = vira::utils::combinePaths(input.rootPath, fs::path(input_node["append-below"].as<std::string>()));
            }

            // Parse the z-scale:
            data.z_scale = input_node["z-scale"] ? input_node["z-scale"].as<float>() : 1.f;
            if (vira::utils::IS_INVALID<float>(data.z_scale)) {
                throw std::runtime_error("Input " + data.file.string() + " has an invalid z-scale of: " + std::to_string(data.z_scale));
            }

            // Parse the compress-flag:
            if (input_node["compress"]) {
                data.compress = input_node["compress"].as<bool>();
            }

            // Parse max-vertices override:
            if (input_node["max-vertices"]) {
                data.max_vertices = input_node["max-vertices"].as<int>();
            }
            else {
                data.max_vertices = max_vertices;
            }

            // Parse default-albedo override:
            if (input_node["default-albedo"]) {
                data.default_albedo = input_node["default-albedo"].as<float>();
                if (data.default_albedo < 0.f || data.default_albedo > 1.f || std::isnan(data.default_albedo)) {
                    throw std::runtime_error("YAML defined default-albedo must be between 0 and 1.  (was set to: " + std::to_string(default_albedo) + ")");
                }
            }
            else {
                data.default_albedo = default_albedo;
            }

            inputs.push_back(data);
        }
        catch (std::exception& e) {
            throw std::runtime_error("While processing input: " + input_node["file"].as<std::string>() + "\n" + std::string(e.what()));
        }
    }

    if (inputs.size() == 0) {
        throw std::runtime_error("No input files were provided");
    }
};

int main(int argc, char* argv[])
{
    // Parse command line inputs:
    Input commandInput;
    try {
        vira::print::initializePrinting();
        commandInput = Input(argc, argv);
    }
    catch (TCLAP::ArgException& e)
    {
        vira::print::printError(e.error() + " for arg " + e.argId());
        return 1;
    }
    catch (std::exception& e) {
        vira::print::printError(e.what());
        return 1;
    }

    // Parse the YAML configurations:
    std::unordered_map<std::string, OverlapsEntry> overlaps;
    std::vector<InputData> inputs;
    try {
        parseYAML(commandInput, inputs, overlaps);
    }
    catch (std::exception& e)
    {
        vira::print::printError(e.what());
        return 1;
    }

    // ======================== //
    // === Begin Processing === //
    // ======================== //
    try {
        vira::enablePrintStatus();

        std::cout << vira::print::printGreen(std::to_string(inputs.size()) + " DEM Processing Jobs Configured:") << "\n";
        for (const auto& input : inputs) {
            std::cout << vira::print::VIRA_INDENT << input.file.filename().string() << "\n";
        }
        std::cout << "\n";

        std::cout << vira::print::printGreen("Max Allowed Memory Usage:") << " ~" + std::to_string(commandInput.maxAllowedMemory) + "GB" << "\n\n";

        vira::dems::setProjDataSearchPaths();

        size_t inputIdx = 1;
        for (const auto& input : inputs) {
            std::cout << vira::print::printGreen("Processing " + std::to_string(inputIdx) + "/" + std::to_string(inputs.size()) + ": ") << input.file.filename().string() << "\n";
            inputIdx++;
            if (input.albedo_files.size() != 0) {
                std::cout << vira::print::VIRA_INDENT << vira::print::printYellow(std::to_string(input.albedo_files.size()) + " Albedo files:") << "\n";
                for (size_t i = 0; i < input.albedo_files.size(); ++i) {
                    std::cout << vira::print::VIRA_DOUBLE_INDENT << input.albedo_files[i].filename().string();
                    if (input.albedo_files_lonwrap[i]) {
                        std::cout << vira::print::printYellow(" (LONWRAP)") << std::endl;
                    }
                    else {
                        std::cout << std::endl;
                    }
                    
                }
            }

            if (!input.append_right.empty()) {
                std::cout << vira::print::VIRA_INDENT << vira::print::printYellow("Append right: ") << input.append_right.filename().string() << std::endl;
            }

            if (!input.append_below.empty()) {
                std::cout << vira::print::VIRA_INDENT << vira::print::printYellow("Append below: ") << input.append_below.filename().string() << std::endl;
            }

            if (input.z_scale != 1.f) {
                std::cout << vira::print::VIRA_INDENT << vira::print::printYellow("Z-scale: ") << input.z_scale << std::endl;
            }

            

            if (input.compress) {
                std::cout << vira::print::VIRA_INDENT << vira::print::printYellow("Using LZ4 Compression") << std::endl;
            }

            std::cout << vira::print::VIRA_INDENT << vira::print::printYellow("Writing outputs to:\n");
            std::cout << vira::print::VIRA_DOUBLE_INDENT << vira::print::printOnBlue(input.output.string()) << "\n";
            std::cout << std::flush;


            // Perform processing:
            vira::quipu::DEMQuipuWriterOptions quipuOptions;
            quipuOptions.compress = input.compress;

            try {
                vira::dems::GDALOptions gdalOptions;
                gdalOptions.scale = input.z_scale;

                vira::dems::GDALOptions gdalTempOptions;
                gdalTempOptions.scale = input.z_scale;

                // Determine if geometry is too big to load all at once:
                vira::images::Resolution resolution = vira::dems::GDALInterface::getResolution(input.file);
                int maxAllowedPixels = commandInput.maxAllowedMemory * 62500000; // (8e9 / (4 * 32));

                // Determine if the appending maps are compatible in size:
                if (!input.append_right.empty()) {
                    vira::images::Resolution resolutionRight = vira::dems::GDALInterface::getResolution(input.append_right);
                    if (resolutionRight.y != resolution.y) {
                        throw std::runtime_error("The specified append-right file does not have a compatible resolution.\n"
                            "    The central DEM has a resolution of: [" + std::to_string(resolution.x) + " x " + std::to_string(resolution.y) + "]\n"
                            "    but the append_right DEM has:        [" + std::to_string(resolutionRight.x) + " x " + std::to_string(resolutionRight.y) + "]"
                        );
                    }
                }

                if (!input.append_below.empty()) {
                    vira::images::Resolution resolutionBelow = vira::dems::GDALInterface::getResolution(input.append_below);
                    if (resolutionBelow.x != resolution.x) {
                        throw std::runtime_error("The specified append-below file does not have a compatible resolution.\n"
                            "    The central DEM has a resolution of: [" + std::to_string(resolution.x) + " x " + std::to_string(resolution.y) + "]\n"
                            "    but the append_below DEM has:        [" + std::to_string(resolutionBelow.x) + " x " + std::to_string(resolutionBelow.y) + "]"
                        );
                    }
                }

                std::vector<std::array<int, 4>> chunks = vira::images::computeChunks(resolution, maxAllowedPixels);
                if (chunks.size() > 1) {
                    std::cout << vira::print::printYellow("File is too large to load all at once.  Breaking into " + std::to_string(chunks.size()) + " chunks\n");
                }

                for (size_t cIdx = 0; cIdx < chunks.size(); ++cIdx) {
                    std::string chunkStr = "";
                    if (chunks.size() > 1) {
                        chunkStr = "_chunk-" + std::to_string(cIdx + 1);
                    }

                    gdalOptions.x_start = chunks[cIdx][0];
                    gdalOptions.y_start = chunks[cIdx][1];
                    gdalOptions.x_width = chunks[cIdx][2];
                    gdalOptions.y_width = chunks[cIdx][3];

                    auto bar = vira::print::makeProgressBar("Reading file");
                    float completion = 0.f;
                    vira::print::updateProgressBar(bar, "Reading file", completion);

                    vira::dems::GeoreferenceImage<float> heightMap = vira::dems::GDALInterface::load(input.file, gdalOptions);
                    completion = 20.f;
                    vira::print::updateProgressBar(bar, "Appending adjacent DEMs", completion);

                    // Append with specified adjacent maps
                    if ((gdalOptions.x_start + gdalOptions.x_width) == resolution.x) {
                        if (!input.append_right.empty()) {
                            gdalTempOptions.x_start = 0;
                            gdalTempOptions.y_start = chunks[cIdx][1];
                            gdalTempOptions.x_width = 1;
                            gdalTempOptions.y_width = chunks[cIdx][3];

                            vira::dems::GeoreferenceImage<float> heightStrip = vira::dems::GDALInterface::load(input.append_right, gdalTempOptions);
                            heightMap.appendRight(heightStrip, 1);
                        }
                    }

                    if ((gdalOptions.y_start + gdalOptions.y_width) == resolution.y) {
                        if (!input.append_below.empty()) {
                            gdalTempOptions.x_start = chunks[cIdx][0];
                            gdalTempOptions.y_start = 0;
                            gdalTempOptions.x_width = chunks[cIdx][2];
                            gdalTempOptions.y_width = 1;

                            // If the DEM has already been augmented horizontally, we need to pad the new DEM to fit:
                            vira::dems::GeoreferenceImage<float> heightStrip = vira::dems::GDALInterface::load(input.append_below, gdalTempOptions);
                            if (!input.append_right.empty()) {
                                heightStrip.padRight(1);
                            }
                            heightMap.appendBelow(heightStrip, 1);
                        }
                    }

                    // Computing tiling:
                    completion = 25.f;
                    vira::print::updateProgressBar(bar, "Generating tiles with " + std::to_string(input.max_vertices) + " vertices", completion);

                    auto tiles = heightMap.tile(input.max_vertices);
                    heightMap.clear();

                    // Remove invalid tiles:
                    std::vector<size_t> invalidTiles;
                    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
                        auto& tile = tiles[tileIdx];
                        bool invalid = true;
                        for (size_t i = 0; i < tile.data.size(); ++i) {
                            if (vira::utils::IS_VALID(tile.data[i])) {
                                invalid = false;
                                break;
                            }
                        }
                        if (invalid) {
                            invalidTiles.push_back(tileIdx);
                        }
                    }
                    std::sort(invalidTiles.begin(), invalidTiles.end(), std::greater<size_t>());

                    // TODO Remove columns/rows of tiles that are fully invalid

                    // Remove the invalid tiles
                    for (size_t index : invalidTiles) {
                        if (index < tiles.size()) {
                            tiles.erase(tiles.begin() + static_cast<std::ptrdiff_t>(index));
                        }
                    }


                    // Allocate albedo tiles:
                    if (input.albedo_files.size() != 0) {
                        vira::dems::GDALOptions albedoOptions;
                        albedoOptions.allow_negative = false;

                        std::vector<vira::dems::GeoreferenceImage<float>> albedoTiles(tiles.size());
                        for (size_t k = 0; k < tiles.size(); ++k) {
                            albedoTiles[k] = tiles[k].newBlank<float>();
                        }


                        float percentageStep = (70.f - completion) / static_cast<float>(2 * input.albedo_files.size());
                        for (size_t aIdx = 0; aIdx < input.albedo_files.size(); ++aIdx) {
                            fs::path albedoPath(input.albedo_files[aIdx]);
                            vira::print::updateProgressBar(bar, "Reading albedo " + std::to_string(aIdx + 1) + "/" + std::to_string(input.albedo_files.size()), completion);

                            auto tmpAlbedo = vira::dems::GDALInterface::load(albedoPath, albedoOptions);

                            if (input.albedo_files_lonwrap[aIdx]) {
                                vira::tools::geoTranslateInMemory(tmpAlbedo);
                            }
                            completion += percentageStep;

                            vira::print::updateProgressBar(bar, "Re-sampling Albedo " + std::to_string(aIdx + 1) + "/" + std::to_string(input.albedo_files.size()), completion);
                            tbb::parallel_for(tbb::blocked_range<size_t>(0, tiles.size()),
                                [&](tbb::blocked_range<size_t> r)
                                {
                                    for (size_t tIdx = r.begin(); tIdx < r.end(); ++tIdx) {
                                        albedoTiles[tIdx].sampleFrom(tmpAlbedo);
                                    }
                                });
                            completion += percentageStep;
                        }
                        quipuOptions.writeAlbedos = true;

                        vira::print::updateProgressBar(bar, "Writing " + std::to_string(tiles.size()) + " Quipu files", 70.f);

                        std::atomic<size_t> counter(0);
                        tbb::parallel_for(tbb::blocked_range<size_t>(0, tiles.size()),
                            [&](tbb::blocked_range<size_t> r)
                            {
                                for (size_t tIdx = r.begin(); tIdx < r.end(); ++tIdx) {
                                    albedoTiles[tIdx].fillMissing(input.default_albedo);

                                    vira::dems::DEM<TSpectral, TFloat, TMeshFloat> dem(tiles[tIdx], albedoTiles[tIdx].data);

                                    // Compute origin:
                                    vira::vec3<double> origin = dem.computeOrigin();
                                    vira::mat4<double> transformation{ 1 };
                                    transformation[3][0] = origin[0];
                                    transformation[3][1] = origin[1];
                                    transformation[3][2] = origin[2];

                                    // Make the DEM Pyramid:
                                    auto pyramid = dem.makePyramid();

                                    // Tile and Write to Quipu
                                    std::string tileName = vira::utils::getFileName(input.file) + chunkStr + "_tile-" + std::to_string(tIdx + 1) + ".qld";
                                    fs::path quipuFile = input.output / tileName;
                                    vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(quipuFile, pyramid, transformation, quipuOptions);

                                    // Update progress bar periodically
                                    size_t current = ++counter;
                                    if (current % 50 == 0 || current == tiles.size()) {
                                        vira::print::updateProgressBar(bar, (static_cast<float>(current) / static_cast<float>(tiles.size()) * (99.f - 70.f)) + 70.f);
                                    }
                                }
                            });
                    }
                    else {
                        quipuOptions.writeAlbedos = false;

                        vira::print::updateProgressBar(bar, "Writing " + std::to_string(tiles.size()) + " Quipu files", 50);

                        std::atomic<size_t> counter(0);
                        tbb::parallel_for(tbb::blocked_range<size_t>(0, tiles.size()),
                            [&](tbb::blocked_range<size_t> r)
                            {
                                for (size_t tIdx = r.begin(); tIdx < r.end(); ++tIdx) {
                                    vira::dems::DEM<TSpectral, TFloat, TMeshFloat> dem(tiles[tIdx]);

                                    // Compute origin:
                                    vira::vec3<double> origin = dem.computeOrigin();
                                    vira::mat4<double> transformation{ 1 };
                                    transformation[3][0] = origin[0];
                                    transformation[3][1] = origin[1];
                                    transformation[3][2] = origin[2];

                                    // Make the DEM Pyramid:
                                    auto pyramid = dem.makePyramid();

                                    // Tile and Write to Quipu
                                    std::string tileName = vira::utils::getFileName(input.file) + chunkStr + "_tile-" + std::to_string(tIdx + 1) + ".qld";
                                    fs::path quipuFile = input.output / tileName;
                                    vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(quipuFile, pyramid, transformation, quipuOptions);

                                    size_t current = ++counter;
                                    if (current % 50 == 0 || current == tiles.size()) {
                                        vira::print::updateProgressBar(bar, (static_cast<float>(current) / static_cast<float>(tiles.size()) * (99.f - 50.f)) + 50.f);
                                    }
                                }
                            });
                    }

                    if (chunks.size() > 1) {
                        vira::print::updateProgressBar(bar, "Completed Chunk " + std::to_string(cIdx + 1) + "/" + std::to_string(chunks.size()), 100);
                    }
                    else {
                        vira::print::updateProgressBar(bar, "Completed", 100);
                    }
                }
            }
            catch (std::exception& e) {
                throw std::runtime_error("While processing: " + input.file.string() + "\n" + std::string(e.what()));
            }

            std::cout << "\n";
        }

        // Perform Overlap Removal:
        std::cout << vira::print::printGreen("Removing overlaps with: ") << std::to_string(overlaps.size()) + " files" << "\n";
        for (const auto& pair : overlaps) {
            auto overlapFile = pair.first;
            std::vector<fs::path> inputPaths = pair.second.paths;
            fs::path output = pair.second.output;

            // Get list of all quipus to process:
            std::vector<fs::path> quipuList;
            for (auto& inputPath : inputPaths) {
                std::string fileGlob = inputPath.filename().replace_extension("").string() + "*.qld";
                std::vector<std::string> newQuipusStr = vira::utils::getFiles(output / fileGlob);
                std::vector<fs::path> newQuipus(newQuipusStr.size());
                for (size_t i = 0; i < newQuipus.size(); ++i) {
                    newQuipus[i] = newQuipusStr[i];
                }
                quipuList.insert(quipuList.end(), newQuipus.begin(), newQuipus.end());
            }

            removeOverlaps<TSpectral, TFloat, TMeshFloat>(overlapFile, quipuList, commandInput.maxAllowedMemory);
        }
    }
    catch (std::exception& e) {
        vira::print::printError(std::string(e.what()));
        return 1;
    }
};