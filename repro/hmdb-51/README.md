# Reproducing HMDB-51 results

## Prerequisites
 - 64-bit *nix, 800 Gb of free disk space, several gigs of RAM
 - Python, NumPy, scikits-learn, PyYAML, ffmpeg with libxvid

## Instructions

Make sure all prerequisites are in place. Put the tools binaries in *bin* (you can get them from the [releases page](http://github.com/vadimkantorov/cvpr2014/releases)), and then run the repro with:
> $ make --jobs 8

The script will automatically download the HMDB-51 dataset and rar decompressor that are required for evaluation.
You can adjust the number of cores used for parallel execution. In about an hour the scripts will fill the data and logs directories. After execution you will see a report like:
```
Report on HMDB-51 classification task.
Reported accuracies are accuracies on dataset splits.

Average frame count: 95
Average frame size: 366x240
Average descriptor count: 62462

All fps are reported without taking file reading and writing into account, howevere, video decoding is included.

Features (HOF, HOG, MBH enabled):
  Average total fps: 775.76
  Average HOG fps: 3807.06
  Average HOF fps: 4364.18
  Average MBH fps: 2840.89

Fisher vectors (components: 256, s-t grids enabled: True, knn: 5, second order enabled: False, FLANN trees: -1, FLANN comparisons: -1):
  Average total fps: 632.27

Classification:
  split_0           0.4588
  split_1           0.4359
  split_2           0.4595

  mean: 0.4514
```

To remove all produced items (no worry, it will not remove the downloaded Hollywood-2 dataset), run:
> $ make clean

# Notes
The code recodes HMDB-51 videos with libxvid to produce reasonable motion vectors, as original videos in HMDB-51 are quite messy.
Key parameters are specified on top of the Makefile and explained in the [Performance section](https://github.com/vadimkantorov/cvpr2014/#performance). You could play with them, hopefully everything will still work.
Features and Fisher vectors are not compressed for the sake of script clarity. For practical usage, feel free to modify the script to use gzip compression or modify the tools to output floats in binary format.
