#!/bin/bash

# Script to extract level count from PS files
# and save the result in .lev files --- written by AI

# Default variables
DIRECTORY="${1:-.}"

# Check if directory exists
if [[ ! -d "$DIRECTORY" ]]; then
    exit 1
fi

# Function to process a PS file
process_ps_file() {
    local ps_file="$1"
    local lev_file="${ps_file%.ps}.lev"
    
    # Read the file and extract level count
    if [[ -r "$ps_file" ]]; then
        # Look for line containing "lev =" and extract the level value
        local levels=$(grep -o 'lev = *[0-9]\+' "$ps_file" | grep -o '[0-9]\+' | head -n 1)
        
        # Check if we found a valid number
        if [[ "$levels" =~ ^[0-9]+$ ]]; then
            echo "$levels" > "$lev_file"
        fi
    fi
}

# Find and process .ps files (recursive search)
while IFS= read -r -d '' ps_file; do
    process_ps_file "$ps_file"
done < <(find "$DIRECTORY" -name "*.ps" -type f -print0)
