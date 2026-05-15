#!/usr/bin/python3

import matplotlib.pyplot as plt
import numpy as np
import subprocess
import os
import sys
import typing
from PIL import Image

#
# This script assumes you built the program using make benchmark
#
# I like to use these commands before running this script:
#
#  sudo cpupower frequency-set -g performance
#  sudo cpupower frequency-set -d 800Mhz
#  sudo cpupower frequency-set -u 4000Mhz
#  echo "1" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
#

sample_len = 10

def collect_data(cmd) -> list:
    sample = [0.0] * sample_len
    for i in range(sample_len):
        result = subprocess.run(cmd, capture_output=True, timeout=40, text=True)

        try:
            if result.returncode == 0:
                time_str = result.stdout.strip()
                elapsed = float(time_str)
                sample[i] = elapsed
            else:
                print(f"ERROR: {result.stderr}")
        except Exception as e:
            print(f"Warning: {e}")
            print("Are you sure you built project with `make benchmark`?")
            exit(1)
    return sample

filter_names = ["ident", "blur3", "blur5", "ridge"]
colors = ["red", "orange", "blue", "green"]
assert len(colors) == len(filter_names)

if not os.path.exists("./build/conv"):
    print("Error: ./build/conv not found. Run 'make benchmark' first.")
    sys.exit(1)

image_dir = "test/images"
if not os.path.exists(image_dir):
    print(f"Error: {image_dir} directory not found.")
    sys.exit(1)

images = sorted([f for f in os.listdir(image_dir) if f.endswith('.bmp')])
if not images:
    print(f"Error: No .bmp images found in {image_dir}")
    sys.exit(1)

imgs = []
for img_file in images:
    img_path = os.path.join(image_dir, img_file)
    try:
        img = Image.open(img_path)
        pixels = img.width * img.height
        imgs.append((pixels, img_file, img_path))
    except Exception as e:
        continue

temp_imgs = sorted(imgs, key=(lambda x: (x[0], x[1], x[2])))
img_paths = [x[2] for x in temp_imgs]

# Collect total execution time for all images per filter
execution_times = {filter_name: [] for filter_name in filter_names}

for filter_name in filter_names:
    total_times = []
    for i in range(sample_len):
        # Run with pipeline (default behavior) - pass all images at once
        cmd = ["./build/conv", "-f", filter_name] + img_paths
        result = subprocess.run(cmd, capture_output=True, timeout=40, text=True)
        
        try:
            if result.returncode == 0:
                time_str = result.stdout.strip()
                elapsed = float(time_str)
                total_times.append(elapsed)
            else:
                print(f"ERROR: {result.stderr}")
                sys.exit(1)
        except Exception as e:
            print(f"Warning: {e}")
            print("Are you sure you built project with `make benchmark`?")
            sys.exit(1)
    
    execution_times[filter_name] = total_times

if execution_times:
    plt.figure(figsize=(10, 6))
    plt.xlabel('Filter')
    plt.ylabel('Total Execution Time (seconds)')
    plt.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()

    means = []
    stds = []
    for filter_name in filter_names:
        samples = np.array(execution_times[filter_name])
        mean = np.mean(samples)
        std = 3 * np.std(samples, ddof=1) / np.sqrt(sample_len)
        means.append(mean)
        stds.append(std)

    x_pos = np.arange(len(filter_names))
    plt.bar(x_pos, means, yerr=stds, capsize=5, color=colors, alpha=0.7, edgecolor='black')
    plt.xticks(x_pos, filter_names)

    output_plot = "pipeline_plot"
    plt.savefig(output_plot)
    print(f"\nPlot saved to: {output_plot}")
    plt.show()
else:
    print("Error: No valid benchmark results to plot.")
