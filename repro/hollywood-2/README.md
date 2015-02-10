# Reproducing Hollywood-2 results

## Prerequisites
 - 64-bit *nix, 800 Gb of free disk space, several gigs of RAM
 - Python, NumPy, scikits-learn, PyYAML, oct2py, octave

## Instructions

Make sure all prerequisites are in place. Put the tools binaries in *bin* (you can get them from the [releases page](http://github.com/vadimkantorov/cvpr2014/releases)), and then run the repro with:
> $ make --jobs 8

The script will automatically download the Hollywood-2 dataset and VLfeat 0.9.19 that are required for evaluation.
You can adjust the number of cores used for parallel execution. In about an hour the scripts will fill the data and logs directories. After execution you will see a report like:
````
Report on Hollywood-2 classification task

Average frame count: 285
Average frame size: 609x338
Average descriptor count: 596177

All fps are reported without taking file reading and writing into account, howevere, video decoding is included.

Features (HOF, HOG, MBH enabled):
  Average total fps: 333.96
  Average HOG fps: 602.25
  Average HOF fps: 635.79
  Average MBH fps: 519.70

Fisher vectors (components: 256, s-t grids enabled: True, knn: 5, second order enabled: False, FLANN trees: -1, FLANN comparisons: -1):
  Average total fps: 227.36

Classification:
  AnswerPhone    	AP = 0.3210
  DriveCar       	AP = 0.8927
  Eat            	AP = 0.6064
  FightPerson    	AP = 0.7101
  GetOutCar      	AP = 0.5536
  HandShake      	AP = 0.3737
  HugPerson      	AP = 0.4174
  Kiss           	AP = 0.6345
  Run            	AP = 0.7090
  SitDown        	AP = 0.7708
  SitUp          	AP = 0.2522
  StandUp        	AP = 0.7445

  mean: 0.5822
```

To remove all produced items (no worry, it will not remove the downloaded Hollywood-2 dataset), run:
> $ make clean

# Notes
Key parameters are specified on top of the Makefile and explained in the [Performance section](https://github.com/vadimkantorov/cvpr2014/#performance). You could play with them, hopefully everything will still work.
Features and Fisher vectors are not compressed for the sake of script clarity. For practical usage, feel free to modify the script to use gzip compression or modify the tools to output floats in binary format.
