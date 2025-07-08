#!/bin/bash

if [ ! -d "benchmark/EPFLfull" ]; then
    log_error "Download EPFL full benchmark."
    exit 1
fi

mkdir -p benchmark/epfl_processed

process_circuit() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/epfl_processed/$relative_path"
    mkdir -p "$output_dir"
    
    local circuit_10x="$output_dir/${circuit_name}_10x.aig"
    local circuit_10x_dc2="$output_dir/${circuit_name}_10x_dc2.aig"
    local circuit_log="$output_dir/${circuit_name}.log"
    
    # Creating 10x circuit
    ./abc -c "r $aig_file; logic; double; double; double; double; double; double; double; double; double; double; strash; write_aiger $circuit_10x;"

    # Optimizing 10x circuit
    ./abc -c "&r $circuit_10x; &dc2; &w $circuit_10x_dc2;"

    # Equivalence checking
    ./abc -c "&r $circuit_10x; &cec $circuit_10x_dc2;" > "$circuit_log"
}

while IFS= read -r -d '' aig_file; do
    # Do not handle MtM files
    if [[ "$aig_file" == *"/MtM/"* ]]; then
        continue
    fi
    
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/EPFLfull/}")
    process_circuit "$aig_file" "$circuit_name" "$relative_path"
    
done < <(find benchmark/EPFLfull -name "*.aig" -type f -print0)
