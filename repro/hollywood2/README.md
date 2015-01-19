This is a set of scripts to reproduce the Hollywood-2 results, with time
measurement.

# Prerequisites
 - 64-bit *nix, 800 Gb of free disk space, several gigs of RAM
 - Python, NumPy, octave, scikits-learn, PyYAML, oct2py, octave
 - Hollywood-2 dataset. To download and unpack it automatically, run:
   $ make get_hwd2
 - VLfeat 0.9.19. To download and unpack it automatically, run:
   $ make get_vlfeat

# Instructions

Make sure all prerequisites are in place. Study the Makefile (paths for fastvideofeat/fastfv and libs they need are set in the beginning), and then run the repro with:
> $ make --jobs 8

You can adjust the number of cores used for parallel execution. In about two hours the scripts will fill the data and logs directories. After execution you will see a report like:
> Report on Hollywood-2 classification task
>
>Average frame count: 285
>Average frame size: 609x338
>Average descriptor count: 596177
>
>All fps are reported without taking file reading and writing into account, howevere, video decoding is included.
>
>Features (HOF, HOG, MBH enabled):
>  Average total fps: 336.15
>  Average HOG fps: 621.60
>  Average HOF fps: 668.25
>  Average MBH fps: 518.88
>
>Fisher vectors (components: 256, grids: ['1x1x1x', '1x3x1x', '1x1x2x', '2x2x1x', '2x2x2x', '1x3x2x'], updates/descriptor: 5, >second order enabled: False, FLANN trees: 4, FLANN comparisons: 32):
>  Average total fps: 72.24
>
>Classification:
  AnswerPhone           AP = 0.3263
  DriveCar              AP = 0.8860
  Eat                   AP = 0.6263
  FightPerson           AP = 0.7391
  GetOutCar             AP = 0.5175
  HandShake             AP = 0.3977
  HugPerson             AP = 0.3839
  Kiss                  AP = 0.6493
  Run                   AP = 0.6991
  SitDown               AP = 0.7160
  SitUp                 AP = 0.3646
  StandUp               AP = 0.7259

>  mean: 0.5860
--------------------------------------------------------------

To remove all produced items (no worry, it will not remove the downloaded Hollywood-2 dataset), run:
> $ make clean

# Notes
Key parameters are specified on top of the Makefile. You may try to change some, hopefully everything will still work.
Features and Fisher vectors are not compressed for the sake of script clarity. For practical usage, feel free to modify the script to use gzip compression or modify the tools to output floats in binary format.
