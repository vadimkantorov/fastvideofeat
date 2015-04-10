# Put your binaries in bin
FASTVIDEOFEAT=../bin/fastvideofeat

# compute HOG/HOF/MBH features from MPEG4-encoded video. features_mpeg4.txt should approx. match uncompressed sample_features_mpeg4.txt.gz modulo minor ffmpeg decoding changes.
$FASTVIDEOFEAT hollywood2_actioncliptrain00001_mpeg4.avi > features_mpeg4.txt

# compute features from first 100 frames from MPEG4-encoded video
$FASTVIDEOFEAT hollywood2_actioncliptrain00001_mpeg4.avi -f 1-100 > features_mpeg4_600frames.txt

# compute only HOF / MBH (no HOG) features from 
$FASTVIDEOFEAT hollywood2_actioncliptrain00001_h264.mp4 --disableHOG | gzip > features_h264_nohog.txt.gz
