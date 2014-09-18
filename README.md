Information & Contact
=====================

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

The paper and the poster are available at the [project webpage](http://www.di.ens.fr/willow/research/fastvideofeat) or in this repository.

For any question or bug report, please contact Vadim Kantorov at vadim.kantorov@inria.fr or vadim.kantorov@gmail.com


Description and usage
=====================

We release two tools in this repository. The first tool **fastvideofeat** is a motion feature extractor based on motion vectors from video compression information. The second is a fast Fisher vector computation tool **fastfv** that uses vector SSE2 CPU instructions.

### fastvideofeat

The tool accepts a video file path as input and writes descriptors to standard output.  
##### Command-line options:

Option | Default | Description
--- | --- | ---
-i video.avi | | specifies the path to the input video
--hog yes/no | **yes** | enables/disables HOG descriptor computation
--hof yes/no | **yes** | enables/disables HOF descriptor computation
--mbh yes/no | **yes** | enables/disables MBH descriptor computation
-f 1-10 | whole video | restricts descriptor computation to the given frame range

**IMPORTANT** Frame range is specified in terms of PTS (presentation time stamp) which are usually equivalent to frame indices, but not always. Beware. You can inspect PTS values of the frames of the video using ffmpeg's ffprobe (fourth column):

> $ ffprobe -print_format csv -show_packets -select_streams 0 video.mp4

The output format:
   The first two lines of the standard output are comments explaining the format):
>  #descr = hog(96) hof(108) mbh(96 + 96)  
   #x y pts StartPTS EndPTS Xoffset Yoffset PatchWidth PatchHeight descr


  + **x** and **y** are the normalized frame coordinates of the spatio-temporal (s-t) patch  
  + **pts** is the frame number of the s-t patch center  
  + **StartPTS** and **EndPTS** are the frame numbers of the first and last frames of the s-t patch  
  + **Xoffset** and **Yoffset** are the non-normalized frame coordinates of the s-t patch  
  + **PatchWidth** and **PatchHeight** are the non-normalized width and height of teh s-t patch
  + **descr** is the array of floats of concatenated descriptors. The size of this array depends on the enabled   descriptor types. All values are from zero to one. The first comment line describes the enabled descriptor types, their order in the array, and the dimension of each descriptor in the array.  
     
After the comments every line corresponds to an extracted descriptor of a patch. All numbers in the output are floating point in text format and are separated by tabs.  
The standard error contains various debug / diagnostic messages like time measurements and parameters in effect.

##### Examples:
  - Compute HOG, HOF, MBH and save the descriptors in descriptors.txt:
    > $ ./fastvideofeat -i video.avi > descriptors.txt

  - Compute only HOF and MBH from the first 600 frames and save the descriptors in descriptors.txt:
    > $ ./fastvideofeat -i video.avi -hog no -hof yes -mbh yes -f 1-600 > descriptors.txt

More examples in samples/compute_mpeg_features.sh.

### fastfv
The tool accepts descriptors on the standard input and writes Fisher vector (FV) to the standard output. The code for saving to an HDF5 file is commented out, try to hack it if you need it.

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
--gmm_k 256 | 256 | specifies the number of GMM components used for FV computation
--knn 5 | 5 | FV parts corresponding to these many closest GMM centroids will be updated during processing of every input descriptor
--vocab 9-104 hog_K256.vocab | | specifies descriptor type location and path to GMM vocabs. This option is mandatory, and several options of this kind are allowed.
--grid 1x3x2x | | specifies the layout of the s-t grid (**x** cells times **y** cells times **t** cells). This option is mandatory, and several options of this kind are allowed.
--buildGmmIndex | | this option will have the GMM vocabs computed and saved to the specified path. No Fisher vector will be computed


##### Examples:
  - Build GMM vocabulary:
    > $ cat descriptors.txt | $EXE_FV --buildGmmIndex --vocab 105-212 hof_K256.vocab

  - Compute Fisher vector:
    > $ zcat sample_features_mpeg4.txt.gz | $EXE_FV --vocab 9-104 hollywood2_sample_vocabs/hog_K256.vocab --vocab 105-212 hollywood2_sample_vocabs/hof_K256.vocab --vocab 213-308 hollywood2_sample_vocabs/mbhx_K256.vocab --vocab 309-404 hollywood2_sample_vocabs/mbhy_K256.vocab --xnpos 0 --ynpos 1 --tnpos 2 --grid 1x1x1x --grid 2x2x1x --grid 1x3x1x --grid 1x1x2x --grid 2x2x2x --grid 1x3x2x --ttot 192 > fv.txt

Examples are explained in samples/compute_fisher_vector.sh. 

Building from source
====================

### Linux
Make sure you have the dependencies installed and visible to the CC compiler (normally gcc). The code is known to work with OpenCV 2.4.9, FFmpeg 2.4, Yael 4.01.  If the dependencies are installed to a custom path, you may want to adjust CPATH and LIBRARY_PATH environment variables. Then navigate to the correspoding directory in **src** and type:
> $ make

The binaries will be placed in the **build** sub-directory.

Dependencies for **fastvideofeat**:
 - opencv (http://opencv.org)
 - ffmpeg (http://ffmpeg.org)

Dependencies for **fastfv**:
 - opencv (http://opencv.org)
 - yael (http://gforge.inria.fr/projects/yael/) [needed for computing the GMM vocab]

A minimal script to install dependencies on Linux is found in the 3rdparty directory.

### Windows
Only **fastvideofeat** should work on Windows. **fastfv** cannot work because yael does not support Windows, though I guess it can be hacked to do so.

To build **fastvideofeat**, you have to define %OPENCV_DIR%, %FFMPEG_DIR% environment variables.  You will also need to have a modern Visual Studio (or Visual C++ Express ). Then navigate to the corresponding directory in **src** and open VS.vcxproj.

The binaries will be placed in the **build** sub-directory.
