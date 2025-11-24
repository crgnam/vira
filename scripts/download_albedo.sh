#!/bin/bash

# Check that user provided output directory
if [ -z "$1" ]; then
    echo "Error: Output directory is required."
    echo "Usage: $0 <output_directory>"
    echo "Example: $0 /home/user/data_sets/"
    exit 1
fi

BASE_DIR="${1%/}"
OUTPUT_DIR="$BASE_DIR/albedo"
mkdir -p "$OUTPUT_DIR"

# List of files
files=(
    WAC_EMP_643NM_P900N0000_304P.IMG
    WAC_EMP_643NM_P900S0000_304P.IMG
    WAC_EMP_643NM_E300N2250_304P.IMG
    WAC_EMP_643NM_E300N3150_304P.IMG
    WAC_EMP_643NM_E300N0450_304P.IMG
    WAC_EMP_643NM_E300N1350_304P.IMG
    WAC_EMP_643NM_E300S2250_304P.IMG
    WAC_EMP_643NM_E300S3150_304P.IMG
    WAC_EMP_643NM_E300S0450_304P.IMG
    WAC_EMP_643NM_E300S1350_304P.IMG
)

# Base URL
base_url="https://pds.lroc.im-ldi.com/data/LRO-L-LROC-5-RDR-V1.0/LROLRC_2001/DATA/MDR/WAC_EMP"

# Loop through each file
for file in "${files[@]}"; do
    echo "Downloading $file..."
    curl -L "${base_url}/${file}" -o "$OUTPUT_DIR/$file"
done

echo ""
echo "All files downloaded successfully to: $OUTPUT_DIR"