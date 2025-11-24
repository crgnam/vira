#include <memory>
#include <string>
#include <iostream>
#include <cstdint>
#include <filesystem>

#include "vira/vira.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    // ========================== //
    // === Validate the Input === //
    // ========================== //
    if (argc != 2) {
        std::cerr << "This example requires passing the vira_box.obj filepath as an argument" << std::endl;
        return 1;
    }

    fs::path filepath = argv[1];

    if (!fs::exists(filepath)) {
        std::cerr << "The vira_box.obj file does not exist at: " << filepath.string() << std::endl;
        return 2;
    }
    
    // ====================== //
    // === Print Progress === //
    // ====================== //
    vira::enablePrintStatus();


    // ======================== //
    // === Create the Scene === //
    // ======================== //
    vira::Scene<vira::Visible_8bin> scene;

    scene.setBackgroundEmissionRGB(vira::ColorRGB{ 0.0053f, 0.0081f, 0.092f });

    
    // ============================ //
    // === Configure the Camera === //
    // ============================ //
    // TODO new configuration:
    auto cameraID = scene.newCamera();
    scene[cameraID].enableParallelInitialization();
    scene[cameraID].enableBlenderFrame();
    scene[cameraID].setFocalLength(50. / 1000.);
    scene[cameraID].setFStop(1.8);
    scene[cameraID].setGain(0.6);
    scene[cameraID].setExposureTime(.001);
    scene[cameraID].setDefaultPhotositeLinearScaleFactor(100); // This is a tuning parameter
    
    scene[cameraID].setResolution(500, 500);
    scene[cameraID].setSensorSize(36. / 1000.);

    scene[cameraID].setLocalRotation(vira::Rotation<float>::eulerAngles(90, 0, 90));
    scene[cameraID].setLocalPosition(3.7f, 0.f, 1.f);
    
    
    //auto orex = scene.loadGeometryAsGroup("C:/Users/cgnam/Downloads/Osiris_Rex.glb", "GLB");
    //scene[orex].setLocalScale(0.1);
    //scene[orex].setLocalPosition(2, 0, 1);
    //scene[orex].setLocalEulerAngles(45, 45, 90);

    // ====================== //
    // === Create a Light === //
    // ====================== //
    auto light = scene.newSphereLight(60 * vira::ColorRGB{ .6, .5, .4 }, 0.25, true);
    scene[light].setLocalPosition(2, -1.5f, 2.5f);


    // ========================= //
    // === Load the Geometry === //
    // ========================= //
    // Load default Vira-box:
    auto loadedAssets = scene.loadGeometry(filepath, "OBJ");

    auto vira_box_mesh = loadedAssets.mesh_ids[0];
    auto moon_mesh = loadedAssets.mesh_ids[1];
    
    scene[moon_mesh].setSmoothShading(true);
    
    // Create instances and add to scene:
    scene.newInstance(vira_box_mesh);
    scene.newInstance(moon_mesh);
    
    
    // ============================ //
    // === Configure Pathtracer === //
    // ============================ //
    scene.pathtracer.options.samples = 10;
    scene.pathtracer.options.bounces = 3;
    scene.pathtracer.options.denoise = false;
    
    scene.pathtracer.renderPasses.simulate_lighting = true;
    
    
    // ========================= //
    // === Perform Rendering === //
    // ========================= //
    auto capturedImageRGB = scene.pathtraceRenderRGB(cameraID);
    auto capturedImageSRGB = vira::images::linearToSRGB(capturedImageRGB);
    fs::path outputDirectory = "vira_box_output/";
    scene.imageInterface.write(outputDirectory / "color.png", capturedImageSRGB);
    
    
    // ============================== //
    // === Save Additional Passes === //
    // ============================== //
    auto& renderPasses = scene.pathtracer.renderPasses;
    scene.imageInterface.write(outputDirectory / "albedos.png", vira::images::spectralToRGB(renderPasses.albedo));
    scene.imageInterface.writeMap(outputDirectory / "depths.png", renderPasses.depth, vira::colormaps::viridis());
    scene.imageInterface.writeNormals(outputDirectory / "globalNormals.png", renderPasses.normal_global);
    scene.imageInterface.writeIDs(outputDirectory / "instances.png", renderPasses.instance_id);
    scene.imageInterface.writeIDs(outputDirectory / "meshes.png", renderPasses.mesh_id);
    scene.imageInterface.writeIDs(outputDirectory / "triangles.png", renderPasses.triangle_id);
    scene.imageInterface.writeIDs(outputDirectory / "materials.png", renderPasses.material_id);
    scene.imageInterface.writeMap(outputDirectory / "directLight.png", vira::images::spectralToMono(renderPasses.direct_radiance));
    scene.imageInterface.writeMap(outputDirectory / "indirectLight.png", vira::images::spectralToMono(renderPasses.indirect_radiance));
    
    // Additional operations such as writing the bounding boxes:
    auto boundingBoxImage = scene.drawBoundingBoxes(capturedImageSRGB, renderPasses.depth, cameraID);
    scene.imageInterface.write(outputDirectory / "aabb.png", boundingBoxImage);

    return 0;
};