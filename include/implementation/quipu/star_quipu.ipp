#include <filesystem>
#include <fstream>

#include "vira/quipu/quipu_io.hpp"
#include "vira/utils/utils.hpp"
#include "vira/unresolved/star.hpp"
#include "vira/unresolved/star_catalogue.hpp"

namespace fs = std::filesystem;

namespace vira::quipu {
    //StarQuipu::StarQuipu(const fs::path& filepath, StarQuipuReaderOptions options)
    //{
    //    std::ifstream file;
    //    QuipuIO::open(filepath, file);
    //    QuipuIO::readVector(file, stars.getVector());
    //    file.close();
    //};

    vira::unresolved::StarCatalogue StarQuipu::read(const fs::path& filepath)
    {
        vira::unresolved::StarCatalogue stars;
        std::ifstream file;
        open(filepath, file);
        readVector(file, stars.getVector());
        file.close();

        return stars;
    };

    void StarQuipu::write(fs::path filepath, const vira::unresolved::StarCatalogue& stars)
    {
        filepath.replace_extension(".qsc");
        utils::makePath(filepath);
        std::ofstream file(filepath, std::ofstream::binary);

        writeIdentifier(file);
        writeVector(file, stars.getVector());

        file.close();
    };
};