#!/bin/bash

process_circuit_epfl() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    local aig_file_drw="$4"
    local iter="$5"

    local output_dir="../EPFLres/$relative_path"
    mkdir -p "$output_dir"

    local out="$output_dir/${circuit_name}_abc_$iter.out"    

    ./abc -c "&r $aig_file; &cec $aig_file_drw" > "$out"
}

process_circuit_beem() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    local aig_file_drw="$4"
    local iter="$5"

    local output_dir="../BEEMres/$relative_path"
    mkdir -p "$output_dir"

    local out="$output_dir/${circuit_name}_abc_$iter.out"    

    ./abc -c "&r $aig_file; &cec $aig_file_drw" > "$out"
}

while IFS= read -r -d '' aig_file; do
    base_name=$(basename "$aig_file" .aig)
    dir_path=$(dirname "$aig_file")
    aig_file_drw="${dir_path}/${base_name}_drw.aig"
    
    if [[ -f "$aig_file_drw" ]]; then
        # Extract relative path from EPFLres
        relative_path=$(dirname "${aig_file#../EPFLres/}")

        for iter in $(seq 1 10);
        do         
            process_circuit_epfl "$aig_file" "$base_name" "$relative_path" "$aig_file_drw" "$iter" &
        done
    else
        echo "Warning: No corresponding _drw.aig file found for $aig_file"
        echo "  Expected: $aig_file_drw"
        echo ""
    fi
    
done < <(find ../EPFLres -name "*.aig" -not -name "*_drw.aig" -print0)

while IFS= read -r -d '' aig_file; do
    base_name=$(basename "$aig_file" .aig)
    dir_path=$(dirname "$aig_file")
    aig_file_drw="${dir_path}/${base_name}_drw.aig"
    
    if [[ -f "$aig_file_drw" ]]; then
        # Extract relative path from EPFLres
        relative_path=$(dirname "${aig_file#../BEEMres/}")

        for iter in $(seq 1 10);
        do         
            process_circuit_beem "$aig_file" "$base_name" "$relative_path" "$aig_file_drw" "$iter" &
        done
    else
        echo "Warning: No corresponding _drw.aig file found for $aig_file"
        echo "  Expected: $aig_file_drw"
        echo ""
    fi
    
done < <(find ../BEEMres -name "*.aig" -not -name "*_drw.aig" -print0)
