# Copy your own binaries in this folder if you built them from sources. Don't forget to have dependencies locatable. If you installed the dep's in a custom path, you may want to uncomment the next line:
# LD_LIBRARY_PATH=$(YOUR_CUSTOM_PREFIX)/lib:$LD_LIBRARY_PATH
# The script will fall back to pre-built ones if no binaries found in this directory.

EXE_FV=fastfv
if [ ! -f $EXE_FV ];
then
	EXE_FV=../bin/ubuntu-x64/fastfv
	LD_LIBRARY_PATH=../bin/ubuntu-x64:$LD_LIBRARY_PATH
fi

# Compute GMM vocabs for HOG / HOF / MBH. Lines below commented because we don't provide a feature archive (features*.gz).
# zcat features*.gz | $EXE_FV --buildGmmIndex --vocab 9-104 hog_K256.vocab
# zcat features*.gz | $EXE_FV --buildGmmIndex --vocab 105-212 hof_K256.vocab
# zcat features*.gz | $EXE_FV --buildGmmIndex --vocab 213-308 mbhx_K256.vocab
# zcat features*.gz | $EXE_FV --buildGmmIndex --vocab 309-404 mbhy_K256.vocab

# compute unnormalized Fisher vectors
# The precomputed vocabs for MPEG features (HOG / HOF / MBHx / MBHy), built from Hollywood-2 dataset.
# The descriptor part indices are zero-based and inclusive:
#  - HOG is columns [9; 104]
#  - HOF is columns [105; 212]
#  - MBHx is columns [213; 308]
#  - MBHy is columns [309; 404]
# xnpos, ynpos, tnpos specify the columns of the spatio-temporal location of the descriptor. For MPEG features these are 0, 1, 2. For MPEG features spatial coordinates are 1-normalized, so no xtot/ytot options are needed. However, the temporal location is given by the PTS of the frame (usually equivalent to the frame index), so here we specify the ttot option, and set it equal to the frame number of the video.
# x/y/t-gridding is controlled by the grid options.
zcat sample_features_mpeg4.txt.gz | $EXE_FV --vocab 9-104 hollywood2_sample_vocabs/hog_K256.vocab --vocab 105-212 hollywood2_sample_vocabs/hof_K256.vocab --vocab 213-308 hollywood2_sample_vocabs/mbhx_K256.vocab --vocab 309-404 hollywood2_sample_vocabs/mbhy_K256.vocab --xnpos 0 --ynpos 1 --tnpos 2 --grid 1x1x1x --grid 2x2x1x --grid 1x3x1x --grid 1x1x2x --grid 2x2x2x --grid 1x3x2x --ttot 192 > fv.txt
