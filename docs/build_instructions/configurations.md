# Configurations

## Build Options
Building with these optional functionalities may require additional pre-requisites.  Many of these additional dependencies will be automatically handled by the package management system (*conda*, *vcpkg*, or *conan*).

Option | Description |  Default | Additional Requirements |
------ | ------| ------ | ------ |
`VIRA_BUILD_TOOLS` | Build command-line tools and utilities | ON | 
`VIRA_BUILD_EXAMPLES` | Build examples | OFF |
`VIRA_BUILD_TESTS` | Build tests using gtest | OFF | 
`VIRA_BUILD_VIRAPY` | Build python bindings | OFF | [See Python Bindings](#building-python-bindings)
`VIRA_BUILD_DOCS` | Build documentation | OFF | [See Building Documentation](#building-documentation)
`VIRA_BUILD_SCRATCH` | Build your included scratch/development programs | ON |
`VIRA_KEEP_VCPKG_BUILDTREES` | Keep the *vcpkg* buildtrees (for debugging *vcpkg* issues) | OFF |
`VIRA_ALL_SELECTED_WARNINGS` | Turn on all selected warnings | OFF |

**By command line, you enable/disable options like this:**
```bash
cmake -DVIRA_BUILD_EXAMPLES=ON -DVIRA_BUILD_TESTS=OFF ..
```

**In Visual Studio:**
- Open the project folder in Visual Studio
- Go to Project -> CMake Settings
- Add variables in the "CMake variables and cache" section


## Common Configurations:

**Release build for distribution:**
```bash
cmake -DVIRA_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release ..
```

**Development build with tests:**
```bash
cmake -DVIRA_BUILD_TESTS=ON -DVIRA_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Debug ..
```

## Building Python Bindings:

To build the *ViraPy* wrapper library you can choose any of the build setups shown for your platform.  As always, we recommend typical users to use [conda](https://github.com/conda-forge/miniforge).

In general the process is:

1. **Install Python3 with Development headers:**
   - Windows: Download from [python.org](https://python.org) (do NOT install from Microsoft Store!)
   - Linux: `sudo apt install python3-dev` 
   - macOS: `brew install python`

   **Note:** If using the *conda* build method, Python dev headers are already included in the `vira_env` environment.

2. **Configure CMake:**
    ```bash
    cmake -DVIRA_BUILD_VIRAPY=ON ../

    # On Linux/MacOS:
    cmake --build . -j

    # On Windows with MSBuild:
    cmake --build . -j --config Release
    ```

3. **Install after building:**
    From within the `build/` directory, simply run:
    ```bash
    pip install .
    ```

### Common Issues:
1. **Python not found:** Depending on the environment you're using, CMake may not be able to find your python executable.  If this is the case, you can manually specify it during cmake configuration:
    ```bash
    -DPython_EXECUTABLE=/path/to/python ..
    ```

2. **Module fails to load:** This often happens when you built *ViraPy* with one version of python, but then tried to use it with another.  It's possible that CMake found a different python executable than you intended to use, in which case manually specifying (using the above) can help resolve this issue.

## Building Documentation:

To build documentation, you will need to install several other dependencies.  We will be assuming you have already setup and activated the `vira_env` environment required to build *Vira* using *conda* managed dependencies.

1. **Install Additional Requirements:**
    ```bash
    conda env update -f docs/docs-requirements.yml
    ```

2. **Build:**
    ```bash
    # From the build/ directory
    cmake -DVIRA_BUILD_DOCS=ON ../
    
    # On Linux/MacOS:
    cmake --build . -j

    # On Windows with MSBuild:
    cmake --build . -j --config Release
    ```

The generated documentation will be available in `build/vira_docs/index.html`

**NOTE:** To build Python API docs, add `-DVIRA_BUILD_VIRAPY=ON` to the CMake command.  (Make sure you can already build the python bindings!)

### Building with Visual Studio
It is possible to build the documentation using Visual Studio, but we need to get Visual Studio to use the conda environment we previously installed the dependencies to.  The easiest way to do this
is to simply activate the conda environment in powershell, and then launch Visual Studio from the command line by running `devenv.exe`.  If this executable is not on your PATH, then you many have to run it using the full path by calling something like:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"
```

**NOTE:** Your specific path may differ based on your Visual Studio version/installation.

### Installing Dependencies Manually:
To build without using a *conda* environment, you will need to install the required dependencies manually.  We do NOT recommend this, but here are instructions to use at your own risk:

1. **Required tools:**
    - **Doxygen 1.9.2+** (for C++20 support) - [Download here](https://www.doxygen.nl/download.html)
    - **Graphviz** - For diagram generation

2. **Python packages:**
    ```bash
    pip install breathe sphinx sphinx-rtd-theme sphinx-autodoc-typehints sphinx-toolbox myst-parser
    ```

Once these are all installed, you can build as normal:

```bash
cmake -DVIRA_BUILD_DOCS=ON ../
cmake --build . -j
```