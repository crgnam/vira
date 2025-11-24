#!/usr/bin/env python3
import sys
import os
from pathlib import Path

# Import the vira bindings - using V8FF submodule (Visible_8bin, float, float)
import virapy as vira

def enable_print_status():
    """Enable print status (equivalent to vira::enablePrintStatus())"""
    # This function may need to be implemented in your bindings
    # For now, we'll skip it or you can add it to the C++ bindings
    pass

def pad_zeros(number, width=3):
    """Utility function to pad zeros (equivalent to vira::utils::padZeros<3>(i))"""
    return f"{number:0{width}d}"

def velocity_to_rgb(velocity_image):
    """Convert velocity to RGB (equivalent to vira::images::velocityToRGB)"""
    # This function may need to be implemented in your bindings
    # For now, we'll return the velocity_image as-is
    return velocity_image

def main():
    # ========================== #
    # === Validate the Input === #
    # ========================== #
    if len(sys.argv) != 2:
        print("This example requires passing the meta_kernel.tm filepath as an argument", file=sys.stderr)
        return 1

    filepath = Path(sys.argv[1])
    if not filepath.exists():
        print(f"The meta_kernel.tm does not exist at: {filepath}", file=sys.stderr)
        return 2

    # Get the file path of the Bennu DSK
    dsk_file_name = "bennu_g_01680mm_alt_obj_0000n00000_v021.bds"
    bennu_path = filepath.parent / "../../data/kernels/bennu/dsk" / dsk_file_name
    if not bennu_path.exists():
        print(f"The {dsk_file_name} file does not exist at: {bennu_path}", file=sys.stderr)
        return 2

    # Compute the tycho2_file
    tycho2_file = filepath.parent / "../../data/tycho2/tycho2.qsc"

    try:
        # Activate the submodule       
        enable_print_status()
        
        # ======================== #
        # === Create the Scene === #
        # ======================== #
        scene = vira.Scene()
        scene.spice.furnsh_relative_to_file(str(filepath))

        # ============================ #
        # === Configure the Camera === #
        # ============================ #
        navcam = scene.new_camera()
        scene[navcam].enable_parallel_initialization()
        scene[navcam].set_focal_length(7.6 / 1000.0)
        scene[navcam].set_f_stop(5.6)
        scene[navcam].set_resolution(1944, 2592)
        scene[navcam].set_sensor_size(4.2768 / 1000.0, 5.7 / 1000.0)
        scene[navcam].set_default_photosite_quantum_efficiency(0.6)
        scene[navcam].set_default_photosite_linear_scale_factor(55)
        scene[navcam].set_exposure_time(0.001)

        # ====================== #
        # === Create the Sun === #
        # ====================== #
        sun = scene.new_sun()

        # ========================= #
        # === Load the Geometry === #
        # ========================= #
        # Load the Bennu geometry and create instance
        scene.dsk_interface.set_albedo = 0.04
        loaded_assets = scene.load_dsk(str(bennu_path))
        bennu_mesh = loaded_assets.mesh_ids[0]
        scene[bennu_mesh].set_smooth_shading(True)

        # Create instances and add to scene
        bennu = scene.new_instance(bennu_mesh)

        # =============================== #
        # === Read the Star Catalogue === #
        # =============================== #
        # et = scene.spice.string_to_et("2022-12-09T17:29:22")
        # scene.load_tycho_quipu(str(tycho2_file), et)

        # ======================= #
        # === Configure SPICE === #
        # ======================= #
        scene.set_spice_names("BENNU", "J2000")
        scene[navcam].set_spice_names("ORX_NAVCAM1", "ORX_NAVCAM1")
        scene[sun].set_spice_names("SUN")
        scene[bennu].set_spice_names("BENNU", "IAU_BENNU")

        # ============================ #
        # === Configure Pathtracer === #
        # ============================ #
        scene.pathtracer.options.samples = 3
        scene.pathtracer.options.bounces = 0
        scene.pathtracer.options.adaptive_sampling = True

        scene.pathtracer.render_passes.simulate_lighting = True
        scene.pathtracer.render_passes.save_global_velocity = True
        scene.pathtracer.render_passes.save_camera_velocity = True

        # ============================ #
        # === Configure Rasterizer === #
        # ============================ #
        scene.rasterizer.render_passes.simulate_lighting = True
        scene.rasterizer.render_passes.save_global_velocity = True
        scene.rasterizer.render_passes.save_camera_velocity = True

        # ========================= #
        # === Perform Rendering === #
        # ========================= #
        step_size = 10 * 60  # seconds
        scene.set_spice_datetime("2019-02-06T10:27:00")
        
        # Create output directories
        os.makedirs("vira_orex_output/raytrace", exist_ok=True)
        os.makedirs("vira_orex_output/rt_camera_velocity", exist_ok=True)
        os.makedirs("vira_orex_output/rt_global_velocity", exist_ok=True)
        os.makedirs("vira_orex_output/raster", exist_ok=True)
        os.makedirs("vira_orex_output/camera_velocity", exist_ok=True)
        os.makedirs("vira_orex_output/global_velocity", exist_ok=True)
        
        for i in range(10):
            output_name = f"frame_{pad_zeros(i)}.png"

            # Pathrace
            raytraced_image = scene.render(navcam)
            scene.image_interface.write(f"vira_orex_output/raytrace/{output_name}", raytraced_image)
            scene.image_interface.write(
                f"vira_orex_output/rt_camera_velocity/{output_name}", 
                velocity_to_rgb(scene.pathtracer.render_passes.camera_velocity_image)
            )
            scene.image_interface.write(
                f"vira_orex_output/rt_global_velocity/{output_name}", 
                velocity_to_rgb(scene.pathtracer.render_passes.global_velocity_image)
            )

            # Rasterize
            rasterized_image = scene.rasterize_render(navcam)
            scene.image_interface.write(f"vira_orex_output/raster/{output_name}", rasterized_image)
            scene.image_interface.write(
                f"vira_orex_output/camera_velocity/{output_name}", 
                velocity_to_rgb(scene.rasterizer.render_passes.camera_velocity_image)
            )
            scene.image_interface.write(
                f"vira_orex_output/global_velocity/{output_name}", 
                velocity_to_rgb(scene.rasterizer.render_passes.global_velocity_image)
            )

            # Update the SPICE frames
            scene.increment_spice_et(step_size)

    except Exception as ex:
        print(f"Error: {ex}")
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())