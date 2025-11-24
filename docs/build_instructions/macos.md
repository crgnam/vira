# MacOS build Instructions

Contents:
- [Prerequisites](#prerequisites)
- [Building Using conda (RECOMMENDED)](#building-using-conda-recommended)
- [Building Using vcpkg](#building-using-vcpkg)
- [Building Using conan](#building-using-conan)
- [Building Using System Libraries (NOT RECOMMENDED)](#building-using-system-libraries-not-recommended)


***
 
## Prerequisites
- **Compiler:** C++20 support required
  - macOS: Xcode 12+ or Clang 10+
- **Git:** Any recent version
- **CMake:** 3.16 or newer
- **pkg-config:** Library configuration tool

If you do not have these on your system, you can install them with:
   ```bash
   # Install Xcode Command Line Tools (includes git, clang):
   xcode-select --install
   
   # Install CMake and pkg-config via Homebrew:
   brew install cmake pkg-config
   ```

**Note For conda Users:** If you use the *conda* environment approach described below (recommended) you can install many of these prerequisites using conda instead of your system package manager (which often requires `sudo` access).  While it is preferred to use the system compilers, if you do not have access, you can install them into the `vira_env` environment (see [Building Using conda](#building-using-conda-recommended)) using:

```bash
conda install -c conda-forge cmake git pkg-config cxx-compiler make
```

***

## Building Using conda (RECOMMENDED)
Using [conda](https://github.com/conda-forge/miniforge) is our recommended way to build and install *Vira*.  We will assume you already have the required [system prerequisites](#prerequisites).

1. **Setup conda (if you haven't already):**
    ```bash
    cd ~
    # For Apple Silicon Macs:
    curl -L -O "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-arm64.sh"
    chmod +x Miniforge3-MacOSX-arm64.sh
    ./Miniforge3-MacOSX-arm64.sh
    
    # For Intel Macs:
    curl -L -O "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-x86_64.sh"
    chmod +x Miniforge3-MacOSX-x86_64.sh
    ./Miniforge3-MacOSX-x86_64.sh
    ```
    Follow the prompts and ensure you can activate the conda `base` environment (You should see a `(base)` to the left of your username in the terminal)

2. **Create the vira_env environment:**
    ```bash
    conda env create -f dependencies/environment.yml
    conda activate vira_env
    ```

3. **Build:**
    ```bash
    mkdir build && cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/conda-toolchain.cmake -DCMAKE_BUILD_TYPE=Release ../
    cmake --build . -j
    ```

4. **OPTIONAL - Installing in conda Environment:**
    Once you have successfully built the project, you can install *Vira* into your conda environment (while still in the `vira_env` environment) using:
    
    ```bash
    cmake --install .
    ```

***

## Building Using vcpkg 
[vcpkg](https://github.com/microsoft/vcpkg) is a free open source package manager available on Windows, Linux, and MacOS.  *vcpkg* builds libraries from source and so first time builds of *Vira* can seem excessive if using vcpkg.  We will assume you already have the required [system prerequisites](#prerequisites).

1. **Setup vcpkg:**
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh
   ~/vcpkg/vcpkg integrate install
   ```

2. **Build:**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release ../
   cmake --build . -j
   ```

***

## Building Using conan
[conan](https://conan.io/) is a free and open source package manager available on Windows, Linux, and MacOS.  **We do NOT regularly test builds with this** but provide some basic support because we recognize some teams use *conan* for package management.  We will assume you already have the required [system prerequisites](#prerequisites), and that you already have *conan* setup.

1. **Install dependencies:**
    ```bash
    conan install dependencies/conanfile.py --build=missing --output-folder=.
    cd build
    ```

2. **Build:**
    ```bash
    cmake -DCMAKE_TOOLCHAIN_FILE=./Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release ../
    cmake --build . -j
    ```

***

## Building Using System Libraries (NOT RECOMMENDED)
In the event that you wish to use your system package manager (such as `brew`) or build the dependencies from source, that should in theory work, however it is not supported (we cannot guarantee to use versions of libraries commonly available on system package managers).  You will need to install the following dependencies on your system manually:

| Library | Version | Description |
|---------|:-------:|-------------|
| assimp | >=5.2,<6.0 | 3D model loading |
| cfitsio | >=3.49 | FITS I/O |
| cspice | =67 | NASA SPICE |
| embree3 | >=3.13,<4.0 | Ray tracing kernels |
| fftw | >=3.3.10,<4.0 | Fast Fourier Transform |
| gdal | >=3.10,<4.0 | Geospatial Abstraction |
| glm | >=1.0.1 | Linear Algebra |
| libtiff | >=4.7.0 | TIFF I/O |
| lz4 | >=1.9 | Compression |
| tbb | >=2021.0 | Intel TBB |
| yaml-cpp | >=0.8.0,<1.0 | YAML parser |
| gtest | >=1.15,<2.0 | Google Test (Only required if enabling `VIRA_BUILD_TESTS`) |

Assuming you have installed these dependencies, the build process will be straight forward.  From within the cloned `vira` repository:

```bash
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```