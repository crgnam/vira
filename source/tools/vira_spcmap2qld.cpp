#include <stdexcept>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cstdint>

#include "tclap/CmdLine.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "vira/vira.hpp"
#include "vira/utils/print_utils.hpp"

using TFloat = float;
using TSpectral = vira::Visible_1bin;
using TMeshFloat = float;

struct Input {
    std::vector<std::string> filepaths;
    std::string outputDirectory;
    bool compressData;
    bool writeAlbedos;
    bool parallel;

    Input() = default;
    Input(int argc, char* argv[])
    {
        TCLAP::CmdLine cmd("Convert SPCMaplets to Quipu QLD files.  Tool provided by Vira", ' ', VIRA_VERSION);

        // Multiple input arguments (variable number)
        TCLAP::UnlabeledMultiArg<std::string> inputArg("input", "Glob(s) of input files", true, "input globs");
        cmd.add(inputArg);

        // Optional output argument with default value
        TCLAP::ValueArg<std::string> outputArg("o", "output", "Directory the output QLD files will be written to", false, ".", "output directory");
        cmd.add(outputArg);

        TCLAP::SwitchArg albedoArg("a", "albedo", "Write albedo data to QLD", false);
        cmd.add(albedoArg);

        TCLAP::SwitchArg compressArg("c", "compress", "Compress data", false);
        cmd.add(compressArg);

        TCLAP::SwitchArg parallelArg("p", "parallel", "Process files in parallel", false);
        cmd.add(parallelArg);

        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the values parsed by each arg
        auto inputs = inputArg.getValue();  // This now returns a vector
        outputDirectory = outputArg.getValue();
        compressData = compressArg.getValue();
        writeAlbedos = albedoArg.getValue();
        parallel = parallelArg.getValue();

        // Validate inputs:
        if (inputs.size() == 0) {
            throw std::runtime_error("No input files were provided");
        }

        // Process all input globs and combine the results
        for (const auto& input : inputs) {
            auto files = vira::utils::getFiles(input);
            filepaths.insert(filepaths.end(), files.begin(), files.end());
        }
    }
};

int main(int argc, char* argv[])
{
    vira::enablePrintStatus();

    Input input;
    try {
        vira::print::initializePrinting();
        input = Input(argc, argv);
    }
    catch (TCLAP::ArgException& e)
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    // Load all Files:
    try {
        // Make the desired output directory if it doesn't already exist:
        vira::utils::makePath(input.outputDirectory);

        // This script uses TFloat = double, TSpectral = Mono, and TMeshFloat = float
        // because that is the most permissive when reading the files back, and does not
        // significantly increase file size due to TransformState being the only impacted value

        // Create the Interfaces:
        vira::dems::SPCMapletOptions spcOptions{};
        vira::dems::SPCMapletInterface<TSpectral, TFloat, TMeshFloat> spcMapletInterface;

        if (input.parallel) {
            std::cout << "Converting " << input.filepaths.size() << " SPCMaplet files in Parallel:\n" << std::flush;
        }
        else {
            std::cout << "Converting " << input.filepaths.size() << " SPCMaplet files:\n" << std::flush;
        }


        auto bar = vira::print::makeProgressBar("Converting Quipu files");
        vira::print::updateProgressBar(bar, "Converting Quipu files", 0.f);

        // Determine progress bar update frequency:
        size_t N = 1;
        if (input.filepaths.size() > 100) {
            N = 10;
        }
        if (input.filepaths.size() > 1000) {
            N = 50;
        }

        vira::quipu::DEMQuipuWriterOptions options;
        options.compress = input.compressData;
        options.writeAlbedos = input.writeAlbedos;
        if (input.parallel) {
            std::atomic<size_t> counter(0);
            tbb::parallel_for(tbb::blocked_range<size_t>(0, input.filepaths.size()),
                [&](tbb::blocked_range<size_t> r)
                {
                    for (size_t i = r.begin(); i < r.end(); ++i) {
                        // Load the SPCMaplet:
                        vira::mat4<double> transformation = spcMapletInterface.loadTransformation(input.filepaths[i]);
                        vira::dems::DEM<TSpectral, TFloat, TMeshFloat> spcMaplet = spcMapletInterface.load(input.filepaths[i], spcOptions);

                        // Make the DEM Pyramid:
                        std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>> pyramid = spcMaplet.makePyramid();

                        // Format output filepath:
                        std::filesystem::path fullPath = input.outputDirectory;
                        fullPath /= vira::utils::getFileName(input.filepaths[i]);
                        std::string quipuFile = fullPath.generic_string() + ".qld";

                        // Write:
                        vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(quipuFile, pyramid, transformation, options);

                        // Update progress bar periodically
                        size_t current = ++counter;
                        if (current % N == 0 || current == input.filepaths.size()) {
                            vira::print::updateProgressBar(bar, 99 * static_cast<float>(current) / static_cast<float>(input.filepaths.size()));
                        }
                    }
                });
        }
        else {
            for (size_t i = 0; i < input.filepaths.size(); ++i)
            {
                if (i % N == 0 || i == input.filepaths.size()) {
                    vira::print::updateProgressBar(bar, 99 * static_cast<float>(i) / static_cast<float>(input.filepaths.size()));
                }

                // Load the SPCMaplet:
                vira::mat4<double> transformation = spcMapletInterface.loadTransformation(input.filepaths[i]);
                vira::dems::DEM<TSpectral, TFloat, TMeshFloat> spcMaplet = spcMapletInterface.load(input.filepaths[i]);

                // Make the DEM Pyramid:
                std::vector<vira::dems::DEM<TSpectral, TFloat, TMeshFloat>> pyramid = spcMaplet.makePyramid();

                // Format output filepath:
                std::filesystem::path fullPath = input.outputDirectory;
                fullPath /= vira::utils::getFileName(input.filepaths[i]);
                std::string quipuFile = fullPath.generic_string() + ".qld";
                std::cout << "    Writing to: " << quipuFile << "\n" << std::flush;

                // Write:
                vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(quipuFile, pyramid, transformation, options);
            }
        }
        vira::print::updateProgressBar(bar, 100);
    }
    catch (std::exception& err) {
        vira::print::printError(err.what());
        return 1;
    }

    return 0;
};