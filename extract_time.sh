#!/bin/bash

# Script to extract time from LOG files
# and save the result in .time files --- written by AI

# Default variables
DIRECTORY="${1:-.}"

# Check if directory exists
if [[ ! -d "$DIRECTORY" ]]; then
    exit 1
fi

# Function to process a LOG file
process_log_file() {
    local log_file="$1"
    local time_file="${log_file%.log}.time"
    
    # Read the file and extract time value
    if [[ -r "$log_file" ]]; then
        # Look for line containing "Time =" and extract the time value
        local time_value=$(grep -o 'Time = *[0-9]*\.*[0-9]\+ sec' "$log_file" | grep -o '[0-9]*\.*[0-9]\+' | head -n 1)
        
        # Check if we found a valid number
        if [[ "$time_value" =~ ^[0-9]*\.?[0-9]+$ ]]; then
            echo "$time_value" > "$time_file"
        fi
    fi
}

# Find and process .log files (recursive search)
while IFS= read -r -d '' log_file; do
    process_log_file "$log_file"
done < <(find "$DIRECTORY" -name "*.log" -type f -print0)
