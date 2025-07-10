#!/bin/bash

if [ ! -d "benchmark/EPFLfull" ]; then
    log_error "Download EPFL full benchmark."
    exit 1
fi

mkdir -p benchmark/epfl_processed

process_circuit_10x() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/epfl_processed/$relative_path"
    mkdir -p "$output_dir"
    
    local circuit_10x="$output_dir/${circuit_name}_10x.aig"
    local circuit_10x_dc2="$output_dir/${circuit_name}_10x_dc2.aig"
    local circuit_log="$output_dir/${circuit_name}.log"
    local circuit_10x_ps="$output_dir/${circuit_name}_10x.ps"
    local circuit_10x_dc2_ps="$output_dir/${circuit_name}_10x_dc2.ps"
    
    # Creating 10x circuit
    ./abc -c "r $aig_file; logic; double; double; double; double; double; double; double; double; double; double; strash; write_aiger $circuit_10x;"

    # Optimizing 10x circuit
    ./abc -c "&r $circuit_10x; &dc2; &w $circuit_10x_dc2;"

    # Dumping stats
    ./abc -c "&r $circuit_10x; &ps;" > "$circuit_10x_ps"
    ./abc -c "&r $circuit_10x_dc2; &ps;" > "$circuit_10x_dc2_ps"

    # Equivalence checking
    ./abc -c "&r $circuit_10x; &cec $circuit_10x_dc2;" > "$circuit_log"
}

process_circuit_mtm() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/epfl_processed/$relative_path"
    mkdir -p "$output_dir"
    
    local circuit_dc2="$output_dir/${circuit_name}_dc2.aig"
    local circuit_log="$output_dir/${circuit_name}.log"
    local circuit_ps="$output_dir/${circuit_name}.ps"
    local circuit_dc2_ps="$output_dir/${circuit_name}_dc2.ps"

    # Optimizing circuit
    ./abc -c "&r $aig_file; &dc2; &w $circuit_dc2;"

    # Dumping stats
    ./abc -c "&r $aig_file; &ps;" > "$circuit_ps"
    ./abc -c "&r $circuit_dc2; &ps;" > "$circuit_dc2_ps"

    # Equivalence checking
    ./abc -c "&r $aig_file; &cec $circuit_dc2;" > "$circuit_log"
}

process_circuit_cpu() {
    local aig_file="$1"
    local circuit_name="$2"
    local relative_path="$3"
    
    local output_dir="benchmark/cpu/$relative_path"
    mkdir -p "$output_dir"
    
    local circuit_dc2="$output_dir/${circuit_name}_dc2.aig"
    local circuit_log="$output_dir/${circuit_name}.log"
    local circuit_ps="$output_dir/${circuit_name}.ps"
    local circuit_dc2_ps="$output_dir/${circuit_name}_dc2.ps"

    # Optimizing circuit
    ./abc -c "&r $aig_file; &dc2; &w $circuit_dc2;"

    # Dumping stats
    ./abc -c "&r $aig_file; &ps;" > "$circuit_ps"
    ./abc -c "&r $circuit_dc2; &ps;" > "$circuit_dc2_ps"

    # Equivalence checking
    ./abc -c "&r $aig_file; &cec $circuit_dc2;" > "$circuit_log"
}

# EPFL benchmark
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/EPFLfull/}")

    if [[ "$aig_file" == *"/MtM/"* ]]; then
        process_circuit_mtm "$aig_file" "$circuit_name" "$relative_path" &
    else
        process_circuit_10x "$aig_file" "$circuit_name" "$relative_path" &
    fi
done < <(find benchmark/EPFLfull -name "*.aig" -type f -print0)

# CPUs
while IFS= read -r -d '' aig_file; do
    circuit_name=$(basename "$aig_file" .aig)
    relative_path=$(dirname "${aig_file#benchmark/cpu/}")    
    process_circuit_cpu "$aig_file" "$circuit_name" "$relative_path" &
done < <(find benchmark/cpu -name "*.aig" -type f -print0)

wait
