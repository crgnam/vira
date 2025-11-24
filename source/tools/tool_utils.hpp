#ifndef TOOLS_TOOL_UTILS_HPP
#define TOOLS_TOOL_UTILS_HPP

#include <string>
#include <array>
#include <cstddef>
#include <iostream>

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "vira/dems/dem.hpp"
#include "vira/dems/interfaces/gdal_interface.hpp"
#include "vira/quipu/dem_quipu.hpp"
#include "vira/utils/utils.hpp"
#include "vira/utils/print_utils.hpp"


// ============================= //
// === Quipu Overlap Removal === //
// ============================= //
template <typename TSpectral, typename TFloat, typename TMeshFloat>
void removeOverlaps(const fs::path& overlapFile, const std::vector<fs::path>& quipuList, int maxAllowedMemory)
{
    std::cout << "\n";
    auto fileName = overlapFile.filename().string();

    try {
        // Determine if geometry is too big to load all at once:
        vira::images::Resolution resolution = vira::dems::GDALInterface::getResolution(overlapFile);
        int64_t maxAllowedPixels = static_cast<int64_t>(maxAllowedMemory) * 125000000; //8e9 / (2 * 32)
        std::vector<std::array<int, 4>> chunks = vira::images::computeChunks(resolution, maxAllowedPixels);
        if (chunks.size() > 1) {
            std::cout << vira::print::printYellow("File is too large to load all at once.  Breaking into " + std::to_string(chunks.size()) + " chunks\n");
        }

        for (size_t cIdx = 0; cIdx < chunks.size(); ++cIdx) {
            std::cout << vira::print::printGreen("Removing Overlap with ") << fileName;
            if (chunks.size() > 1) {
                std::cout << " (chunk " << cIdx << "/" << chunks.size() << ")";
            }
            std::cout << "\n";

            vira::dems::GDALOptions gdalOptions;
            gdalOptions.x_start = chunks[cIdx][0];
            gdalOptions.y_start = chunks[cIdx][1];
            gdalOptions.x_width = chunks[cIdx][2];
            gdalOptions.y_width = chunks[cIdx][3];

            auto bar = vira::print::makeProgressBar("Reading " + fileName);
            vira::print::updateProgressBar(bar, "Reading overlap file", 0);
            vira::dems::GeoreferenceImage<float> baseMap = vira::dems::GDALInterface::load(overlapFile, gdalOptions);


            vira::print::updateProgressBar(bar, "Processing " + std::to_string(quipuList.size()) + " QLD files", 20);
            // Loop over provided quipu files:
            std::atomic<size_t> counter(0);
            tbb::parallel_for(tbb::blocked_range<size_t>(0, quipuList.size()),
                [&](tbb::blocked_range<size_t> r)
                {
                    for (size_t inputIdx = r.begin(); inputIdx < r.end(); ++inputIdx) {
                        auto quipuFile = quipuList[inputIdx];

                        // Load Quipu and remove overlap:
                        vira::quipu::DEMQuipuReaderOptions<TSpectral> quipuReaderOptions;
                        quipuReaderOptions.readAlbedos = true;
                        vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu(quipuFile, quipuReaderOptions);

                        auto pyramid = quipu.readPyramid();
                        vira::mat4<double> transformation = quipu.getTransformation();


                        // Perform removal:
                        bool fullyOverlapping = true;
                        bool checked = false;
                        for (auto& lod : pyramid) {
                            lod.removeOverlap(baseMap);

                            // Check if the entire map is invalid:
                            if (!checked) {
                                checked = true;
                                auto heights = lod.getHeights();
                                for (size_t i = 0; i < heights.size(); ++i) {
                                    if (vira::utils::IS_VALID(heights[i])) {
                                        fullyOverlapping = false;
                                        break;
                                    }
                                }
                            }
                        }


                        // Write (or remove) the updated quipu:
                        if (fullyOverlapping) {
                            std::remove(quipuFile.string().c_str());
                        }
                        else {
                            vira::quipu::DEMQuipuWriterOptions quipuOptions;
                            quipuOptions.writeAlbedos = quipu.hasAlbedos();
                            quipuOptions.compress = quipu.isCompressed();
                            vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>::write(quipuFile, pyramid, transformation, quipuOptions);
                        }

                        // Increment the counter
                        size_t current = ++counter;

                        // Update progress bar periodically (e.g., every 100 items)
                        if (current % 50 == 0 || current == quipuList.size()) {
                            vira::print::updateProgressBar(bar, (static_cast<float>(current) / static_cast<float>(quipuList.size()) * (100.f - 20.f)) + 20.f);
                        }
                    }
                });
        }
    }
    catch (const std::exception& e) {
        vira::print::printError(e.what());
        exit(1);
    }
}


#endif