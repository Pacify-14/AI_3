#!/bin/bash

# --- Script Configuration ---
# The generic file name used for the SAT workflow commands
GENERIC_BASE="sample"
GENERIC_CITY="${GENERIC_BASE}.city"
GENERIC_SATINPUT="${GENERIC_BASE}.satinput"
GENERIC_SATOUTPUT="${GENERIC_BASE}.satoutput"

# --- Main Testing Loop ---
echo "--- Starting SAT Workflow for all sample_*.city files ---"
echo "Searching for sample_*.city files..."

# Loop through all files matching the pattern sample_*.city
for original_city_file in sample_*.city; do
    
    # Check if any files were found
    if [ "$original_city_file" == "sample_*.city" ]; then
        echo "No sample_*.city files found. Exiting."
        exit 1
    fi

    echo ""
    echo "========================================================="
    echo "Processing Test Case: ${original_city_file}"
    echo "========================================================="

    # 1. Copy the specific test case to the generic 'sample.city'
    cp "$original_city_file" "$GENERIC_CITY"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy ${original_city_file} to ${GENERIC_CITY}. Skipping."
        continue
    fi
    echo "Copied ${original_city_file} to ${GENERIC_CITY}"

    # 2. Run the SAT encoding script (./run1.sh sample)
    echo "2. Encoding problem to SAT: ./run1.sh ${GENERIC_BASE}"
    ./run1.sh "$GENERIC_BASE"
    
    if [ $? -ne 0 ]; then
        echo "Error: ./run1.sh failed. Skipping to cleanup."
        continue
    fi
    if [ ! -f "$GENERIC_SATINPUT" ]; then
        echo "Error: ${GENERIC_SATINPUT} not created. Skipping to cleanup."
        continue
    fi

    # 3. Run the SAT solver (minisat sample.satinput sample.satoutput)
    echo "3. Solving SAT problem: minisat ${GENERIC_SATINPUT} -> ${GENERIC_SATOUTPUT}"
    minisat "$GENERIC_SATINPUT" "$GENERIC_SATOUTPUT"
    
    # Note: minisat usually returns 10 for SAT, 20 for UNSAT, 0 for error.
    if [ ! -f "$GENERIC_SATOUTPUT" ]; then
        echo "Error: ${GENERIC_SATOUTPUT} not created by minisat. Skipping to cleanup."
        continue
    fi

    # 4. Run the path decoding script (./run2.sh sample)
    echo "4. Decoding SAT result: ./run2.sh ${GENERIC_BASE}"
    ./run2.sh "$GENERIC_BASE"

    # 5. Run the format checker (python3 format_checker.py sample)
    echo "5. Checking output format with format_checker.py:"
    python3 format_checker.py "$GENERIC_BASE"
    
    # --- Cleanup Temporary Files ---
    echo "6. Cleaning up temporary files for next iteration..."
    rm -f "$GENERIC_CITY" "$GENERIC_SATINPUT" "$GENERIC_SATOUTPUT" "${GENERIC_BASE}.metromap"
    
done

echo ""
echo "--- All sample tests completed. ---"
