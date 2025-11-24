#include <iostream>
#include <filesystem>

#include "vira/vira.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    // ========================== //
    // === Validate the Input === //
    // ========================== //
    if (argc != 2) {
        std::cerr << "This example requires passing the meta_kernel.tm filepath as an argument" << std::endl;
        return 1;
    }

    fs::path filepath = argv[1];
    if (!fs::exists(filepath)) {
        std::cerr << "The meta_kernel.tm does not exist at: " << filepath.string() << std::endl;
        return 2;
    }

    // Get the file path of the Bennu DSK:
    std::string dskFileName = "bennu_g_01680mm_alt_obj_0000n00000_v021.bds";
    fs::path bennu_path = filepath.parent_path() / "bennu/dsk" / dskFileName;
    if (!fs::exists(filepath)) {
        std::cerr << "The " << dskFileName << " file does not exist at: " << filepath.string() << std::endl;
        return 2;
    }

        
    vira::enablePrintStatus();
    
    // ======================== //
    // === Create the Scene === //
    // ======================== //
    vira::Scene<vira::Visible_8bin, float, float> scene;
    scene.spice.furnsh_relative_to_file(filepath);


    // ============================ //
    // === Configure the Camera === //
    // ============================ //
    auto navcam = scene.newCamera();
    scene[navcam].enableParallelInitialization();
    scene[navcam].setFocalLength(7.6 / 1000.);
    scene[navcam].setFStop(5.6);
    scene[navcam].setResolution(1944, 2592);
    scene[navcam].setSensorSize(4.2768 / 1000., 5.7 / 1000.);
    scene[navcam].setDefaultPhotositeQuantumEfficiency(0.8);
    scene[navcam].setDefaultPhotositeLinearScaleFactor(7); // This is a tuning parameter
    scene[navcam].setDefaultAiryDiskPSF();

    // ====================== //
    // === Create the Sun === //
    // ====================== //
    auto sun = scene.newSun();


    // ========================= //
    // === Load the Geometry === //
    // ========================= //
    // Load the Bennu geometry and create instance:
    auto loadedAssets = scene.loadGeometry(bennu_path, "DSK");
    auto bennu_mesh = loadedAssets.mesh_ids[0];
    scene[bennu_mesh].setSmoothShading(true);

    // Create instances and add to scene:
    auto bennu = scene.newInstance(bennu_mesh);


    // ======================= //
    // === Configure SPICE === //
    // ======================= //
    scene.configureSPICE("BENNU", "J2000");
    scene[navcam].configureSPICE("ORX_NAVCAM1", "ORX_NAVCAM1");
    scene[bennu].configureSPICE("BENNU", "IAU_BENNU");
    scene[sun].setNAIFID("SUN"); // No orientation required


    // ============================ //
    // === Configure Pathtracer === //
    // ============================ //
    scene.pathtracer.options.samples = 3;
    scene.pathtracer.options.bounces = 0;
    scene.pathtracer.options.adaptive_sampling = true;

    scene.pathtracer.renderPasses.simulate_lighting = true;
    scene.pathtracer.renderPasses.save_velocity = true;


    // ========================= //
    // === Perform Rendering === //
    // ========================= //
    double step_size = 10 * 60; // seconds
    scene.setSpiceDatetime("2019-02-06T10:27:00");
    std::cout << scene.isDirty() << "\n";
    for (int i = 0; i < 10; i++) {

        // Pathrace short exposure:
        scene[navcam].setExposureTime(.0014);
        std::string output_name = "short_" + vira::utils::padZeros<3>(i) + ".png";
        auto raytraced_image = scene.render(navcam);
        scene.imageInterface.write("vira_orex_output/" + output_name, raytraced_image);
        scene.imageInterface.writeMap("vira_orex_output/depth/" + output_name, scene.pathtracer.renderPasses.depth, vira::colormaps::viridis());
        scene.imageInterface.writeVelocities("vira_orex_output/camera_velocity/" + output_name, scene.pathtracer.renderPasses.velocity_camera);
        scene.imageInterface.writeVelocities("vira_orex_output/global_velocity/" + output_name, scene.pathtracer.renderPasses.velocity_global);


        // Pathrace long exposure:
        scene[navcam].setExposureTime(1);
        output_name = "long_" + vira::utils::padZeros<3>(i) + ".png";
        raytraced_image = scene.render(navcam);
        scene.imageInterface.write("vira_orex_output/" + output_name, raytraced_image);


        // Update the SPICE frames:
        scene.incrementSpiceET(step_size);
    }

    return 0;
};