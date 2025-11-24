#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "vira/vira.hpp"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // ========================== //
    // === Validate the Input === //
    // ========================== //
    if (argc != 2) {
        std::cerr << "This example requires passing the tycho2.qsc filepath as an argument" << std::endl;
        return 1;
    }

    fs::path tycho2_file = argv[1];

    fs::path root_path = tycho2_file.parent_path();
    if (!fs::exists(tycho2_file)) {
        std::cerr << "The tycho2.qsc does not exist at: " << tycho2_file.string() << std::endl;
        return 2;
    }

    fs::path naif0012 = root_path / "../kernels/generic/lsk/naif0012.tls";
    if (!fs::exists(naif0012)) {
        std::cerr << "The naif0012.tls does not exist at: " << naif0012.string() << std::endl;
    }

    // ======================== //
    // === Create the Scene === //
    // ======================== //
    vira::Scene<vira::Visible_8bin> scene;
    
    
    // =============================== //
    // === Read the Star Catalogue === //
    // =============================== //
    scene.spice.furnsh(naif0012);
    
    double et = scene.spice.stringToET("2022-12-09T17:29:22");
    scene.loadTychoQuipu(tycho2_file, et);


    // ============================ //
    // === Configure the Camera === //
    // ============================ //
    //float noiseFactor = 1.f; // Tuning parameter (hand tuned just to "look right")
    std::vector<vira::units::Nanometer> wavelengths{ 400, 500, 600, 700, 800, 900, 1000 };
    std::vector<float> QE{ .42f, .53f, .50f, .40f, .27f, .12f, .3f };

    auto cameraID = scene.newCamera();
    scene[cameraID].enableParallelInitialization();
    scene[cameraID].setFocalLength(35.1 / 1000.);
    scene[cameraID].setFStop(2.8);
    scene[cameraID].setResolution(2592, 2048);
    scene[cameraID].setSensorSize(12.4416 / 1000., 9.8304 / 1000.);

    scene[cameraID].setLocalRotation(vira::Rotation<float>::eulerAngles(vira::units::Degree{ 90.f }, 0.f, vira::units::Degree{ 90.f }));
    scene[cameraID].setLocalPosition(3.7f, 0.f, 1.f);

    scene[cameraID].setDefaultAiryDiskPSF();

    scene[cameraID].setGainDB(12.04);
    scene[cameraID].setDefaultPhotositeQuantumEfficiency(wavelengths, QE);
    scene[cameraID].setDefaultPhotositeBitDepth(8);
    scene[cameraID].setDefaultPhotositeWellDepth(13700);
    scene[cameraID].setDefaultPhotositeLinearScaleFactor(55);

    scene[cameraID].setDefaultLowNoise();

    scene[cameraID].setExposureTime(10);


    // ======================== //
    // === Add Some Planets === //
    // ======================== //
    auto jupiterID = scene.newUnresolvedObject();
    scene[jupiterID].setIrradianceFromVisualMagnitude(-4);
    scene[jupiterID].setLocalPosition(915000000000.f, 0, 0);

    // TODO Change this to track the actual object ID
    scene[cameraID].lookAt(scene[jupiterID].getGlobalPosition());

    // ========================= //
    // === Perform Rendering === //
    // ========================= //
    auto capturedImage = scene.unresolvedRenderRGB(cameraID);
    scene.imageInterface.write("vira_stars_output/starfield.png", vira::images::linearToSRGB(capturedImage));

    return 0;
}