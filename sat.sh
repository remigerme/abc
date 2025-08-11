#!/bin/bash

process_circuit_beem() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/beem_processed/$relative_path"
    mkdir -p "$output_dir"

    local circuit="$output_dir/${circuit_name}.aig"
    local circuit_out="$output_dir/${circuit_name}.out"

    ./abc -c "&r $aig_file; &dc2; &cec -v; &ps;" > "$circuit_out"
}

process_circuit_aig_cpu() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/aig_cpu_processed/$relative_path"
    mkdir -p "$output_dir"

    local circuit="$output_dir/${circuit_name}.aig"
    local circuit_out="$output_dir/${circuit_name}.out"

    ./abc -c "&r $aig_file; &dc2; &cec -v; &ps;" > "$circuit_out"
}

process_circuit_epfl() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/epfl_processed/$relative_path"
    mkdir -p "$output_dir"
    
    local circuit_5x="$output_dir/${circuit_name}_5x.aig"
    local circuit_out="$output_dir/${circuit_name}.out"
    local circuit_5x_out="$output_dir/${circuit_name}_5x.out"

    # ./abc -c "r $aig_file; logic; double; double; double; double; double; strash; w $circuit_5x"

    ./abc -c "&r $aig_file; &dc2; &cec -v; ps;" > "$circuit_out"
    # ./abc -c "&r $circuit_5x; &dc2; &cec -v; ps;" > "$circuit_5x_out"
}

# EPFL benchmark
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/EPFLfull/}")

    process_circuit_epfl "$aig_file" "$circuit_name" "$relative_path" &

done < <(find benchmark/EPFLfull -name "*.aig" -type f -print0)

# Beems
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/beem/}")    
    process_circuit_beem "$aig_file" "$circuit_name" "$relative_path" &
done < <(find benchmark/beem -name "*.aig" -type f -print0)

# CPU
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/aig_cpu/}")    
    process_circuit_aig_cpu "$aig_file" "$circuit_name" "$relative_path" &
done < <(find benchmark/aig_cpu -name "*.aig" -type f -print0)


# wait
