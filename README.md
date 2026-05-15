# Convolution

## How to start

You need the OpenMP library in order to compile.
Then, just run `make`
``` bash
make
```

The binary is in the `build` directory

## Graph plots
Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz

=======
RAM: 16 GB

OS: GNU/Linux 6.12.67_1

CC: gcc 14.2.1

CFLAGS: see Makefile for info

### Sequential
<p align="center">
<img src="plot_imgs/output_sequential_plot.png" width="600" alt="description">
</p>

### Parallel (OpenMP)

#### Rows
<p align="center">
<img src="plot_imgs/output_plot_rows.png" width="600" alt="description">
</p>

#### Columns
<p align="center">
<img src="plot_imgs/output_plot_columns.png" width="600" alt="description">
</p>

#### Pixel by pixel
<p align="center">
<img src="plot_imgs/output_plot_pixel_by_pixel.png" width="600" alt="description">
</p>

#### Blocks
<p align="center">
<img src="plot_imgs/output_plot_block.png" width="600" alt="description">
</p>

### Parallel (Pipeline)
Image used are in test/images directory
<p align="center">
<img src="plot_imgs/pipeline_plot.png" width="600" alt="description">
</p>
