# Windows Build Instructions

Contents:
- [Prerequisites](#prerequisites)
- [Building Using conda (RECOMMENDED)](#building-using-conda-recommended)
- [Building Using vcpkg](#building-using-vcpkg)


***
 
## Prerequisites
- **Compiler:** C++20 support required
  - Windows: Visual Studio 2019 16.8+ or Visual Studio 2022
- **Git:** Any recent version
- **CMake:** 3.16 or newer

If you do not have these on your system, you can install them with:

**Option 1: Visual Studio Installer (Recommended)**
   - Download and install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/) (free)
   - During installation, select "Desktop development with C++" workload
   - This includes MSVC compiler, CMake, and Git

**Option 2: Individual installations**
   ```powershell
   # Install via winget (Windows Package Manager):
   winget install Git.Git
   winget install Kitware.CMake
   winget install Microsoft.VisualStudio.2022.Community
   ```

**Option 3: Chocolatey**
   ```powershell
   # Install Chocolatey first, then:
   choco install git cmake visualstudio2022community
   ```

***

## Building Using conda (RECOMMENDED)
Using [conda](https://github.com/conda-forge/miniforge) is our recommended way to build and install *Vira*.  We will assume you already have the required [system prerequisites](#prerequisites).

1. **Setup conda (if you haven't already):**
    ```powershell
    # Download and install Miniforge for Windows
    # Go to: https://github.com/conda-forge/miniforge/releases/latest
    # Download: Miniforge3-Windows-x86_64.exe
    # Run the installer and follow the prompts
    ```
    
    After installation, open "Anaconda Prompt (miniforge3)" from the Start Menu and ensure you can activate the conda `base` environment (You should see a `(base)` to the left of your prompt)

2. **Create the vira_env environment:**
    ```powershell
    conda env create -f dependencies/environment.yml
    conda activate vira_env
    ```

3. **Build:**
    ```powershell
    mkdir build && cd build
    cmake -DCMAKE_TOOLCHAIN_FILE="../cmake/conda-toolchain.cmake" -DCMAKE_BUILD_TYPE=Release ../
    cmake --build . --config Release -j
    ```

4. **OPTIONAL - Installing in conda Environment:**
    Once you have successfully built the project, you can install *Vira* into your conda environment (while still in the `vira_env` environment) using:
    
    ```powershell
    cmake --install . --config Release
    ```

***

## Building Using vcpkg 
[vcpkg](https://github.com/microsoft/vcpkg) is a free open source package manager available on Windows, Linux, and MacOS. This is our primary recommendation when using Visual Studio on Windows. *vcpkg* builds libraries from source and so first time builds of *Vira* can seem excessive if using vcpkg.  We will assume you already have the required [system prerequisites](#prerequisites).

1. **Setup vcpkg:**
   ```powershell
   # Clone vcpkg to C:\vcpkg (or your preferred location)
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

2. **Build:**
   ```powershell
   mkdir build && cd build
   cmake -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release ../
   cmake --build . --config Release -j
   ```

   **Note:** If you installed vcpkg to a different location, adjust the toolchain file path accordingly.