# Put your binaries in bin
FASTFV=../bin/fastfv

# Compute GMM vocabs for HOG / HOF / MBH. Lines below commented because we don't provide a feature archive.
# YAELDIR=$(pwd)/../bin/dependencies/yael/yael
# export PYTHONPATH=$YAELDIR:$PYTHONPATH
# cat features*.gz | ../src/gmm_train.py --gmm_ncomponents 256 --vocab 10-105 10-105.hog.gmm
# cat features*.gz | ../src/gmm_train.py --gmm_ncomponents 256 --vocab 106-213 106-213.hog.gmm
# cat features*.gz | ../src/gmm_train.py --gmm_ncomponents 256 --vocab 214-309 214-309.mbhx.gmm
# cat features*.gz | ../src/gmm_train.py --gmm_ncomponents 256 --vocab 310-405 310-405.mbhy.gmm

# compute unnormalized Fisher vectors for MPEG features using precomputed vocabs
# The descriptor part indices are zero-based and inclusive:
#  - HOG is columns [10; 105]
#  - HOF is columns [106; 212]
#  - MBHx is columns [214; 309]
#  - MBHy is columns [310; 405]
# xnpos, ynpos, tnpos specify the columns of the spatio-temporal location of the descriptor (0, 1, 2 in our case). For MPEG features spatio-temporal coordinates are 1-normalized, so no xtot/ytot/ttot options are needed.
# x/y/t-gridding is controlled by the grid options.

zcat sample_features_mpeg4.txt.gz | $FASTFV --xpos 0 --ypos 1 --tpos 2 --enablespatiotemporalgrids --enableflann 4 32 --vocab 10-105 hollywood2_sample_vocabs/10-105.hog.gmm --vocab 106-213 hollywood2_sample_vocabs/106-213.hog.gmm --vocab 214-309 hollywood2_sample_vocabs/214-309.mbhx.gmm --vocab 310-405 hollywood2_sample_vocabs/310-405.mbhy.gmm > fv.txt
