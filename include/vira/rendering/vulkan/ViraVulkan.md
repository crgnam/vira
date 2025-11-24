This Merge request is the final review of the Vulkan PathTracer and Rasterizer task deliverable. It is shown relative to the 'main' branch, which has been merged in to the 'vulkan' branch.

# <a href="#introduction-to-vira" id="user-content-introduction-to-vira" class="anchor" aria-hidden="true"></a>Introduction to Vira

Vira is a C++ image rendering library capable of producing a variety of color type and non-color type images from a scene using rasterization or pathtracing.

In a manner similar to GIANT, Vira makes use of a Scene class to manage the 'pose' (position and orientation) of mesh objects, light sources, and the cameras. It utilizes mesh instancing to create renderable objects with defined poses from the base mesh geometries defined by DSKs, DEMs, etc. Vira makes use of its renderer classes to execute rasterization or pathtracing of that scene to generate images, and its camera class manages image storage and output.

## <a href="#rendering-methods-overview" id="user-content-rendering-methods-overview" class="anchor" aria-hidden="true"></a>Rendering Methods Overview

### <a href="#path-tracers" id="user-content-path-tracers" class="anchor" aria-hidden="true"></a>Path Tracers

Path Tracers (sometimes referred to as Ray Tracers) dispatch sampled rays from each pixel of the camera to find intersection points (if any) with target mesh geometries. If an intersection is found, they calculate radiance contributions from each light source at that intersection point reflected along the image-target ray. They can then dispatch additional rays from the intersection point to test for shadowing and reflections of light sources. While similar to ray tracing, with the terms sometimes used interchangeably, path tracing encompasses the more complex photometric calculations and effects along the rays' paths.

### <a href="#rasterizers" id="user-content-rasterizers" class="anchor" aria-hidden="true"></a>Rasterizers

Rasterizers work in a fundamentally different manner than pathtracers when rendering a scene. Instead of tracing rays from the image pixels, rasterizers render scene objects one at a time, using provided frame transformations to calculate the positions of its mesh primitives (vertices, triangles, etc.) on the image plane and constructing "fragments" from them. Fragments can be thought of as the portion of the mesh primitive visible in a specific single pixel. Rasterizers then use the relative position and orientations of the fragment, light sources, and camera, to calculate the radiance of each fragment and write it to the correct pixel. Fragments are depth tested to determine which are visible and which are occluded by other fragments. Light-source relative depth testing can be used to implement shadowing effects as well.

## <a href="#existing-cpu-renderers" id="user-content-existing-cpu-renderers" class="anchor" aria-hidden="true"></a>Existing CPU Renderers

Vira has existing pathtracer and rasterizer classes that utilize the CPU alone to render the scene.

<a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/cpu_path_tracer.hpp" data-sourcepos="23:1-23:152">CPUPathTracer</a>

<a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/cpu_rasterizer.hpp" data-sourcepos="25:1-25:151">CPURasterizer</a>

## <a href="#gpu-vulkan-renderers" id="user-content-gpu-vulkan-renderers" class="anchor" aria-hidden="true"></a>GPU (Vulkan) Renderers

This delivery comprises two new renderer classes (and associated classes) that make use of the Vulkan GPU rendering framework to implement rasterization and pathtracing.

### <a href="#vulkanpathtracer" id="user-content-vulkanpathtracer" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan/vulkan_path_tracer.hpp" data-sourcepos="31:5-31:169">VulkanPathTracer</a>

The VulkanPathTracer class implements a similar pathtracing workflow as the CPUPathTracer, however it executes it on the GPU. The VulkanPathTracer extracts data from the Vira Scene and builds a Vulkan scene with gpu usable resources from it. It writes scene objects to Vulkan buffers, builds acceleration structures that manage the scene's geometry, and executes the pathtracing workflow using GPU parallelization. The GPU workflow is defined by GLSL shaders, each of which handles a different stage of rendering. extracts data from the Vira Scene and builds Vulkan usable resources. The VulkanPathTracer manages the overall geometry of the Vulkan scene, the camera and lighting sources, the mesh data and object instances, the rendering pipeline, and the GLSL shaders.

### <a href="#vulkanrasterizer" id="user-content-vulkanrasterizer" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan/vulkan_rasterizer.hpp" data-sourcepos="35:5-35:168">VulkanRasterizer</a>

The VulkanRasterizer class implements a similar rasterization workflow as the CPURasterizer, however it executes it on the GPU. It constructs Vulkan scene objects similar to the pathtracer's, but instead of using the acceleration structures of the path tracer, it manages the scene using Vertex and Instance buffers. The vertex buffer defines mesh geometries, and the instance buffer places instances of those meshes in the scene.

