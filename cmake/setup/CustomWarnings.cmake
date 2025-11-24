# Helper function to add warnings that should always be disabled
function(add_always_disabled_warnings target)
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} INTERFACE 
            # Style/layout warnings that don't affect correctness (ALWAYS OFF):
            /wd5246 # brace initialization (style, not correctness)
            /wd4820 # padding warnings (layout, not correctness)
            /wd4710 # function not inlined (optimization, not correctness)
            /wd4711 # function selected for automatic inline expansion (optimization, not correctness)
            /wd4371 # class layout changes (compiler optimization)
            /wd5045 # Spectre mitigation (security feature, not code issue)
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        include(CheckCXXCompilerFlag)
        
        # Style/layout warnings to disable (ALWAYS OFF)
        set(ALWAYS_DISABLED_WARNINGS
            "newline-eof"
            "double-promotion"
            "covered-switch-default"
            "float-equal"
            "disabled-macro-expansion"
            "unsafe-buffer-usage"
        )
        
        foreach(WARNING ${ALWAYS_DISABLED_WARNINGS})
            check_cxx_compiler_flag("-W${WARNING}" HAS_WARNING_${WARNING})
            if(HAS_WARNING_${WARNING})
                target_compile_options(${target} INTERFACE "-Wno-${WARNING}")
            endif()
        endforeach()
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} INTERFACE 
            # Style warnings that don't affect correctness (ALWAYS OFF):
            -Wno-double-promotion
            -Wno-float-equal # Vira uses this for specific checks
        )
    endif()
endfunction()

# Now the main warning configuration
if (VIRA_ALL_SELECTED_WARNINGS)
    # COMPREHENSIVE WARNING SET
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # MSVC comprehensive warnings
        target_compile_options(vira INTERFACE 
            /Wall 
            /w14263 # Enable warning for missing override
            /w14242 # Enable warning for extra semicolons
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang comprehensive warnings
        target_compile_options(vira INTERFACE
            -Wall -Wextra
            -Wconversion -Wsign-conversion -Wcast-qual -Wcast-align
            -Wunused -Wuninitialized -Winit-self
            -Wnull-dereference -Wformat=2
            -Wimplicit-fallthrough
            -Winconsistent-missing-override
        )
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC comprehensive warnings
        target_compile_options(vira INTERFACE 
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion -Wcast-qual -Wcast-align
            -Wunused -Wuninitialized -Winit-self -Wlogical-op
            -Wduplicated-cond -Wduplicated-branches -Wrestrict
            -Wnull-dereference -Wformat=2
            -Wimplicit-fallthrough
            -Wsuggest-override
        )
        
    else()
        # Fallback for other compilers
        target_compile_options(vira INTERFACE -Wall -Wextra)
    endif()
    
else()
    # BASIC WARNING SET (but still useful!)
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(vira INTERFACE 
            /W4  # Use /W4 instead of just /Wall to get more warnings
            /w14263  # Enable warning for missing override
            /w14242  # Enable warning for extra semicolons
        )
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Basic but still comprehensive set
        target_compile_options(vira INTERFACE
            -Wall -Wextra
            -Wconversion -Wsign-conversion  # Keep these - they catch real bugs!
            -Wcast-qual -Wcast-align
            -Winconsistent-missing-override
        )
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(vira INTERFACE 
            -Wall -Wextra
            -Wconversion -Wsign-conversion  # Keep these - they catch real bugs!
            -Wcast-qual -Wcast-align
            -Wsuggest-override
        )
        
    else()
        # Fallback for other compilers
        target_compile_options(vira INTERFACE -Wall -Wextra)
    endif()
endif()

# ALWAYS apply the disabled warnings (regardless of VIRA_ALL_SELECTED_WARNINGS)
add_always_disabled_warnings(vira)

# Function to safely disable backwards compatibility warnings for C++20+ projects
function(disable_backwards_compatibility_warnings target)
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} INTERFACE 
            /wd4996  # Disable deprecation warnings for older C++ features
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        include(CheckCXXCompilerFlag)
        
        set(COMPAT_WARNINGS_TO_DISABLE
            "c++98-compat"
            "c++98-compat-pedantic"
            "c++98-compat-extra-semi"
            "c++98-compat-local-type-template-args"
            "c++11-compat"
            "c++14-compat"
            "c++17-compat"
            "c++20-compat"
            "c++23-compat"
            "pre-c++14-compat"
            "pre-c++17-compat"
            "pre-c++20-compat"
            "c99-extensions"
            "gnu-zero-variadic-macro-arguments"
            "zero-length-array"
            "gnu-statement-expression"
            "gnu-conditional-omitted-operand"
            "gnu-empty-initializer"
        )
        
        foreach(WARNING ${COMPAT_WARNINGS_TO_DISABLE})
            check_cxx_compiler_flag("-W${WARNING}" HAS_WARNING_${WARNING})
            if(HAS_WARNING_${WARNING})
                target_compile_options(${target} INTERFACE "-Wno-${WARNING}")
            endif()
        endforeach()
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        include(CheckCXXCompilerFlag)
        
        set(GCC_COMPAT_WARNINGS
            "c++11-compat"
            "c++14-compat"
            "c++17-compat"
            "c++20-compat"
        )
        
        foreach(WARNING ${GCC_COMPAT_WARNINGS})
            check_cxx_compiler_flag("-W${WARNING}" HAS_WARNING_${WARNING})
            if(HAS_WARNING_${WARNING})
                target_compile_options(${target} INTERFACE "-Wno-${WARNING}")
            endif()
        endforeach()
    endif()
endfunction()

# Apply backwards compatibility warning suppression to all configurations
disable_backwards_compatibility_warnings(vira)