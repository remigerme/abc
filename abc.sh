#!/bin/bash

# Make sure dependencies are met
if ! command -v 7z &> /dev/null; then
  echo "Error: 7z is not installed. You must install it before using this script."
  exit 1
fi

if [[ ! -f "./abc" ]]; then
  echo "Error: this script must be executed within a folder containing 'abc' binary."
  exit 1
fi

# Setting up the benchmark
BENCHMARK_URL="https://fmv.jku.at/aiger/beemaigs.7z"
BENCHMARK_DIR="benchmark"
ARCHIVE_NAME="beemaigs.7z"

if [[ ! -d "$BENCHMARK_DIR" ]]; then
  if [[ ! -f "$ARCHIVE_NAME" ]]; then
    echo "Downloading benchmark..."
    curl -O "$BENCHMARK_URL"
  fi

  echo "Uncompressing benchmark..."
  mkdir -p "$BENCHMARK_DIR"
  7z x "$ARCHIVE_NAME" -o"$BENCHMARK_DIR"
else
  echo "Benchmark folder already exists. Skipping download and extraction."
fi

# Running the benchmark
echo "Running benchmark..."
OUTPUT_FILE="benchmark_results.txt"
for FILE in "$BENCHMARK_DIR"/beem/*.aig; do
  if [[ -f "$FILE" ]]; then
    OUTPUT=$(./abc -c "r $FILE; rw -l; cec; write_aiger $FILE.optimized")
    TIME=$(echo "$OUTPUT" | grep "Time =" | awk '{print $(NF-1)}')
    echo "$FILE : $TIME"
    echo "$FILE : $TIME" >> "$OUTPUT_FILE"
  fi
done

echo "Results have been saved in $OUTPUT_FILE"
