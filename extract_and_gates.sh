#!/bin/bash

# Script to extract AND gate count from AIG files
# and save the result in .and files --- written by AI

# Default variables
DIRECTORY="${1:-.}"

# Check if directory exists
if [[ ! -d "$DIRECTORY" ]]; then
    exit 1
fi

# Function to process an AIG file
process_aig_file() {
    local aig_file="$1"
    local and_file="${aig_file%.aig}.and"
    
    # Read first line and extract 6th field (AND gate count)
    if [[ -r "$aig_file" ]]; then
        local first_line=$(head -n 1 "$aig_file")
        
        # Check if it's a valid AIG file (starts with "aig")
        if [[ $first_line =~ ^aig[[:space:]] ]]; then
            # Extract 6th field (AND gate count)
            local and_gates=$(echo "$first_line" | awk '{print $6}')
            
            # Check if it's a number
            if [[ "$and_gates" =~ ^[0-9]+$ ]]; then
                echo "$and_gates" > "$and_file"
            fi
        fi
    fi
}

# Find and process .aig files (recursive search)
while IFS= read -r -d '' aig_file; do
    process_aig_file "$aig_file"
done < <(find "$DIRECTORY" -name "*.aig" -type f -print0)
