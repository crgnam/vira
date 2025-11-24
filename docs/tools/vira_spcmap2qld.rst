vira_spcmap2qld
==============

**Convert SPCMaplets to Quipu QLD files**

Synopsis
--------

.. code-block:: bash

   vira_spcmap2qld [OPTIONS] INPUT_GLOB [INPUT_GLOB ...]

Description
-----------

``vira_spcmap2qld`` is a command-line utility for converting SPCMaplet files to Quipu QLD (Quipu Level-of-Detail) format. SPCMaplets are specialized mesh data structures used in planetary science applications, and this tool converts them into the more general-purpose QLD format for visualization and analysis.

The tool supports batch processing of multiple files using glob patterns and can optionally compress the output data and include albedo information.

Arguments
---------

**INPUT_GLOB [INPUT_GLOB ...]**
   One or more glob patterns specifying the SPCMaplet files to convert. Supports standard shell glob patterns like ``*.MAP``, etc.

Options
-------

**-o, --output** *DIRECTORY*
   Directory where the output QLD files will be written (default: current directory ``.``).

**-a, --albedo**
   Write albedo data to the QLD files. This includes surface reflectance information if available in the source SPCMaplet files.

**-c, --compress**
   Enable data compression in the output QLD files to reduce file size.

**-p, --parallel**
   Process files in parallel using multiple CPU cores for faster conversion of large batches.

**--help**
   Display help information and exit.

**--version**
   Display version information and exit.

Features
--------

**Batch Processing**
   Convert multiple SPCMaplet files in a single operation using glob patterns.

**Parallel Processing**
   Optional multi-threaded processing for improved performance on multi-core systems.

**Albedo Support**
   Preserve and convert albedo (surface reflectance) data when available.

**Data Compression**
   Optional compression to reduce output file sizes without quality loss.

**Pyramid Generation**
   Automatically generates hierarchical level-of-detail pyramids for efficient rendering.

**Transformation Preservation**
   Maintains spatial transformation matrices from the original SPCMaplet files.

Output
------

For each input SPCMaplet file, the tool generates a corresponding ``.qld`` file with the same base filename. The QLD files contain:

- Hierarchical mesh data with multiple levels of detail
- Preserved spatial transformation information
- Optional albedo data (if ``--albedo`` flag is used)
- Optional compression (if ``--compress`` flag is used)

File naming: ``<input_basename>.qld``

Examples
--------

Convert all SPCMaplet files in current directory:

.. code-block:: bash

   vira_spcmap2qld *.MAP

Convert with custom output directory:

.. code-block:: bash

   vira_spcmap2qld -o /path/to/output *.MAP

Convert with albedo data and compression:

.. code-block:: bash

   vira_spcmap2qld --albedo --compress data/*.MAP

Parallel processing of large dataset:

.. code-block:: bash

   vira_spcmap2qld --parallel --output results/ bigdata/*.MAP

Convert multiple file patterns:

.. code-block:: bash

   vira_spcmap2qld dataset1/*.MAP dataset2/*.MAP misc_data/special_*.MAP