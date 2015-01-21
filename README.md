# Information & Contact

This code was used to compute the results of the following paper:
  
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

The paper and the poster are available at the [project webpage](http://www.di.ens.fr/willow/research/fastvideofeat) or in this repository, the binaries are published on the repository [releases page](http://github.com/vadimkantorov/cvpr2014/releases), the Hollywood-2 repro scripts are in the [repro directory] (http://github.com/vadimkantorov/cvpr2014/tree/master/repro/).

Please submit bugs on [GitHub](http://github.com/vadimkantorov/cvpr2014/issues) directly.

For any other question, please contact Vadim Kantorov at vadim.kantorov@inria.fr or vadim.kantorov@gmail.com.


# Description and usage

We release two tools in this repository. The first tool **fastvideofeat** is a motion feature extractor based on motion vectors from video compression information. The second is a fast Fisher vector computation tool **fastfv** that uses vector SSE2 CPU instructions.

We also release scripts (in the *repro* directory) for reproducing our results on Hollywood-2 dataset.

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

   ```#Descriptor format: xnorm ynorm tnorm pts StartPTS EndPTS Xoffset Yoffset PatchWidth PatchHeight hog (dim. 96) hof (dim. 108) mbhx (dim. 96) mbhy(dim. 96)```

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

More examples in samples/compute_mpeg_features.sh.

### fastfv
The tool accepts descriptors on the standard input and writes Fisher vector (FV) to the standard output. The tool consumes GMM vocabs saved by Yael library. A [sample script](https://github.com/vadimkantorov/cvpr2014/blob/master/src/gmm_train.py) to build GMM vocabs with Yael is provided, as well as its [usage example](https://github.com/vadimkantorov/cvpr2014/blob/master/samples/compute_fisher_vectors.sh).

**IMPORTANT** The computed Fisher vectors are non-normalized. Please apply signed square rooting / power normalization, L2-normalization, clipping etc before training a classifier.
##### Command-line options:

Option | Default | Description
--- | --- | ---
--xnpos 0 | | specifies the column with **x** coordinate of the s-t patch in the descriptor array
--xtot 1.0 | 1.0 | specifies the frame width. If the **x** coordinate is non-normalized, this option is mandatory
--ynpos 1 | | specifies the column with **y** coordinate of the s-t patch in the descriptor array
--ytot 1.0 | 1.0 | specifies the frame height. If the **y** coordinate is non-normalized, this option is mandatory
--tnpos 2 | | specifies the column with **t** coordinate of the s-t patch in the descriptor array
--ttot 192 | 1.0 | specifies the number of frames in the video (actually the last PTS of the video, usually the two are equivalent, but not always). If the **t** coordinate is non-normalized, this option is mandatory
--gmm_ncomponents 256 | 256 | specifies the number of GMM components used for FV computation
--updatesperdescriptor 5 | 5 | FV parts corresponding to these many closest GMM centroids will be updated during processing of every input descriptor
--enablesecondorder | | Enables second-order part of the Fisher vector
--vocab 9-104 hog_K256.gmm | | specifies descriptor type location and path to GMM vocab. This option is mandatory, and several options of this kind are allowed.
--grid 1x3x2x | | specifies the layout of the s-t grid (**x** cells times **y** cells times **t** cells). This option is mandatory, and several options of this kind are allowed.


##### Examples:
  - Compute Fisher vector:

    > $ zcat sample_features_mpeg4.txt.gz | ./fastfv --vocab 9-104 hollywood2_sample_vocabs/hog_K256.gmm --vocab 105-212 hollywood2_sample_vocabs/hof_K256.gmm --vocab 213-308 hollywood2_sample_vocabs/mbhx_K256.gmm --vocab 309-404 hollywood2_sample_vocabs/mbhy_K256.gmm --xnpos 0 --ynpos 1 --tnpos 2 --grid 1x1x1x --grid 2x2x1x --grid 1x3x1x --grid 1x1x2x --grid 2x2x2x --grid 1x3x2x --ttot 192 > fv.txt

Examples are explained in samples/compute_fisher_vector.sh. 

# Building from source

On both Linux and Windows, the binaries will appear in **bin** after building. By default, code links statically with dependencies below, check Makefiles for details.

### Linux
Make sure you have the dependencies installed and visible to g++. You can build the tools by running *make*.

Dependencies for **fastvideofeat**:
 - opencv (http://opencv.org)
 - ffmpeg (http://ffmpeg.org)

Dependencies for **fastfv**:
 - opencv (http://opencv.org)
 - yael (http://gforge.inria.fr/projects/yael/) [needed for reading the GMM vocab from a file]
 - openblas (http://openblas.net) [needed by yael]

The code is known to work with OpenCV 2.4.9, FFmpeg 2.4, Yael 4.01, OpenBLAS 2.0.13. A minimal script to download and install these libraries is in the *bin* directory.

### Windows
Only **fastvideofeat** builds and works on Windows, **fastfv** doesn't build because yael currently does not support Windows.

To build **fastvideofeat**, set in Makefile the good paths to the dependencies, processor architecture and Visual C++ version, and run from a Visual Studio Developer Command Prompt:
 > $ nmake -f Makefile.nmake

# Notes

For practical usage, software needs to be modified to save and read features in some binary format, because the overhead on that part is huge.
