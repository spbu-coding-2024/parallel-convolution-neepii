#!/usr/bin/python3

import matplotlib.pyplot as plt
import numpy as np
import subprocess
import os
import sys
import typing
import numpy as np
from PIL import Image

# Build program with `make benchmark` before running this script

def collect_data() -> array[float]:
    sample = []
    iterations = 10
    for i in range(iterations):
        result = subprocess.run(cmd, capture_output=True, timeout=20, text=True)

        if result.returncode == 0:
            time_str = result.stdout.strip()
            elapsed = float(time_str)
            sample.append(elapsed)
            # print(f"{img_file:20} {pixels:8} pixels  {elapsed:.6f} sec")
        else:
            print(f"{img_file:20} ERROR: {result.stderr}")
    return sample

filter_names = ["ident", "prewitt", "blur3", "blur5", "ridge", "sharp"]

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

pixel_counts = []
imgs = []
execution_times = [[] for _ in range(len(filter_names))]

print("=" * 50)

for img_file in images:
    img_path = os.path.join(image_dir, img_file)
    try:
        img = Image.open(img_path)
        pixels = img.width * img.height
        pixel_counts.append(pixels)
        imgs.append(img_file)
    except Exception as e:
        # print(f"Warning: Could not read {img_file}: {e}")
        continue

for i in range(len(filter_names)):
    for j in range(len(imgs)):
        img_file = imgs[j]
        output_file = f"/tmp/{img_file}.out"
        cmd = ["./build/conv", "-i", img_path, "-o", output_file, "-f", filter_names[i]]
        sample = collect_data()
        execution_times[i].append(sample)

if pixel_counts and execution_times:

    plt.figure(figsize=(10, 6))
    plt.xlabel('Image Size (pixel count)')
    plt.ylabel('Execution Time (seconds)')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    for j in range(6):
        r = np.round(np.random.rand(),1)
        g = np.round(np.random.rand(),1)
        b = np.round(np.random.rand(),1)
        samples = list(map(lambda x : np.array(x), execution_times[j]))
        mean_results = list(map(lambda x : np.mean(x), samples))
        std_results = list(map(lambda x : np.std(x, ddof=1), samples))
        plt.errorbar(pixel_counts, mean_results, yerr=std_results, marker='o', linestyle='--', color=[r,g,b], linewidth=2)

    output_plot = "output_plot"
    plt.savefig(output_plot)
    # print(f"\nPlot saved to: {output_plot}")
    plt.show()
else:
    print("Error: No valid benchmark results to plot.")
