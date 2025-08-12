#!/bin/bash

process_circuit_epfl() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="../EPFLres/$relative_path"
    mkdir -p "$output_dir"

    local circuit="$output_dir/${circuit_name}.aig"    
    local circuit_drw="$output_dir/${circuit_name}_drw.aig"    

    ./abc -c "r $aig_file; w $circuit; drw; w $circuit_drw;"
}

process_circuit_beem() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="../BEEMres/$relative_path"
    mkdir -p "$output_dir"

    local circuit="$output_dir/${circuit_name}.aig"    
    local circuit_drw="$output_dir/${circuit_name}_drw.aig"    

    ./abc -c "r $aig_file; w $circuit; drw; w $circuit_drw;"
}

# EPFL benchmark
# while IFS= read -r -d '' aig_file; do
#     circuit_name=$(basename "$aig_file" .aig)
#     relative_path=$(dirname "${aig_file#../EPFLnomtm/}")

#     process_circuit_epfl "$aig_file" "$circuit_name" "$relative_path" &

# done < <(find ../EPFLnomtm -name "*.aig" -type f -print0)

# BEEM benchmark
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/beem/}")

    process_circuit_beem "$aig_file" "$circuit_name" "$relative_path" &

done < <(find benchmark/beem -name "*.aig" -type f -print0)
