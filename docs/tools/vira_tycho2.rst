vira_tycho2
===========

**Convert Tycho2 .dat files to QSC file**

Synopsis
--------

.. code-block:: bash

   vira_tycho2 [OPTIONS] INPUT_GLOB [INPUT_GLOB ...]

Description
-----------

``vira_tycho2`` is a command-line utility for converting Tycho-2 star catalog data files (``.dat`` format) into a single Quipu Star Catalog (QSC) file. The Tycho-2 catalog is one of the most comprehensive and accurate star catalogs available, containing astrometric and photometric data for over 2.5 million stars.

This tool processes multiple Tycho-2 data files (typically named ``tyc2.dat.XX`` where XX is a sequence number) and combines them into a unified, optimized QSC format suitable for astronomical visualization and simulation applications.

Arguments
---------

**INPUT_GLOB [INPUT_GLOB ...]**
   One or more glob patterns specifying the Tycho-2 ``.dat`` files to process. Common patterns include ``tyc2.dat.*``, ``*.dat``, or explicit file lists.

Options
-------

**-o, --output** *DIRECTORY*
   Directory where the output ``tycho2.qsc`` file will be written (default: current directory ``.``).

**--help**
   Display help information and exit.

**--version**
   Display version information and exit.


Input Data Format
-----------------

The tool expects Tycho-2 catalog files in the standard ``.dat`` format as distributed by astronomical data centers. These files typically contain:

- Star positions (Right Ascension, Declination)
- Proper motions
- Photometric data (magnitudes in multiple bands)
- Astrometric quality indicators
- Cross-reference identifiers

Common file naming patterns:
- ``tyc2.dat.00``, ``tyc2.dat.01``, ..., ``tyc2.dat.19`` (official distribution)
- ``tycho2_*.dat`` (alternative naming)

Output
------

The tool generates a single ``tycho2.qsc`` file containing:

- Complete merged star catalog data
- Stars sorted by magnitude (brightest first)
- Optimized binary format for fast access
- Preserved astrometric and photometric precision

The output file is named ``tycho2.qsc`` and will be placed in the specified output directory.

Examples
--------

Convert all Tycho-2 files in current directory:

.. code-block:: bash

   vira_tycho2 tyc2.dat.*


References
----------

- Tycho-2 Catalogue: https://cdsarc.cds.unistra.fr/viz-bin/cat/I/259