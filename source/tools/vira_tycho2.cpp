#include "tclap/CmdLine.h"

#include "tools/tool_utils.hpp"
#include "vira/vira.hpp"

using TFloat = float;
using TSpectral = vira::Visible_1bin;
using TMeshFloat = float;


struct Input {
    std::vector<std::string> files;
    fs::path tycho_path;

    Input() = default;
    Input(int argc, char* argv[])
    {
        TCLAP::CmdLine cmd("Convert Tycho2 .dat files to QSC file.  Tool provided by Vira", ' ', VIRA_VERSION);

        // Multiple input arguments (variable number)
        TCLAP::UnlabeledMultiArg<std::string> inputArg("input", "Input glob(s) of Tycho2 .dat files", true, "input files");
        cmd.add(inputArg);

        // Optional output argument with default value
        TCLAP::ValueArg<std::string> outputArg("o", "output", "Directory the tycho2.qsc file will be written to", false, ".", "output directory");
        cmd.add(outputArg);

        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the values parsed by each arg
        auto inputs = inputArg.getValue();
        fs::path output = outputArg.getValue();

        // Validate inputs:
        if (inputs.size() == 0) {
            throw std::runtime_error("No input files were provided");
        }

        // Process all input globs and combine the results
        for (const auto& input : inputs) {
            auto files_ = vira::utils::getFiles(input);
            files.insert(files.end(), files_.begin(), files_.end());
        }

        tycho_path = output / "tycho2.qsc";
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

    // Begin writing outputs:
    try {
        vira::unresolved::StarCatalogue tycho;
        auto bar = vira::print::makeProgressBar("Reading .dat files");
        float completion = 0.f;
        vira::print::updateProgressBar(bar, "Reading " + std::to_string(input.files.size()) + " .dat.XX files", completion);

        size_t i = 0;
        for (auto& dat_file : input.files) {
            fs::path dat = dat_file;
            completion = 100.f * (static_cast<float>(i) / static_cast<float>(input.files.size()));
            vira::print::updateProgressBar(bar, "Reading " + dat.filename().string() + " (" + std::to_string(i + 1) + "/" + std::to_string(input.files.size()) + ")", completion);
            i++;

            vira::unresolved::StarCatalogue _tycho = vira::unresolved::Tycho2Interface::read(dat_file);
            tycho.append(_tycho);
        }
        vira::print::updateProgressBar(bar, "Read " + std::to_string(tycho.size()) + " stars", 100.f);
        
        std::cout << "Saving to: " << input.tycho_path.string() << "..." << std::flush;
        tycho.sortByMagnitude();
        vira::quipu::StarQuipu::write(input.tycho_path, tycho);

        std::cout << "DONE\n";
    }
    catch (std::exception& err) {
        vira::print::printError(err.what());
        return 1;
    }

    return 0;
};