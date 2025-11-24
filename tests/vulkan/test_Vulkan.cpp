#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "vulkan_testing.hpp"

using namespace vira::vulkan;

TEST(VulkanTest, pathtracer) {
    
    // =======================
    // Parameters
    // =======================

    // Render Image Parameters

    // Raytracing Parameters
    vira::vulkan::VulkanRendererOptions pathtracerOptions;
    pathtracerOptions.nSamples = 3;
    pathtracerOptions.smoothShading = false;
    
    // Render Loop
    RenderLoopOptions renderOptions{};
    renderOptions.numFrames = 3;
    renderOptions.step_size = 60; // seconds
    std::string start_date = "2019-02-06T10:27:00";

    // Spice / Target Parameters
    std::string meta_kernel_path = "./vira_test_data/vulkan/meta_kernel.tm";
    std::string bennu_path = "./vira_test_data/vulkan/kernels/bennu/dsk/bennu_g_01680mm_alt_obj_0000n00000_v021.bds";
    float global_bennu_albedo = 0.4f;
    std::shared_ptr<vira::McEwen<TSpectral>> material = std::make_shared<vira::McEwen<TSpectral>>();

    // Sensor Parameters
    vira::SensorConfig<TSpectral> sensorConfig;
    sensorConfig.resolution = {WIDTH,  HEIGHT };
    sensorConfig.gain = .6f;
    sensorConfig.quantumEfficiency = TSpectral{ 0.6 };
    sensorConfig.size = { 4.2768f / 1000.f , 5.7f / 1000.f };

    const vk::Extent2D extent{WIDTH, HEIGHT};

    // Camera Parameters
    vira::CameraConfig<TSpectral, TFloat> cameraConfig;
    cameraConfig.focalLength = 7.64f / 1000.f;
    cameraConfig.exposureTime = EXPTIME;
    cameraConfig.renderPasses.render = true;
    cameraConfig.renderPasses.albedo = true;
    cameraConfig.renderPasses.directRadiance = true;
    cameraConfig.renderPasses.indirectRadiance = true;
    cameraConfig.renderPasses.globalNormal = true;
    cameraConfig.renderPasses.alpha = true;
    cameraConfig.renderPasses.depth = true;
    cameraConfig.renderPasses.globalNormal = true;
    cameraConfig.renderPasses.triangleID = true;
    cameraConfig.sensor = std::make_shared<vira::Sensor<TSpectral, TFloat>>(sensorConfig);
    cameraConfig.aperture = std::make_shared<vira::Aperture<TSpectral, TFloat>>(cameraConfig.focalLength / 3.f);
    cameraConfig.distortion = std::make_shared<vira::OpenCVDistortion<TSpectral, TFloat>>();
    cameraConfig.depthOfField = true;


    // =======================
    // Perform Vira Scene Setup
    // =======================

    // Create the Scene object:
    vira::Scene<TSpectral, TFloat, TMeshFloat>* scene = new vira::Scene<TSpectral, TFloat, TMeshFloat>();
    scene->spice.furnsh(meta_kernel_path);

    // Create two viewportCamera objects for the GPU and CPU use:
    vira::ViewportCamera<TSpectral, TFloat> viewportCameraOBJ = vira::ViewportCamera<TSpectral, TFloat>(cameraConfig);
    vira::ViewportCamera<TSpectral, TFloat>* viewportCamera = &viewportCameraOBJ;
    
    // Create light instance:
    std::shared_ptr<vira::Light<TSpectral, TFloat>> sun = scene->newSun();
    
    // Load the Bennu geometry and create instance:
    std::cout << "Reading Bennu DSK from: " << bennu_path << "...\n" << std::flush;
    vira::DSKOptions dskOptions;
    dskOptions.setAlbedo = global_bennu_albedo;
    vira::DSKInterface<TSpectral, TFloat, TMeshFloat> dskInterface;
    auto bennu_mesh = dskInterface.load(bennu_path, dskOptions);
    bennu_mesh->setMaterial(0, material);
    bennu_mesh->setSmoothShading(pathtracerOptions.smoothShading);
    auto bennu = scene->newInstance(bennu_mesh);
    
    // Configure SPICE:
    viewportCamera->setSpicePositionConfig("ORX_NAVCAM1", "BENNU");
    viewportCamera->setSpiceFrameConfig("ORX_NAVCAM1");

    sun->setSpicePositionConfig("SUN", "BENNU");
    bennu->setSpiceFrameConfig("IAU_BENNU");
    renderOptions.et = scene->spice.stringToET(start_date);

    // Update SPICE poses:
    viewportCamera->updateSpicePose(renderOptions.et);

    bennu->updateSpicePose(renderOptions.et);
    sun->updateSpicePose(renderOptions.et);
    scene->processSceneGraph();


    // =======================
    // Perform Vulkan Pathtracer Rendering
    // =======================

    // Create Vira Instance and Vira Device
    std::shared_ptr<vira::vulkan::InstanceFactoryInterface> instanceFactory = std::make_shared<vira::vulkan::VulkanInstanceFactory>();
    std::shared_ptr<vira::vulkan::DeviceFactoryInterface> deviceFactory = std::make_shared<vira::vulkan::VulkanDeviceFactory>();

    vira::vulkan::ViraDevice* viraDevice = new vira::vulkan::ViraDevice{instanceFactory, deviceFactory};
    std::unique_ptr<vira::vulkan::ViraSwapchain> viraSwapchain = std::make_unique<vira::vulkan::ViraSwapchain>(viraDevice, extent, true);        

    // Create Pathtracer
    vira::vulkan::VulkanPathTracer<TSpectral, TFloat, TMeshFloat>* pathtracer = new vira::vulkan::VulkanPathTracer<TSpectral, TFloat, TMeshFloat>(viewportCamera, scene, viraDevice, viraSwapchain.get(), pathtracerOptions);

    // Create Render loop
    vira::vulkan::ViraRenderLoop<TSpectral, TFloat, TMeshFloat>* renderLoop = new vira::vulkan::ViraRenderLoop<TSpectral, TFloat, TMeshFloat>(viraDevice, pathtracer, renderOptions);
    renderLoop->setViraSwapchain(std::move(viraSwapchain));

    // Run Render Loop with Benchmarking
    auto start = std::chrono::high_resolution_clock::now();

    renderLoop->runPathTracerRenderLoop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;    
    std::cout << "Render Loop Elapsed time: " << elapsed_seconds.count() << "s\n";
        
}