## <a href="#gpu-renderer-resources" id="user-content-gpu-renderer-resources" class="anchor" aria-hidden="true"></a>GPU Renderer Resources

There are a few ways to make Vulkan resources available to your Vulkan workflow. The primary method is to store them into Vulkan Buffers and bind those buffers to your rendering pipeline using Descriptor Sets. Descriptor Set Layouts define the layout/format of these sets, and Descriptor Pools allocate them.

### <a href="#descriptor-sets--bindings" id="user-content-descriptor-sets--bindings" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ada34b5ff6ff8d79907d4e6f313c3f923a42b60/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L71" data-sourcepos="43:5-43:193">Descriptor Sets / Bindings</a>

- Descriptors are the primary way to bind data to the Vulkan pipeline.
- They specify the format of this data and allocate resources to create the Vulkan descriptors
- They define and limit the accessibility of these buffers to different shader stages
- When the vulkan buffers are created, the read/write/copy access to the CPU is defined.

#### <a href="#vulkanpathtracer-descriptors" id="user-content-vulkanpathtracer-descriptors" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L59" data-sourcepos="50:6-50:196">VulkanPathTracer Descriptors</a>

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L68" data-sourcepos="52:3-52:171">Camera</a>: Camera parameters
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L122" data-sourcepos="53:3-53:188">Materials and Textures</a>: Material and texture data for per face material responses
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L221" data-sourcepos="54:3-54:181">Light Resources</a>: Scene light sources (poses, radiance)
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L256" data-sourcepos="55:3-55:184">Geometry Resources</a>: Top Level Acceleration Structure (TLAS)
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L289" data-sourcepos="56:3-56:180">Mesh Resources</a>: Vertex and index buffers for mesh instances
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L372" data-sourcepos="57:3-57:181">Image resources</a>: Vulkan images to write data to
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L414" data-sourcepos="58:3-58:179">Debug Buffers</a>: Vulkan vector and scalar buffers to store debug information from shaders

#### <a href="#vulkanrasterizer-descriptors" id="user-content-vulkanrasterizer-descriptors" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L388" data-sourcepos="60:6-60:196">VulkanRasterizer Descriptors</a>

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L398" data-sourcepos="62:3-62:171">Camera</a>: Camera parameters
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L453" data-sourcepos="63:3-63:187">Materials and Textures</a>: Material and texture data for per face material responses
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L552" data-sourcepos="64:3-64:180">Light Resources</a>: Scene light sources (poses, radiance)
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L587" data-sourcepos="65:3-65:179">Mesh Resources</a>: Per-mesh data including material indices/offsets
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/109ad996c88cc1b19285af56ff11ca9d0c6c883d/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L641" data-sourcepos="66:3-66:178">Debug Buffers</a>: Buffers to store debug information from shaders

### <a href="#other-pipeline-bindings" id="user-content-other-pipeline-bindings" class="anchor" aria-hidden="true"></a>Other Pipeline Bindings

Other Vulkan buffers can be bound to the pipeline directly as:

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/vulkan_render_resources.hpp#L119" data-sourcepos="72:3-72:183">Push Constants</a>: Smaller constant data to pushed to the rendering pipeline. These are useful for setting runtime parameters in the shaders.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/c85bc3220fbbd952bf09bcc71349f6d604f030c9/include/implementation/rendering/vulkan_private/swapchain.ipp#L427" data-sourcepos="73:3-73:179">Frame Buffers</a> (rasterizer pipeline): Hold color and depth images for writing output to from the rasterizer pipeline.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/c85bc3220fbbd952bf09bcc71349f6d604f030c9/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L230" data-sourcepos="74:3-74:178">Vertex, Index</a>, and <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/c85bc3220fbbd952bf09bcc71349f6d604f030c9/include/implementation/rendering/vulkan/vulkan_rasterizer.ipp#L338" data-sourcepos="74:185-74:355">Instance</a> buffers (rasterizer pipeline): Holds scene geometry for a rasterizer pipeline. Takes the place of the TLAS in the pathtracer pipeline.

### <a href="#camera-renderpass-images" id="user-content-camera-renderpass-images" class="anchor" aria-hidden="true"></a>Camera RenderPass Images

