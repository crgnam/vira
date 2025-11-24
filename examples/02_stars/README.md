# Stars Example
This example demonstrates how to render an image containing unresolved objects, primarily stars using the Tycho2 catalogue.  This includes fetching the raw catalogue data, pre-processing it into the required Vira format, and then writing an executable to use the Vira API to render an image.

## 1. Download the Data
In the `data/` directory (in the root of the Vira repository), there is a `download_tycho` script (available both as a `.bat` for Windows, and `.sh` for Linux/MacOS).  This will download all of the required `.dat` files for the [Tycho2 catalogue](https://cdsarc.cds.unistra.fr/viz-bin/cat/I/259) to a new directory located at `data/tycho2/`.

Also in the `data/` directory is a `download_kernels` script (again both a `.bat` and `.sh`).  This will download all required SPICE kernels to a new directory located at `data/kernels/`.

## 2. Build the Vira command line tools
We'll need the `vira_tycho2` command line tool to pre-process the `.dat` files we just downloaded.  To do this, run the CMake configuration with the `-DVIRA_BUILD_TOOLS=ON` option.  So from the root directory, you can run:

```
mkdir build;
cd build;
cmake ../ -DCMAKE_TOOLCHAIN_FILE=<you/path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DVIRA_BUILD_TOOLS=ON -DVIRA_BUILD_EXAMPLES=ON
cmake --build . -j
```

Optionally, to install the tools on your system, you can install after building by running the following command:

```
cmake --install .
```

*NOTE: The `-DVIRA_BUILD_EXAMPLES=ON` option will build the example programs, including the `vira_stars` executable for this example.  The `-j` option in `cmake --build . -j` configures the build process to use all available cores in parallel.*

## 2. Pre-process the Tycho2 .dat files
The `vira_tycho2` executable takes in all of the `.dat` files, processes them, and saves the data to a special format specific to Vira known as the "quipu star catalogue" (`.qsc`).  Quipu files are binary files used by Vira to quickly access data required for rendering.

To perform this pre-processing we simply provide `vira_tycho2` a GLOB of the `.dat` files, and the directory where we want to save the resulting `tycho2.qsc` file.  For the purposes of this example, that means running:

```
vira_tycho2 data/tycho2/*.dat.* data/tycho2/
```

## 3. Rendering an Image
If you built Vira with the `-DVIRA_BUILD_EXAMPLES=ON` option, then the `vira_stars` executable will have already been built to your build directory.  Simply run that executable, while providing the path to the generated `tycho2.qsc` file.