TEST(VulkanTest, rasterizer) {

    // =======================
    // Parameters
    // =======================

    // Rasterizer Parameters
    vira::vulkan::VulkanRendererOptions rasterizerOptions{};
    rasterizerOptions.smoothShading = true;

    // Render Loop Options
    RenderLoopOptions renderOptions{};
    renderOptions.numFrames = 3;
    renderOptions.step_size = 60; // seconds
    std::string start_date = "2019-02-06T10:27:00";

    // Spice / Target Parameters
    std::string meta_kernel_path = "./vira_test_data/vulkan/meta_kernel.tm";
    std::string bennu_path = "./vira_test_data/vulkan/kernels/bennu/dsk/bennu_g_01680mm_alt_obj_0000n00000_v021.bds";
    float global_bennu_albedo = 0.4f;
    std::shared_ptr<vira::McEwen<TSpectral>> material = std::make_shared<vira::McEwen<TSpectral>>();

    // Sensor Parameters
    vira::SensorConfig<TSpectral> sensorConfig;
    sensorConfig.resolution = {1944,  2592 };
    sensorConfig.gain = .6f;
    sensorConfig.quantumEfficiency = TSpectral{ 0.6 };
    sensorConfig.size = { 4.2768f / 1000.f , 5.7f / 1000.f };

    const vk::Extent2D extent{WIDTH, HEIGHT};

    // Camera Parameters
    vira::CameraConfig<TSpectral, TFloat> cameraConfig;
    cameraConfig.focalLength = 7.64f / 1000.f;
    cameraConfig.exposureTime = EXPTIME;
    cameraConfig.renderPasses.render = true;
    cameraConfig.renderPasses.alpha = true;
    cameraConfig.renderPasses.depth = true;
    cameraConfig.renderPasses.globalNormal = true;
    cameraConfig.renderPasses.triangleID = true;
    cameraConfig.sensor = std::make_shared<vira::Sensor<TSpectral, TFloat>>(sensorConfig);
    cameraConfig.aperture = std::make_shared<vira::Aperture<TSpectral, TFloat>>(cameraConfig.focalLength / 3.f);
    cameraConfig.distortion = std::make_shared<vira::OpenCVDistortion<TSpectral, TFloat>>();

    // =======================
    // Perform Vira Scene Setup
    // =======================

    // Create the Scene object:
    vira::Scene<TSpectral, TFloat, TMeshFloat>* scene = new vira::Scene<TSpectral, TFloat, TMeshFloat>();
    scene->spice.furnsh(meta_kernel_path);

    // Create the viewportCamera object:
    vira::ViewportCamera<TSpectral, TFloat> viewportCameraOBJ = vira::ViewportCamera<TSpectral, TFloat>(cameraConfig);
    vira::ViewportCamera<TSpectral, TFloat>* viewportCamera = &viewportCameraOBJ;
    
    // Create light instance:
    std::shared_ptr<vira::Light<TSpectral, TFloat>> sun = scene->newSun();
    
    // Load the Bennu geometry and create instance:
    std::cout << "Reading Bennu DSK from: " << bennu_path << "...\n" << std::flush;
    vira::DSKOptions dskOptions;
    dskOptions.setAlbedo = global_bennu_albedo;
    vira::DSKInterface<TSpectral, TFloat, TMeshFloat> dskInterface;
    auto bennu_mesh = dskInterface.load(bennu_path, dskOptions);
    bennu_mesh->setMaterial(0, material);
    bennu_mesh->setSmoothShading(rasterizerOptions.smoothShading);
    auto bennu = scene->newInstance(bennu_mesh);
    
    // Configure SPICE:
    viewportCamera->setSpicePositionConfig("ORX_NAVCAM1", "BENNU");
    viewportCamera->setSpiceFrameConfig("ORX_NAVCAM1");
    sun->setSpicePositionConfig("SUN", "BENNU");
    bennu->setSpiceFrameConfig("IAU_BENNU");

    // Update SPICE poses:
    renderOptions.et = scene->spice.stringToET(start_date);
    viewportCamera->updateSpicePose(renderOptions.et);
    bennu->updateSpicePose(renderOptions.et);
    sun->updateSpicePose(renderOptions.et);
    scene->processSceneGraph();


    // =======================
    // Perform Vulkan Rasterizer Rendering
    // =======================

    // Create Vira Instance and Vira Device
    std::shared_ptr<vira::vulkan::InstanceFactoryInterface> instanceFactory = std::make_shared<vira::vulkan::VulkanInstanceFactory>();
    std::shared_ptr<vira::vulkan::DeviceFactoryInterface> deviceFactory = std::make_shared<vira::vulkan::VulkanDeviceFactory>();

    vira::vulkan::ViraDevice* viraDevice = new vira::vulkan::ViraDevice{instanceFactory, deviceFactory};
    std::unique_ptr<vira::vulkan::ViraSwapchain> viraSwapchain = std::make_unique<vira::vulkan::ViraSwapchain>(viraDevice, extent, false);        

    // Create Rasterizer
    vira::vulkan::VulkanRasterizer<TSpectral, TFloat, TMeshFloat>* rasterizer = new vira::vulkan::VulkanRasterizer<TSpectral, TFloat, TMeshFloat>(viewportCamera, scene, viraDevice, viraSwapchain.get(), rasterizerOptions);

    // Create Render loop
    vira::vulkan::ViraRenderLoop<TSpectral, TFloat, TMeshFloat>* renderLoop = new vira::vulkan::ViraRenderLoop<TSpectral, TFloat, TMeshFloat>(viraDevice, rasterizer, renderOptions);
    renderLoop->setViraSwapchain(std::move(viraSwapchain));

    // Run Render Loop with Benchmarking
    auto start = std::chrono::high_resolution_clock::now();

    renderLoop->runRasterizerRenderLoop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;    
    std::cout << "Render Loop Elapsed time: " << elapsed_seconds.count() << "s\n";

}