Though the two Vulkan renderers have slightly different bindings and resources, they each write the same output images to the Camera after rendering. The camera manages these images in its <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/passes.hpp#L39" data-sourcepos="78:190-78:335">RenderPasses</a> struct. These outputs are:
- receivedPowerImage ( Image&lt;TSpectral&gt;): Total power received by camera in pixel
- backgroundPowerImage ( Image&lt;TSpectral&gt;): Background power received by camera in pixel
- albedoImage ( Image&lt;TSpectral&gt;): Observed albedo
- depthImage ( Image&lt;float&gt; ): Mesh primitive depth in scene (for depth testing, etc.)
- cameraNormalImage ( Image&lt;ColorRGB&gt;): Facet normal in camera frame
- Image&lt;ColorRGB&gt; globalNormalImage: Facet normal in world frame
- alphaImage ( Image&lt;float&gt;): Alpha channel
- instanceIDImage (Image&lt;size\_t&gt;): Mesh instance ID of intersection
- meshIDImage (Image&lt;size\_t&gt;): Mesh (BLAS) ID of intersection
- triangleIDImage (Image&lt;size\_t&gt;): Mesh primitive (triangle) ID of intersection
- materialIDImage (Image&lt;size\_t&gt;): Material ID of intersection
- triangleSizeImage (Image&lt;float&gt;): Mesh primitive (triangle) size of intersection
- directRadianceImage( Image&lt;TSpectral&gt;): Direct radiance observed in pixel
- indirectRadianceImage( Image&lt;TSpectral&gt;): Indirect radiance observed in pixel

<a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/cameras/camera.ipp#L166" data-sourcepos="95:1-95:253">The camera then uses these images to implement the camera sensor's response to create a visual rendered image.</a>

### <a href="#common-resources" id="user-content-common-resources" class="anchor" aria-hidden="true"></a>Common Resources

The VulkanPathTracer and VulkanRasterizer share a good deal of their rendering resources, binding and using them in similar ways (the common resources and methods suggest a parent VulkanRenderer class could be created for the VulkanRasterizer and VulkanPathTracer to inherit from to reduce code replication):

#### <a href="#vulkancamera" id="user-content-vulkancamera" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/vulkan_camera.hpp" data-sourcepos="101:6-101:169">VulkanCamera</a>

This struct contains data members defining the camera's transformation matrices, position, optical properties, and distortion parameters used in rendering and simulations. It holds this data in a format easy to write to a Vulkan buffer. It extracts these parameters from a ViewportCamera, which is an extension of the Camera class with additional info required by Vulkan. The Camera is bound to the pipelines via a descriptor set in the same way for both renderers.

- **View and Projection Matrices:** 
    - viewMatrix: View transformation matrix. 
    - inverseViewMatrix: Inverse of the view matrix. 
    - projectionMatrix: Projection transformation matrix. 
    - normalMatrix: Normal transformation matrix. 
    - instrinsicMatrix: Camera intrinsic matrix. 
    - inverseInstrinsicMatrix: Inverse of the intrinsic matrix.
- **Camera Position and Orientation:** 
    - position: Camera’s position in world space. 
    - zDir: Z-axis direction of the camera.
- **Optical Properties:** 
    - focalLength: Camera focal length. 
    - aperture: Camera aperture size. 
    - aspectRatio: Aspect ratio of the camera. 
    - nearPlane: Near clipping plane distance. 
    - farPlane: Far clipping plane distance. 
    - focusDistance: Distance at which objects appear sharp in depth-of-field calculations. 
    - exposure: Exposure time of images. 
    - depthOfField: Enables or disables depth-of-field simulation.
- **Distortion Coefficients (OpenCV Model):** 
    - k1, k2, k3, k4, k5, k6: Radial distortion coefficients. 
    - p1, p2: Tangential distortion coefficients. 
    - s1, s2, s3, s4: Thin prism distortion coefficients.
- **Miscellaneous:** 
    - resolution: Camera resolution (width & height). 
    - padding1: Padding for 16-byte memory alignment.

#### <a href="#vulkanlight" id="user-content-vulkanlight" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/vulkan_light.hpp" data-sourcepos="132:6-132:167">VulkanLight</a>

This struct holds light parameters in a format writable to a Vulkan buffer.

