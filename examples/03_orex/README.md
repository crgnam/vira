# Orex Example
This example demonstrates how to load a DSK file and how to use the SPICE interfaces within Vira effectively.

## 1. Download the Data
In the `data/` directory (in the root of the Vira repository), there is a `download_kernels` script (available both as a `.bat` for Windows, and `.sh` for Linux/MacOS).  This will download all of the required SPICE kernels to a new directory located at `data/kernels/`.

## 2. Rendering an Image
If you built Vira with the `-DVIRA_BUILD_EXAMPLES=ON` option, then the `vira_orex` executable will have already been built to your build directory.  Simply run that executable, while providing the path to the `meta_kernel.tm` file provided in this example.