# Compile Shader code:
# This is modified from evilactually's GitHub Gist: 
# https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396'

if(MSVC OR MINGW)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
		message(STATUS "The value of CMAKE_HOST_SYSTEM_PROCESSOR is: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    	if (VIRA_USE_GLSLC)
    		set(GLSL_COMPILER "${VULKAN_SDK}/Bin/glslc.exe")
    	else()
    		set(GLSL_COMPILER "${VULKAN_SDK}/Bin/glslangValidator.exe")
    	endif()
    else()
		message(STATUS "The value of CMAKE_HOST_SYSTEM_PROCESSOR is: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
		if (VIRA_USE_GLSLC)
    		set(GLSL_COMPILER "${VULKAN_SDK}/Bin/glslc")
    	else()
    		set(GLSL_COMPILER "${VULKAN_SDK}/Bin/glslangValidator")
    	endif()
    endif()

elseif(UNIX AND NOT APPLE)
    if (VIRA_USE_GLSLC)
    	set(GLSL_COMPILER "glslc")
    else()
    	set(GLSL_COMPILER "glslangValidator")
    endif()

endif()

message("Shader compiler: ${GLSL_COMPILER}")

set(GLSL_SOURCE_FILES ${GLSL_SOURCE_FILES}
	vertex/pinhole.vert
	fragment/mcewan.frag
	pathtracer/ray_generation.glsl
	pathtracer/miss_radiance.glsl
	pathtracer/miss_shadow.glsl
	pathtracer/hit_radiance.glsl	
	pathtracer/hit_shadow.glsl	
	vertex/testVert.vert
	fragment/testFrag.frag
)

if (VIRA_USE_GLSLC)
    message("Running: ${GLSL_COMPILER} -fshader-stage=\${SHADER_STAGE} \${GLSL_INPUT} --target-env=vulkan1.2 -o \${SPIRV}")
else()
    message("Running: ${GLSL_COMPILER} -V -S \${SHADER_STAGE} --target-env vulkan1.2 \${GLSL_INPUT} -o \${SPIRV}")
endif()

foreach(GLSL ${GLSL_SOURCE_FILES})
	set(GLSL_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${GLSL})
	get_filename_component(FILE_NAME ${GLSL_INPUT} NAME)
	get_filename_component(FILE_NAME_ONLY ${GLSL_INPUT} NAME_WE)
    get_filename_component(FILE_EXT ${GLSL_INPUT} EXT)

    # Determine shader stage
    if(${FILE_EXT} STREQUAL ".vert")
        set(SHADER_STAGE "vert")
    elseif(${FILE_EXT} STREQUAL ".frag")
        set(SHADER_STAGE "frag")
    else()
		if(${GLSL} STREQUAL "pathtracer/ray_generation.glsl")
			set(SHADER_STAGE "rgen")

		elseif(${GLSL} STREQUAL "pathtracer/hit_radiance.glsl")
			set(SHADER_STAGE "rchit")

		elseif(${GLSL} STREQUAL "pathtracer/hit_shadow.glsl")
			set(SHADER_STAGE "rchit")

		elseif(${GLSL} STREQUAL "pathtracer/miss_radiance.glsl")
			set(SHADER_STAGE "rmiss")

		elseif(${GLSL} STREQUAL "pathtracer/miss_shadow.glsl")
			set(SHADER_STAGE "rmiss")

		else()
	        message(FATAL_ERROR "Unknown shader stage for ${GLSL_INPUT}")

		endif()
    endif()

	set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
	set(SPIRV "${SHADER_OUTPUT_DIR}/${FILE_NAME_ONLY}.spv")

	if(NOT EXISTS ${GLSL_INPUT})
		message (FATAL_ERROR "${GLSL_INPUT} not found.")
	endif()

	if (VIRA_USE_GLSLC)
		add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
			COMMAND ${GLSL_COMPILER} -fshader-stage=${SHADER_STAGE} ${GLSL_INPUT} --target-env=vulkan1.2  -o ${SPIRV}
			DEPENDS ${GLSL_INPUT})
	else()
	add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
			COMMAND ${GLSL_COMPILER} -V -S ${SHADER_STAGE} --target-env vulkan1.2 ${GLSL_INPUT} -o ${SPIRV}
			DEPENDS ${GLSL_INPUT})
	endif()
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

message("SPIR-V binary files: ${SPIRV_BINARY_FILES}")

add_custom_target(
	shaders
	DEPENDS ${SPIRV_BINARY_FILES}
)

install(FILES
    ${SPIRV_BINARY_FILES}
    DESTINATION ${CMAKE_BINARY_DIR}/shaders/
)