- position: World-space position of the light.
- type: Light type identifier (\`0 = point light\`, \`1 = spherical light\`).
- color: Spectral intensity or radiance of the light (for point lights).
- radius: Radius of a spherical light (\`0\` for point lights).

#### <a href="#material-and-texture-resources" id="user-content-material-and-texture-resources" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/vulkan_material.hpp" data-sourcepos="141:6-141:189">Material and Texture Resources</a>

- Materials in Vira are indexed by triangle face, and each BLAS (mesh) can have multiple materials. There is some manual indexing needed to access the materials efficiently for each BLAS/face.
- Each Material defines a BSDF, a sampling method, and texture maps for 
    - Albedo 
    - Normal 
    - Roughness 
    - Metalness 
    - Transmission 
    - Emission
- These texture maps are stored in Vira as Images. From these, the <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L1359" data-sourcepos="151:68-151:291">pathtracer creates Vulkan images, image views, and samplers</a> as a <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/vulkan_render_resources.hpp#L458" data-sourcepos="151:298-151:476">TextureImage</a> object, and bound via the material descriptor set.
- Some of these texture maps are of a SpectralData pixel type. For N&gt;4, additional texture maps are added per variable to handle the additional bands. (see 'Color Representation' section below)

### <a href="#differing-resources" id="user-content-differing-resources" class="anchor" aria-hidden="true"></a>Differing Resources

#### <a href="#meshinstance-data" id="user-content-meshinstance-data" class="anchor" aria-hidden="true"></a>Mesh/Instance Data

The Vulkan Pathtracer manages mesh and instance data using Vulkan <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp#L787" data-sourcepos="158:67-158:273">Bottom Level Acceleration Structures (BLAS)</a> and a <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blame/vulkan/include/implementation/rendering/vulkan/vulkan_path_tracer.ipp?ref_type=heads&amp;page=2#L1201" data-sourcepos="158:281-158:473">Top Level Acceleration Structure (TLAS)</a>. These Vulkan objects are built using vertex, index, and instance buffers. The underlying resources of the TLAS are generally updated and modified between render passes by rebuilding the TLAS. Ideally, the TLAS poses could be updated without rebuilding the TLAS to save overhead, however that overhead is small and currently the TLAS is rebuilt between frames.

The Vulkan Rasterizer on the other hand does not need these ray tracing acceleration objects, as it renders each mesh primitive triangle individually and does not execute any ray tracing. It provides similar vertex, index, and instance buffers, but does not build acceleration objects from them.

#### <a href="#output-images" id="user-content-output-images" class="anchor" aria-hidden="true"></a>Output Images

The Vulkan PathTracer binds output images using a descriptor set. It uses separate image layers to store different data, such as direct radiance, alpha, depth, received power, etc.

The Vulkan Rasterizer pipeline does not use layered images, but instead binds separate output images directly using a FrameBuffer with multiple color attachments and depth attachments.

#### <a href="#pipeline" id="user-content-pipeline" class="anchor" aria-hidden="true"></a>Pipeline

The Vulkan PathTracer must use a Vulkan raytracing pipeline, while the Vulkan Rasterizer uses a general graphics pipeline. These pipelines differ in their stages, general programmatic flow, and bindings.

#### <a href="#shaders" id="user-content-shaders" class="anchor" aria-hidden="true"></a>Shaders

Shaders define the logic that is run on the GPU during rendering. They are written in GLSL (which has similar syntax to C++), and are compiled to .spv files which are then loaded into Vira. PathTracer and Rasterizer pipelines have different shaders stages (sets of shaders for a specific purpose) that handle different responsibilities, and different programmatic flows between them. Shader stages communicate using shared and passed resources, including descriptor bindings, push constants, and payload variables.

##### <a href="#pathtracer-shaders" id="user-content-pathtracer-shaders" class="anchor" aria-hidden="true"></a>PathTracer Shaders

PathTracer shaders implement the raytracing and light & material effects. The pathtracer dispatches two types of rays: a 'Radiance' ray that contributes to the radiance of the path, and a 'Shadow' ray that tests shadowing between a target and light source. Primary and multi-bounce rays are of type 'Radiance.' Shadow rays are dispatched from primary ray intersections. The pathtracer currently has five shaders, though more complex rendering cases or additional material models may necessitate additional calculation or hit shaders in the future:

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/source/shaders/pathtracer/ray_generation.glsl" data-sourcepos="180:3-180:165">Ray Generation Shader</a>: 
    - Creates and dispatches the primary rays, and is called once for each pixel. 
    - Handles frame transformations for rays. 
    - Applies distortion models to ray directions. 
    - Combines the response of the multiple rays issued per pixel to generate a pixel value. 
    - Writes the pixel value to the output image pixel directly.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/source/shaders/pathtracer/miss_background.glsl" data-sourcepos="186:3-186:165">Radiance Miss Shader</a>: 
    - Defines miss (background) response for primary and multi-bounce rays when the ray does not intersect a triangle.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/source/shaders/pathtracer/closest_hit.glsl" data-sourcepos="188:3-188:160">Radiance Hit Shader</a>: 
    - Defines the renderer response when a primary or multi-bounce ray hits a triangle. 
    - Does all interaction/intersection material responses and returns the photometric response of each ray. 
    - The hit shaders are material properties and textures are sampled, and the BSDF is applied. 
    - Manual interpolation of vertex data to the hit point is done here as well.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/f8df966d87a8809e7b6ca49c39bcdb545f005e4e/source/shaders/pathtracer/miss_radiance.glsl" data-sourcepos="193:3-193:161">Shadow Miss Shader</a>: 
    - Minimal shader that returns a boolean specifying that the shadow ray has not hit another primitive, signifying the primary ray is not shadowed.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/f8df966d87a8809e7b6ca49c39bcdb545f005e4e/source/shaders/pathtracer/hit_shadow.glsl" data-sourcepos="195:3-195:158">Shadow Hit Shader:</a> 
    - Minimal shader that returns a boolean specifying that the shadow ray has hit another primitive, signifying the primary ray is shadowed.

###### <a href="#development-notes" id="user-content-development-notes" class="anchor" aria-hidden="true"></a>Development Notes:

Recent pathtracer shader development has focused on the following:
- Bindings debugging
- Ray geometry implementation and debugging 
    - Ray creation debugging 
- Distortion 
    - Depth of field 
    - Shadowing
- Material response development and debugging 
    - Texture samplers
- Buffer writing and data payload communication between shader stages

##### <a href="#rasterizer-shaders" id="user-content-rasterizer-shaders" class="anchor" aria-hidden="true"></a>Rasterizer Shaders

Rasterizer shaders implement the projection of mesh elements to the image plane / pixel grid, the interpolation of vertex parameters for triangle faces, and light & material effects. The Rasterizer has two shader stages:

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/source/shaders/vertex/pinhole.vert" data-sourcepos="216:3-216:195">Vertex Shaders (current implementations working: pinhole.vert)</a>: 
    - These handle the geometry of the render, projecting vertices onto the image frame which are used by the pipeline to produce renderable fragments, which represent the part of a mesh triangle that is present in a particular pixel. 
    - In this shader, you can set the value of fragment variables for each vertex, which will then be interpolated by Vulkan to produce a value for the triangle and therefore the fragment. These interpolated values are then available in the fragment shader.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/source/shaders/fragment/lambert.frag" data-sourcepos="219:3-219:199">Fragment Shaders (current implementations working: lambert.frag)</a>: 
    - After the Vertex shader does its job, a fragment is produced and dispatched to the fragment shader. 
    - Fragment shaders use the interpolated and direct values from the vertex shader to compute and apply lighting and material effects. 
    - Fragment shaders are where material properties and textures are sampled, and the BSDF is applied. 
    - Lens distortion can be applied in the fragment shader, or in post-processing. Distortion for the rasterizer shaders has not been implemented yet. 
    - Fragment shaders return output variables which are written to the pixel being rendered to. 
    - The pipeline manages depth calculations.

###### <a href="#development-notes-1" id="user-content-development-notes-1" class="anchor" aria-hidden="true"></a>Development Notes:

- Distortion has not yet been implemented fully in the rasterizer shaders.
- Shadow testing has not yet been implemented in the rasterizer shaders.

# <a href="#color-representation" id="user-content-color-representation" class="anchor" aria-hidden="true"></a>Color Representation

### <a href="#spectraldata" id="user-content-spectraldata" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/spectral_data.hpp" data-sourcepos="234:5-234:143">SpectralData</a>

A major part of the recent development has been on the usage and management of SpectralData classes for light/color representation within Vulkan rendering and resources. This affected any data or resource that had a SpectralData type, such as albedos, radiances, or certain camera parameters like optical efficiency.

The SpectralData class defines a spectral color's bands and provides operators and constructors for spectral color.

In Vulkan, a four element vector, vec4, is the preferred or required type for non-scalar values, and is the largest vector type supported in vk::Images. Representing data as vec4s is also highly optimized. For this reason, spectral data in the Vulkan pathtracer is generally represented by multiple vec4s, with the last one padded if nSpectral is not a multiple of four. In the shader logic, these vec4s should be processed individually and recombined on the CPU side for post-processing. This affects the following: 
- Textures
- Vertex Albedos
- Output Images
- Light Source color

###### <a href="#development-notes-2" id="user-content-development-notes-2" class="anchor" aria-hidden="true"></a>Development Notes:

Currently, support for spectral numbers &gt; 4 are not fully implemented in the shaders, but the necessary structures have been set up to allow for future implementation, and merely requires a bit of indexing work in the shaders to complete.

# <a href="#control-classes" id="user-content-control-classes" class="anchor" aria-hidden="true"></a>Control Classes:

## <a href="#vulkanrenderloop" id="user-content-vulkanrenderloop" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/render_loop.hpp" data-sourcepos="253:4-253:169">VulkanRenderLoop</a>

- The VulkanRenderLoop class manages the top level rendering loop and controls the programmatic flow of the individual render passes. It coordinates the Vira Scene updating and writing the output to the camera, and makes the actual submit calls to the command buffers to execute Vulkan rendering.
- The primary calls to VulkanRenderLoop are 
    - runPathTracerRenderLoop 
    - runRasterizerRenderLoop
- Once the Vira scene is created and the Vulkan renderer and resources are set up, a renderloop is instantiated and initialized, and the above run commands are issued to execute the <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/render_loop.ipp#L204" data-sourcepos="259:183-259:368">pathtracer renderloop</a> or the <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/render_loop.ipp#L101" data-sourcepos="259:377-259:562">rasterizer renderloop</a>.

###### <a href="#development-notes-3" id="user-content-development-notes-3" class="anchor" aria-hidden="true"></a>Development Notes:

- VulkanRenderloop currently has some methods that have generalized names but are only used for the VulkanRasterizer render loop.
 Bringing the two class strategies together should be done if VulkanRasterizer and VulkanPathTracer are made to inherit the same parent class, VulkanRenderer.

- The most recent work comprises the implementation, debugging, and refinement of the following

## <a href="#viraswapchain" id="user-content-viraswapchain" class="anchor" aria-hidden="true"></a><a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/vira/rendering/vulkan_private/swapchain.hpp" data-sourcepos="268:4-268:164">ViraSwapchain</a>

The VulkanSwapchain class manages the output image resources reused across subsequent render passes. It creates the Vulkan image and/or frame buffers the Vulkan renderers use to write their image data to. It also creates and helps manage the synchronization objects necessary to deconflict usage of these resources across multiple frames that may or may not be worked on simultaneously.

The primary synchronization control object in Vulkan is a vk::SwapchainKHR object, however this level of synchronization is only required when rendering dynamically onscreen. For the current offscreen (to file) rendering workflows, it is not used, however the ViraSwapchain class still manages the lesser degree of synchronization still required.

###### <a href="#development-notes-4" id="user-content-development-notes-4" class="anchor" aria-hidden="true"></a>Development Notes:

Recent Development on the ViraSwapchain has focused on image resource creation for the pathtracer and rasterizer renderers. These classes store image data in different manners, and the ViraSwapchain accommodates both.

# <a href="#helper-classes" id="user-content-helper-classes" class="anchor" aria-hidden="true"></a>Helper Classes

These handle the profiling and utilization of the available Vulkan physical devices, as well as the creation and management of fundamental Vulkan objects. They are the plumbing of the Vira Vulkan render system. The helper classes are:

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/buffer.ipp" data-sourcepos="282:3-282:167">ViraBuffer</a>: Manages buffer creation, memory allocation, writing, and retrieval.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/device.ipp" data-sourcepos="283:3-283:167">ViraDevice</a>: Creates and manages the Vulkan logical device, the main controller of Vulkan.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/window.ipp" data-sourcepos="284:3-284:167">ViraWindow</a>: Creates and manages a GLFW window for onscreen rendering (onscreen rendering not currently implemented);
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/pipeline.ipp" data-sourcepos="285:3-285:171">ViraPipeline</a>: Creates and manages the Vira pipeline and its bindings.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/descriptors.ipp#L37" data-sourcepos="286:3-286:189">ViraDescriptorSetLayout</a>: Defines the layout/format of a descriptor set.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/descriptors.ipp#L104" data-sourcepos="287:3-287:185">ViraDescriptorPool</a>: Allocates descriptor sets.
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/descriptors.ipp#L166" data-sourcepos="288:3-288:187">ViraDescriptorWriter</a>: Writes descriptor sets.

# <a href="#vulkan-interfaceimplementation-classes" id="user-content-vulkan-interfaceimplementation-classes" class="anchor" aria-hidden="true"></a>Vulkan Interface/Implementation Classes  

The Vira Vulkan codebase is also made up of interfaces/implementations of certain Vulkan classes/functions to allow for Mock classes during testing. This creates an additional layer of abstraction, however the interfaces are one-to-one, as they mirror the Vulkan function calls. These interface wrappers also allow for the use of a dynamic dispatcher for Vulkan calls, which is added to all Vulkan calls in the interface implementations. These interfaces/implementations have had additions made when further functionality was required from them, as they define only the required Vulkan functionality. The Vulkan classes wrapped in interfaces are:

- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/render_loop.ipp#L204" data-sourcepos="294:3-294:190">vk::UniqueCommandBuffer</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/command_pool/vulkan_command_pool.ipp" data-sourcepos="295:3-295:221">vk::UniqueCommandPool</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/device/vulkan_device.ipp" data-sourcepos="296:3-296:204">vk::UniqueDevice</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance.ipp" data-sourcepos="297:3-297:210">vk::UniqueInstance</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.ipp" data-sourcepos="298:3-298:210">vk::UniqueDeviceMemory</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.ipp" data-sourcepos="299:3-299:206">vk::PhysicalDevice</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/queue/vulkan_queue.ipp" data-sourcepos="300:3-300:195">vk::Queue</a>
- <a href="https://gitlab.ad.kinetx.com/opnav/gsfc/vira/-/blob/6ab2e5de297584c543c9b7cb2ad5cc599e9db17f/include/implementation/rendering/vulkan_private/vulkan_interface/swapchain/vulkan_swapchain.ipp" data-sourcepos="301:3-301:216">vk::UniqueSwapchainKHR</a>

# <a href="#vulkan-definitions" id="user-content-vulkan-definitions" class="anchor" aria-hidden="true"></a>Vulkan Definitions

**Instance**: Represents the connection between your application and the Vulkan library. Initializes Vulkan and sets up the environment for all subsequent Vulkan operations.

**PhysicalDevice**: Represents a physical GPU in the system. Provides information about the GPU’s capabilities and features.

**Device**: Represents a logical connection to a physical GPU. Manages resources and issues commands to the GPU, enabling rendering and compute operations.

**QueueFamily**: Represents a group of queues that support specific types of operations (graphics, compute, transfer). Allows selection of appropriate queues based on the required operations.

**Queue**: The execution engine of the GPU, processing and executing commands submitted by the application. Multiple queues of the same type can be used for parallel workloads.

**CommandBuffer**: A recorded list of commands for the GPU to execute. Commands in a CommandBuffer are submitted to a Queue for execution.

**Pipeline**: Represents a sequence of operations that the GPU performs for rendering or computing. Configures shaders, fixed-function state, and other stages for efficient execution.

**ShaderModule**: Represents compiled shader code. Provides the GPU with code for programmable stages of the pipeline (e.g., vertex or fragment shaders).

**RenderPass**: Encapsulates a sequence of rendering operations on framebuffers. Defines how rendering occurs, including attachments and subpasses, optimizing memory usage.

**Framebuffer**: Represents a collection of image views for the GPU to render into. Serves as the memory target for rendering operations defined by a RenderPass.

**Buffer**: Represents a block of memory for storing data (vertices, indices, uniform values). Facilitates data transfer to and from the GPU for rendering and compute operations.

**Image**: Represents a GPU resource for storing pixel data (textures, render targets). Used for rendering, texturing, or storage in shaders.

**ImageView**: A view into an Image, describing how the image data should be accessed. Defines specific parts or aspects of an Image for use in shaders or as render targets.

**DescriptorSet**: Represents a collection of resources (buffers, textures) that shaders can access. Manages and binds resources to pipelines for shader access.

**Semaphore** **and** **Fence**: Synchronization primitives. Manage synchronization between GPU operations and between CPU and GPU, ensuring proper command execution order.

**Surface**: Represents a platform-specific rendering target (typically a window). Interfaces between Vulkan and the window system, enabling image presentation on the screen.

**SwapChain**: Manages the presentation of rendered images to the screen. Handles the series of images used for rendering and display, typically in a double- or triple-buffered setup.

**Window**: Represents the surface where the rendered output is displayed. Provides the interface between Vulkan and the operating system's windowing system.

# <a href="#guide-to-levels-of-abstraction-in-vira-vulkan" id="user-content-guide-to-levels-of-abstraction-in-vira-vulkan" class="anchor" aria-hidden="true"></a>Guide to Levels of Abstraction in Vira Vulkan

Vira Vulkan uses interfaces and implementations of Vulkan classes, factory interfaces, real and mock Vulkan implementations, and custom wrapper classes. This layered approach allows for flexible, testable code with clear separation between the Vulkan driver and higher-level application logic.

#### <a href="#custom-wrapper-classes" id="user-content-custom-wrapper-classes" class="anchor" aria-hidden="true"></a>Custom Wrapper Classes

These classes manage Vulkan interactions at the highest level. They encapsulate complex Vulkan operations in a user-friendly manner, providing methods for resource management, rendering, and synchronization. These classes often use unique Vulkan handles (vk::UniqueDevice,vk::UniqueSwapchainKHR, etc.) to ensure that resources are automatically cleaned up when no longer needed. They interact with both real and mock implementations through interfaces to provide flexibility and testing capabilities. 

#### <a href="#factory-interfaces-real-and-mock-implementations" id="user-content-factory-interfaces-real-and-mock-implementations" class="anchor" aria-hidden="true"></a>Factory Interfaces (Real and Mock Implementations)

Factory interfaces are used to create instances of Vulkan objects. These interfaces decouple object creation from the code that uses them, making it easier to switch between real and mock implementations. Real factories create actual Vulkan objects, while mock factories create test objects, enabling unit testing of higher-level logic. 

#### <a href="#real-and-mock-vulkan-implementations" id="user-content-real-and-mock-vulkan-implementations" class="anchor" aria-hidden="true"></a>Real and Mock Vulkan Implementations

These classes implement interfaces that represent various Vulkan components (e.g., devices, queues, command buffers). Real implementations wrap actual Vulkan objects, while mock implementations provide testable behavior. These classes can manage both raw Vulkan handles (e.g., \`VkDevice\`) and C++ wrappers (e.g., \`vk::Device\`).

#### <a href="#vulkan-c-api-vkuniquedevice-vkinstance-etc" id="user-content-vulkan-c-api-vkuniquedevice-vkinstance-etc" class="anchor" aria-hidden="true"></a>Vulkan C++ API (vk::UniqueDevice, vk::Instance, etc.)

The Vulkan C++ API provides RAII wrappers for Vulkan handles, ensuring automatic cleanup of resources. These classes such as vk::UniqueDevice and vk::Instance provide an object-oriented interface for Vulkan operations while managing the underlying Vulkan handles' lifecycles.

#### <a href="#raw-vulkan-handles-vkdevice-vkinstance-etc" id="user-content-raw-vulkan-handles-vkdevice-vkinstance-etc" class="anchor" aria-hidden="true"></a>Raw Vulkan Handles (VkDevice, VkInstance, etc.)

Raw Vulkan handles, such as VkDevice and VkInstance, are opaque pointers that represent Vulkan resources. They are managed by the Vulkan driver, and their lifecycles are explicitly controlled by Vulkan API functions like \`vkCreateDevice\` and \`vkDestroyDevice\`.

#### <a href="#vulkan-drivergpu-resources" id="user-content-vulkan-drivergpu-resources" class="anchor" aria-hidden="true"></a>Vulkan Driver/GPU Resources

At the lowest level, the Vulkan driver manages the actual GPU resources. These are represented by the raw handles, but their memory and execution are controlled by the Vulkan driver and hardware. Applications never directly manage these resources but interact with them through Vulkan API calls.

# <a href="#current-limitations--future-work" id="user-content-current-limitations--future-work" class="anchor" aria-hidden="true"></a>Current Limitations & Future Work

| Feature                | Pathtracer                                                                                                         | Rasterizer                                                                                                          |
|------------------------|--------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------|
| Distortion             | OpenCV is the only model implemented. To add other models: <br> - VulkanCamera class needs to be generalized for other distortion models, as it currently binds OpenCV coefficients directly. <br> - VulkanCamera should either become a parent class to different distortion model cameras, or the bindings should be reworked to accept a distortion model rather than individual coefficients. <br> - The camera binding definitions in the vulkan shaders needs to be generalized for distortion models similarly to VulkanCamera class on the CPU side | Distortion not yet implemented. Functions are present in the shaders but not used in the vertex shader main.      |
| Shadow Testing         | Shadow testing is implemented but has a bug: <br> - In the hit_radiance shader, there is a commented out block of code that dispatches shadow rays. <br> - When uncommented, this block breaks the shader logic, and prevents the radiance rays from being dispatched correctly. <br> - Shadow rays are defined as a second type of ray and call different shaders upon intersection with a primitive. <br> - These shadow shaders are added to the shader groups in the Shader Binding Table (SBT). <br> - It is likely that the bug stems from either an error in the SBT or the definitions of ray types. | Shadow testing is not implemented. If needed, shadow rays should be dispatched from light sources to establish light source relative depth maps |
| Spectral Data           | Structures and methods for n>4 spectral data are partially implemented. <br> - The management of spectral sets, including breaking color data into sets and stitching it back together, and binding these additional sets to the pipeline, are fully implemented on the CPU side for the pathtracer. <br> - On the shader side, the following work is needed to complete implementation: <br> - In the hit_radiance shader, a loop over the spectral sets should be added around the radiance calculations. <br> - Additionally, in the hit_radiance shader a loob over spectral sets should be added around the sampling of color-type textures (currently albedo and emission) <br> - In the ray_generation shader, a loop over the spectral sets should be added around the received power calculations and output to the color-type data. <br> - This affects all color-type variables, including albedo, radiance (direct and indirect), power (received and background) as well as the albedo and emission textures.    | Implementing n>4 spectral color in the rasterizer pipeline would require creating additional framebuffers for each spectral set, and writing and computing the additional spectral set data in the shaders. |   
| Materials               | McEwan material model is fully implemented, but not all textures are utilized in shaders.   | McEwan model implemented, but texture sampling has a bug and returns incorrect values.  |
| Multi-bounce            | Multi-bounce is not implemented. Future development would require dispatching additional radiance rays from the hit_radiance shader.          | Not applicable.                                                                                                    |
| Partially Opacity | Partial opacity is implemented only in the sense that the shaders will set the alpha channel to a fractional value if only some of the sampled rays of the pixel intersect a primitive.  | Partial opacity is not calculated/supported. Alpha values are set to 0 or 1.  |
| Onscreen Rendering      | Not yet implemented. Framework present in ViraSwapchain.  | <-- |
| Render Loop Bottlenecks | Biggest bottleneck is copying rendered data from GPU to CPU memory. SBT resources need optimization for frame synchronization. ViraSwapchain resources could be refined for better synchronization with CPU image writing and multiple frame buffers. | <-- |

