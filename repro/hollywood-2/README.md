# Reproducing Hollywood-2 results

## Prerequisites
 - 64-bit *nix, 800 Gb of free disk space, several gigs of RAM
 - Python, NumPy, octave, scikits-learn, PyYAML, oct2py, octave
 - Hollywood-2 dataset. To download and unpack it automatically, run:
 
   > $ make get_hwd2
 - VLfeat 0.9.19. To download and unpack it automatically, run:
 
   > $ make get_vlfeat

## Instructions

Make sure all prerequisites are in place. Adjust the paths to the tools in Makefile, and then run the repro with:
> $ make --jobs 8

You can adjust the number of cores used for parallel execution. In about an hour the scripts will fill the data and logs directories. After execution you will see a report like:
````
Report on Hollywood-2 classification task

Average frame count: 285
Average frame size: 609x338
Average descriptor count: 596177

All fps are reported without taking file reading and writing into account, howevere, video decoding is included.

Features (HOF, HOG, MBH enabled):
  Average total fps: 336.15
  Average HOG fps: 621.60
  Average HOF fps: 668.25
  Average MBH fps: 518.88

Fisher vectors (components: 256, grids: ['1x1x1', '1x3x1', '1x1x2'], updates/descriptor: 5, second order enabled: False, FLANN trees: 4, FLANN comparisons: 32):
  Average total fps: 101.73

Classification:
  AnswerPhone           AP = 0.3368
  DriveCar              AP = 0.8976
  Eat                   AP = 0.6187
  FightPerson           AP = 0.7357
  GetOutCar             AP = 0.5383
  HandShake             AP = 0.3676
  HugPerson             AP = 0.3956
  Kiss                  AP = 0.6409
  Run                   AP = 0.7144
  SitDown               AP = 0.7548
  SitUp                 AP = 0.2775
  StandUp               AP = 0.7344

  mean: 0.5843
```

To remove all produced items (no worry, it will not remove the downloaded Hollywood-2 dataset), run:
> $ make clean

# Notes
Key parameters are specified on top of the Makefile. You may try to change some, hopefully everything will still work.
Features and Fisher vectors are not compressed for the sake of script clarity. For practical usage, feel free to modify the script to use gzip compression or modify the tools to output floats in binary format.
