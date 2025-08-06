#!/usr/bin/env python3
"""
Script to plot a 3D graph showing execution time vs number of AND gates vs levels
Reads .time, .and, and .lev files to generate the visualization
"""

import os
import glob
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import argparse


def read_value_from_file(filepath):
    """Read a single numeric value from a file"""
    try:
        with open(filepath, 'r') as f:
            return float(f.read().strip())
    except (FileNotFoundError, ValueError):
        return None


def collect_data(directory, filter_type='all'):
    """Collect data from .time, .and, and .lev files in the given directory
    
    Args:
        directory: Directory to search
        filter_type: 'all', 'dc2', 'non-dc2', '10x', 'non-10x'
    """
    data = []
    
    # Find all .time files recursively
    time_files = glob.glob(os.path.join(directory, '**', '*.time'), recursive=True)
    
    for time_file in time_files:
        # Get the base name without extension
        base_path = time_file[:-5]  # Remove '.time'
        
        # Find all corresponding .and files that match the filter
        and_pattern = base_path + '*.and'
        matching_and_files = glob.glob(and_pattern)
        
        for and_file in matching_and_files:
            # Get base name of the and file (without .and extension)
            and_base = and_file[:-4]
            and_filename = os.path.basename(and_base)
            
            # Apply filter
            if filter_type == 'dc2' and not and_filename.endswith('_dc2'):
                continue
            elif filter_type == 'non-dc2' and and_filename.endswith('_dc2'):
                continue
            elif filter_type == '10x' and '_10x' not in and_filename:
                continue
            elif filter_type == 'non-10x' and '_10x' in and_filename:
                continue
            
            # Corresponding .lev file
            lev_file = and_base + '.lev'
            
            # Read values
            time_val = read_value_from_file(time_file)
            and_val = read_value_from_file(and_file)
            lev_val = read_value_from_file(lev_file)
            
            # Only include if all three values are available
            if all(val is not None for val in [time_val, and_val, lev_val]):
                data.append({
                    'time': time_val,
                    'and_gates': and_val,
                    'levels': lev_val,
                    'circuit': and_filename
                })
    
    return data


def plot_3d_scatter(data, output_file=None):
    """Create a 3D scatter plot"""
    if not data:
        print("No data found to plot")
        return
    
    # Extract data arrays
    and_gates = [d['and_gates'] for d in data]
    levels = [d['levels'] for d in data]
    times = [d['time'] for d in data]
    
    # Create 3D plot
    fig = plt.figure(figsize=(12, 9))
    ax = fig.add_subplot(111, projection='3d')
    
    # Create scatter plot
    scatter = ax.scatter(and_gates, levels, times, c=times, cmap='viridis', 
                        s=60, alpha=0.7, edgecolors='black', linewidth=0.5)
    
    # Set labels and title
    ax.set_xlabel('Number of AND Gates', fontsize=12, labelpad=10)
    ax.set_ylabel('Number of Levels', fontsize=12, labelpad=10)
    ax.set_zlabel('Execution Time (sec)', fontsize=12, labelpad=10)
    ax.set_title('Execution Time vs AND Gates vs Levels', fontsize=14, pad=20)
    
    # Add colorbar
    cbar = plt.colorbar(scatter, ax=ax, shrink=0.8, aspect=20, pad=0.1)
    cbar.set_label('Execution Time (sec)', fontsize=12)
    
    # Improve layout
    ax.view_init(elev=20, azim=45)
    
    # Add grid
    ax.grid(True, alpha=0.3)
    
    # Print statistics
    print(f"Plotted {len(data)} circuits")
    print(f"AND gates range: {min(and_gates):.0f} - {max(and_gates):.0f}")
    print(f"Levels range: {min(levels):.0f} - {max(levels):.0f}")
    print(f"Time range: {min(times):.3f} - {max(times):.3f} sec")
    
    # Save or show plot
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved to {output_file}")
    else:
        plt.show()


def plot_2d_projections(data, output_prefix=None):
    """Create 2D projections of the data"""
    if not data:
        print("No data found to plot")
        return
    
    # Extract data arrays
    and_gates = [d['and_gates'] for d in data]
    levels = [d['levels'] for d in data]
    times = [d['time'] for d in data]
    
    # Create subplots
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    
    # Time vs AND gates
    axes[0].scatter(and_gates, times, alpha=0.7, s=30)
    axes[0].set_xlabel('Number of AND Gates')
    axes[0].set_ylabel('Execution Time (sec)')
    axes[0].set_title('Time vs AND Gates')
    axes[0].grid(True, alpha=0.3)
    
    # Time vs Levels
    axes[1].scatter(levels, times, alpha=0.7, s=30)
    axes[1].set_xlabel('Number of Levels')
    axes[1].set_ylabel('Execution Time (sec)')
    axes[1].set_title('Time vs Levels')
    axes[1].grid(True, alpha=0.3)
    
    # AND gates vs Levels
    scatter = axes[2].scatter(and_gates, levels, c=times, cmap='viridis', alpha=0.7, s=30)
    axes[2].set_xlabel('Number of AND Gates')
    axes[2].set_ylabel('Number of Levels')
    axes[2].set_title('AND Gates vs Levels (colored by time)')
    axes[2].grid(True, alpha=0.3)
    
    # Add colorbar for the third plot
    cbar = plt.colorbar(scatter, ax=axes[2])
    cbar.set_label('Execution Time (sec)')
    
    plt.tight_layout()
    
    # Save or show plot
    if output_prefix:
        output_file = f"{output_prefix}_2d_projections.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"2D projections saved to {output_file}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(description='Plot 3D graph of execution time vs AND gates vs levels')
    parser.add_argument('directory', help='Directory to search for .time, .and, and .lev files')
    parser.add_argument('-o', '--output', help='Output file for the 3D plot (PNG format)')
    parser.add_argument('-2', '--projections', action='store_true', 
                       help='Also create 2D projections')
    parser.add_argument('--output-prefix', help='Prefix for output files when using projections')
    parser.add_argument('-f', '--filter', choices=['all', 'dc2', 'non-dc2', '10x', 'non-10x'], 
                       default='all', help='Filter circuits by type (default: all)')
    
    args = parser.parse_args()
    
    if not os.path.isdir(args.directory):
        print(f"Error: Directory '{args.directory}' not found")
        return
    
    print(f"Collecting data from {args.directory}...")
    print(f"Using filter: {args.filter}")
    data = collect_data(args.directory, args.filter)
    
    if not data:
        print("No complete datasets found (need matching .time, .and, and .lev files)")
        return
    
    # Create 3D plot
    plot_3d_scatter(data, args.output)
    
    # Create 2D projections if requested
    if args.projections:
        plot_2d_projections(data, args.output_prefix)


if __name__ == "__main__":
    main()
