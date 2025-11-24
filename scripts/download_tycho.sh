#!/bin/bash

# Check that user provided output directory
if [ -z "$1" ]; then
    echo "Error: Output directory is required."
    echo "Usage: $0 <output_directory>"
    echo "Example: $0 /home/user/data_sets/"
    exit 1
fi

BASE_DIR="${1%/}"
OUTPUT_DIR="$BASE_DIR/tycho2"
mkdir -p ${OUTPUT_DIR}

# List of files
files=(
    tyc2.dat.00 tyc2.dat.01 tyc2.dat.02 tyc2.dat.03 tyc2.dat.04
    tyc2.dat.05 tyc2.dat.06 tyc2.dat.07 tyc2.dat.08 tyc2.dat.09
    tyc2.dat.10 tyc2.dat.11 tyc2.dat.12 tyc2.dat.13 tyc2.dat.14
    tyc2.dat.15 tyc2.dat.16 tyc2.dat.17 tyc2.dat.18 tyc2.dat.19
    #suppl_1.dat suppl_2.dat
)

# Base URL
base_url="https://cdsarc.cds.unistra.fr/viz-bin/nph-Cat/txt?I/259"

# Loop through each file
for file in "${files[@]}"; do
    echo "Downloading $file..."
    curl -L "${base_url}/${file}.gz" -o "$OUTPUT_DIR/$file"
done

echo ""
echo "All files downloaded successfully to: $OUTPUT_DIR"