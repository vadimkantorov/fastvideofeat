# Information & Contact

An earlier version of this code was used to compute the results of the following paper:
  
>"Efficient feature extraction, encoding and classification for action recognition",  
Vadim Kantorov, Ivan Laptev,  
In Proc. Computer Vision and Pattern Recognition (CVPR), IEEE, 2014  

If you use this code, please cite our work:

> @inproceedings{kantorov2014,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;author = {Kantorov, V. and Laptev, I.},  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;title = {Efficient feature extraction, encoding and classification for action recognition},  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;booktitle = {Proc. Computer Vision and Pattern Recognition (CVPR), IEEE, 2014},  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;year = {2014}  
}

The paper and the poster are available at the [project webpage](http://www.di.ens.fr/willow/research/fastvideofeat) or in this repository, the binaries are published on the repository [releases page](http://github.com/vadimkantorov/cvpr2014/releases), the Hollywood-2 and HMDB-51 repro scripts are in the [repro directory] (http://github.com/vadimkantorov/cvpr2014/tree/master/repro/).

Please submit bugs on [GitHub](http://github.com/vadimkantorov/cvpr2014/issues) directly.

For any other question, please contact Vadim Kantorov at vadim.kantorov@inria.fr or vadim.kantorov@gmail.com.


# Description and usage

We release two tools in this repository. The first tool **fastvideofeat** is a motion feature extractor based on motion vectors from video compression information. The second is a fast Fisher vector computation tool **fastfv** that uses vector SSE2 CPU instructions.

We also release scripts (in the *repro* directory) for reproducing our results on Hollywood-2 dataset and on HMDB-51 dataset.

All code is released under the [MIT license](http://github.com/vadimkantorov/cvpr2014/blob/master/LICENSE).

### fastvideofeat

The tool accepts a video file path as input and writes descriptors to standard output.  
##### Command-line options:

Option | Description
--- | ---
--disableHOG | disables HOG descriptor computation
--disableHOF | disables HOF descriptor computation
--disableMBH | disables MBH descriptor computation
-f 1-10 | restricts descriptor computation to the given frame range

**IMPORTANT** Frame range is specified in terms of PTS (presentation time stamp) which are usually equivalent to frame indices, but not always. Beware. You can inspect PTS values of the frames of the video using ffmpeg's ffprobe (fourth column):

> $ ffprobe -print_format csv -show_packets -select_streams 0 video.mp4

The output format (also reminded on standard error):

   ```#Descriptor format: xnorm ynorm tnorm pts StartPTS EndPTS Xoffset Yoffset PatchWidth PatchHeight hog (dim. 96) hof (dim. 108) mbhx (dim. 96) mbhy (dim. 96)```

  + **xnorm** and **ynorm** are the normalized frame coordinates of the spatio-temporal (s-t) patch  
  + **tnorm** and **pts** are the normalized and unnormalized frame number of the s-t patch center  
  + **StartPTS** and **EndPTS** are the frame numbers of the first and last frames of the s-t patch  
  + **Xoffset** and **Yoffset** are the non-normalized frame coordinates of the s-t patch  
  + **PatchWidth** and **PatchHeight** are the non-normalized width and height of teh s-t patch
  + **descr** is the array of floats of concatenated descriptors. The size of this array depends on the enabled   descriptor types. All values are from zero to one. The first comment line describes the enabled descriptor types, their order in the array, and the dimension of each descriptor in the array.  
     
Every line on standard output corresponds to an extracted descriptor of a patch anc consists of tab-separated floats.  

##### Examples:
  - Compute HOG, HOF, MBH and save the descriptors in descriptors.txt:
    > $ ./fastvideofeat video.avi > descriptors.txt

  - Compute only HOF and MBH from the first 600 frames and save the descriptors in descriptors.txt:
    > $ ./fastvideofeat video.avi --disableHOG -f 1-600 > descriptors.txt

More examples in *examples/compute_mpeg_features.sh*.

##### Video format support
We've tested **fastvideofeat** only videos encoded in H.264 and MPEG-4. Whether motion vectors can be extracted and processed depends completely on FFmpeg's ability to put them into the right structures. Last time I've checked it was not working for VP9, for example. And in general, video reading depends fully on FFmpeg libraries.

### fastfv
The tool accepts descriptors on the standard input and writes Fisher vector (FV) to the standard output. The tool consumes GMM vocabs saved by Yael library. A [sample script](https://github.com/vadimkantorov/cvpr2014/blob/master/src/gmm_train.py) to build GMM vocabs with Yael is provided, as well as its [usage example](https://github.com/vadimkantorov/cvpr2014/blob/master/examples/compute_fisher_vectors.sh).

**IMPORTANT** The computed Fisher vectors are non-normalized, apply signed square rooting / power normalization, L2-normalization, clipping etc before training a classifier.
##### Command-line options:

Option | Description
--- | ---
--xpos 0 | specifies the column with **x** coordinate of the s-t patch in the descriptor array
--ypos 1 | specifies the column with **y** coordinate of the s-t patch in the descriptor array
--tpos 2 | specifies the column with **t** coordinate of the s-t patch in the descriptor array
--knn 5 | FV parts corresponding to these many closest GMM centroids will be updated during processing of every input descriptor
--vocab 10-105 10-105.hog.gmm | specifies descriptor type location and path to GMM vocab. This option is mandatory, and several options of this kind are allowed.
--enableflann 4 32 | use FLANN instead of knn for descriptor attribution, first argument is number of kd-trees, second argument is number of checks performed during attribution
--enablespatiotemporalgrids | enables spatio-temporal grids: 1x1x1, 1x3x1, 1x1x2
--enablesecondorder | enables second-order part of the Fisher vector

##### Examples:
  - Compute Fisher vector:
    > $ zcat sample_features_mpeg4.txt.gz | ../bin/fastfv --xpos 0 --ypos 1 --tpos 2 --enablespatiotemporalgrids --enableflann 4 32 --vocab 10-105 hollywood2_sample_vocabs/10-105.hog.gmm --vocab 106-213 hollywood2_sample_vocabs/106-213.hog.gmm --vocab 214-309 hollywood2_sample_vocabs/214-309.mbhx.gmm --vocab 310-405 hollywood2_sample_vocabs/310-405.mbhy.gmm > fv.txt

  - Build GMM vocab with Yael:
    > $ PYTHONPATH=$(pwd)/../bin/dependencies/yael/yael:$PYTHONPATH cat features*.gz | ../src/gmm_train.py --gmm_ncomponents 256 --vocab 10-105 10-105.hog.gmm

Examples are explained in *examples/compute_fisher_vector.sh*.

##### Performance
We haven't observed enabling second order boosts accuracy, so it's disabled by default. Enabling second order part increases Fisher vector size twice.

Using simple knn descriptor attribution (default) beats FLANN in speed by a factor of two, yet leads to <1% accuracy degradation. It's enabled by default because of its speed. 

Enabling spatio-temporal grids (disabled by default) is important for maximum accuracy (~2% gain).

If you use FLANN, it's the number of checks that defines speed, try reducing it to gain speed.

# Building from source

On both Linux and Windows, the binaries will appear in *bin* after building. By default, code links statically with dependencies below, check Makefiles for details.

Dependencies for **fastvideofeat**:
 - opencv (http://opencv.org)
 - ffmpeg (http://ffmpeg.org)

Dependencies for **fastfv**:
 - opencv (http://opencv.org)
 - yael (http://gforge.inria.fr/projects/yael/), ATLAS with LAPACK (required by yael)

The code is known to work with OpenCV 2.4.9, FFmpeg 2.4, Yael 4.38, ATLAS 3.10.2, LAPACK 3.5.0. 

### Linux
Make sure you have the dependencies installed and visible to g++ (a minimal installation script is in the *bin/dependencies* directory). Build the tools by running **make**.

### Windows
Only **fastvideofeat** builds and works on Windows, **fastfv** doesn't build because yael currently does not support Windows.

To build **fastvideofeat**, set in Makefile the good paths to the dependencies, processor architecture and Visual C++ version, and run from an appropriate Visual Studio Developer Command Prompt (specifically, VS2013 x64 Native Tools Command Prompt worked for us):
 > $ nmake -f Makefile.nmake

# Notes
For practical usage, software needs to be modified to save and read features in some binary format, because the overhead on text file reading/writing is huge.

# License and acknowledgements
All code and scripts are licensed under the [MIT license](http://github.com/vadimkantorov/cvpr2014/blob/master/LICENSE).

We greatly thank [Heng Wang](http://lear.inrialpes.fr/people/wang) and [his work](http://lear.inrialpes.fr/people/wang/improved_trajectories) which was of significant help. 
