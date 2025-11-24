#ifndef VIRA_HPP
#define VIRA_HPP

// Master header for including the complete vira API

// Provide the basic classes:
#include "vira/constraints.hpp"
#include "vira/logger.hpp"
#include "vira/math.hpp"
#include "vira/reference_frame.hpp"
#include "vira/rotation.hpp"
#include "vira/sampling.hpp"
#include "vira/scene.hpp"
#include "vira/spectral_data.hpp"
#include "vira/vec.hpp"


// Provide cameras:
#include "vira/cameras/camera.hpp"
#include "vira/cameras/apertures/aperture.hpp"
#include "vira/cameras/apertures/circular_aperture.hpp"
#include "vira/cameras/psfs/psf.hpp"
#include "vira/cameras/psfs/gaussian_psf.hpp"
#include "vira/cameras/psfs/airy_disk.hpp"
#include "vira/cameras/distortions/brown_distortion.hpp"
#include "vira/cameras/distortions/opencv_distortion.hpp"
#include "vira/cameras/distortions/owen_distortion.hpp"
#include "vira/cameras/noise_models/noise_model.hpp"
#include "vira/cameras/photosites/photosite.hpp"
#include "vira/cameras/filter_arrays.hpp"

// Provide DEMs:
#include "vira/dems/dem.hpp"
#include "vira/dems/dem_projection.hpp"
#include "vira/dems/georeference_image.hpp"
#include "vira/dems/set_proj_data.hpp"
#include "vira/dems/interfaces/gdal_interface.hpp"
#include "vira/dems/interfaces/spcmap_interface.hpp"

// Provide geometry:
#include "vira/geometry/mesh.hpp"

// Provide images:
#include "vira/images/color_map.hpp"
#include "vira/images/image.hpp"
#include "vira/images/compositing.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/images/interfaces/image_interface.hpp"

// Provide lights:
#include "vira/lights/light.hpp"

// Provide materials:
#include "vira/materials/material.hpp"
#include "vira/materials/lambertian.hpp"
#include "vira/materials/pbr_material.hpp"
#include "vira/materials/mcewen.hpp"

// Provide quipu:
#include "vira/quipu/dem_quipu.hpp"
#include "vira/quipu/star_quipu.hpp"

// Provide renderers:
#include "vira/rendering/cpu_rasterizer.hpp"
#include "vira/rendering/cpu_path_tracer.hpp"
#include "vira/rendering/cpu_unresolved_renderer.hpp"
#include "vira/rendering/passes.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/bresenham.hpp"

// Provoide Scene:
#include "vira/scene/group.hpp"
#include "vira/scene/ids.hpp"
#include "vira/scene/instance.hpp"
#include "vira/scene/lod_manager.hpp"

// Provide tools:
#include "vira/tools/geo_translate.hpp"
#include "vira/tools/load_robbins.hpp"

// Provide Unresolved Objects:
#include "vira/unresolved/magnitudes.hpp"
#include "vira/unresolved/star.hpp"
#include "vira/unresolved/star_catalogue.hpp"
#include "vira/unresolved/unresolved_object.hpp"
#include "vira/unresolved/interfaces/tycho2_interface.hpp"

// Provide Utilities:
#include "vira/utils/utils.hpp"

#endif