# Copy your own binaries in this folder if you built them from sources. Don't forget to have dependencies locatable. If you installed the dep's in a custom path, you may want to uncomment the next line:
# LD_LIBRARY_PATH=$(YOUR_CUSTOM_PREFIX)/lib:$LD_LIBRARY_PATH
# The script will fall back to pre-built ones if no binaries found in this directory.

EXE_FEAT=fastvideofeat
if [ ! -f $EXE_FEAT ];
then
	EXE_FEAT=../bin/ubuntu-x64/fastvideofeat
	LD_LIBRARY_PATH=../bin/ubuntu-x64:$LD_LIBRARY_PATH
fi

# compute HOG/HOF/MBH features from MPEG4-encoded video. features_mpeg4.txt should approx. match uncompressed sample_features_mpeg4.txt.gz modulo minor ffmpeg decoding changes.
$EXE_FEAT -i hollywood2_actioncliptrain00001_mpeg4.avi > features_mpeg4.txt

# compute features from first 600 frames from MPEG4-encoded video
$EXE_FEAT -i hollywood2_actioncliptrain00001_mpeg4.avi -f 1-600 > features_mpeg4_600frames.txt

# compute only HOF / MBH (no HOG) features from 
$EXE_FEAT -i hollywood2_actioncliptrain00001_h264.mp4 -hog no | gzip > features_h264_nohog.txt.gz
