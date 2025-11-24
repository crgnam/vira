vira_dem2qld
============

**DEM Data Pre-processing Script**

Synopsis
--------

.. code-block:: bash

   vira_dem2qld [OPTIONS] CONFIG_FILE

Description
-----------

``vira_dem2qld`` is a command-line utility for converting Digital Elevation Model (DEM) data into Quipu QLD format files. The tool processes DEM files through YAML configuration, supporting features like albedo mapping, chunking for large datasets, and parallel processing.

The utility can handle large DEM files by automatically breaking them into manageable chunks based on available memory, and supports appending adjacent DEM files for seamless terrain processing.

Arguments
---------

**CONFIG_FILE**
   Path to the YAML configuration file that defines the DEM processing parameters and input files.

Options
-------

**-r, --root-path** *PATH*
   Root file path. If not provided, the location of the config YAML file is used as the root path.

**-m, --max-memory** *GB*
   Maximum allowed memory usage in gigabytes (default: 25). Must be greater than 2 GB.
   
**--help**
   Display help information and exit.

**--version**
   Display version information and exit.


YAML Configuration Format
--------------------------

The configuration file must be in YAML format such as the following exmaple which specifies wrapping the LDEM128 global map of the moon with various WAC_EMP albedo maps.

.. code-block:: yaml

    output: qpu/ldem128/

    max-vertices: 1000000
    
    default-albedo: 0.12
    
    inputs:
      - file: LDEM_128.tif
        albedo-files:
          - albedo/WAC_EMP_643NM_P900N0000_304P.IMG
          - albedo/WAC_EMP_643NM_P900S0000_304P.IMG
        albedo-files-lonwrap:
          - albedo/WAC_EMP_643NM_E300*.IMG
        append-right: LDEM_128.tif


Output
------

The tool generates one or more ``.qld`` (Quipu Level-of-Detail) files containing:

- Hierarchical mesh pyramids for efficient rendering
- Embedded albedo data (if provided)
- Geospatial transformation information
- Compressed data (if enabled)

File naming convention: ``<input_filename>[_chunk-N]_tile-N.qld``

Examples
--------

Basic DEM conversion:

.. code-block:: bash

   vira_dem2qld config.yaml

Convert with custom memory limit:

.. code-block:: bash

   vira_dem2qld --max-memory 16 config.yaml

Process with custom root path:

.. code-block:: bash

   vira_dem2qld --root-path /data/dems config.yaml