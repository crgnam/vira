from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeDeps, CMakeToolchain


class ViraConan(ConanFile):
    name = "vira"
    description = "A rendering and ray tracing library for space exploration and mission design purposes."
    
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    
    # Package configuration
    options = {
        "tests": [True, False],
        "shared": [True, False],
        "fPIC": [True, False]
    }
    
    default_options = {
        "tests": False,
        "shared": False,
        "fPIC": True
    }
    
    def layout(self):
        cmake_layout(self)
    
    def configure(self):
        # Remove fPIC option for Windows
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

            # Minimal GDAL Build (Poor support on Windows)
            self.options["gdal"].with_curl = False
            self.options["gdal"].with_geos = False
            self.options["gdal"].with_arrow = False
            self.options["gdal"].with_opencl = False
            self.options["gdal"].ogr_optional_drivers = False  # This might disable elastic
            self.options["gdal"].gdal_optional_drivers = False

        # Ensure C++20 is used
        if self.settings.compiler.cppstd:
            self.settings.compiler.cppstd = "20"
    
    def requirements(self):
        # Core dependencies with version constraints
        self.requires("assimp/5.4.3")
        self.requires("cfitsio/4.4.0")
        self.requires("cspice/0067")
        self.requires("embree3/3.13.5")
        self.requires("fftw/3.3.10")
        self.requires("gdal/3.10.3")
        self.requires("glm/1.0.1")
        self.requires("indicators/2.3")
        self.requires("libtiff/4.6.0")
        self.requires("lz4/1.9.4")
        self.requires("plog/1.1.10")
        self.requires("tclap/1.2.5")
        self.requires("termcolor/2.0.0")
        self.requires("onetbb/[>=2021.0]")  # Keep flexible for embree3 compatibility
        self.requires("yaml-cpp/0.8.0")
        
        # Test dependencies
        if self.options.tests:
            self.requires("gtest/1.16.0")
    
    def build_requirements(self):
        self.tool_requires("cmake/[>=3.15]")
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
    
    def build(self):
        pass
    
    def package(self):
        pass
    
    def package_info(self):
        pass