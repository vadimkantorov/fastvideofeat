# Parameter settings
FV_GMM_NCOMPONENTS = 256
FASTFV_KNN = 5

# uncomment next three lines to enable flann
#FASTFV_FLANN_NTREES = 4
#FASTFV_FLANN_NCOMPARISONS = 32
#FASTFV_QUANTIZER = --enableflann $(FASTFV_FLANN_NTREES) $(FASTFV_FLANN_NCOMPARISONS)

# comment next line to disable s-t grids
FV_SPATIO_TEMPORAL_GRIDS = --enablespatiotemporalgrids

# comment next line and uncomment the one after to use yael
FV = $(FASTFV)
#FV = ../yaelfv.py

# Tools paths
FASTVIDEOFEAT = ../../bin/fastvideofeat
FASTFV = ../../bin/fastfv
YAELDIR = ../../bin/dependencies/yael/yael
export PYTHONPATH := $(shell pwd)/$(YAELDIR):$(PYTHONPATH)

# repro below
clipFile = allClips.txt
allClips = $(shell cat $(clipFile))
 
videoDir = data/dataset/avi
evalDir = data/dataset/testTrainMulti_7030_splits
recodedDir = data/dataset/avi_recoded

dataset = $(addprefix $(videoDir)/, $(allClips))
features = $(addprefix data/features/, $(addsuffix .txt, $(allClips)))
features_sampled = data/features_sampled.txt
vocabs = $(addprefix data/vocabs/, 10-105.hog.gmm 106-213.hog.gmm 214-309.mbhx.gmm 310-405.mbhy.gmm)
fisher_vectors = $(addprefix data/fisher_vectors/, $(addsuffix .txt, $(allClips)))
kernel = data/kernel.txt
classification = data/classification.txt
report = data/report.txt
artefactDirs = data/features data/vocabs data/fisher_vectors data/dataset/avi_recoded logs/features logs/fisher_vectors logs/vocabs logs/recoding

log = $(subst data, logs, $@).log
vocab = $(basename $(basename $(notdir $(1)))) $(1)

all: $(report)
	echo "Report on HMDB-51 classification task."
	echo "Reported accuracies are accuracies on dataset splits."
	echo ""
	cat $(report)

$(artefactDirs) data/dataset:
	mkdir -p $@

$(dataset): $(videoDir)

$(features): $(dataset) | $(artefactDirs)
	ffmpeg -y -an -i $(videoDir)/$(basename $(notdir $@)) -vcodec libxvid -q:v 1 $(recodedDir)/$(basename $(notdir $@)) > $(subst features,recoding, $(log)) 2>&1
	$(FASTVIDEOFEAT) $(recodedDir)/$(basename $(notdir $@)) > $@ 2> $(log)

$(features_sampled): $(features)
	awk '{ if (rand() <= .01) print }' data/features/* > $@

$(vocabs): $(features_sampled)
	cat $(features_sampled) | ../../src/gmm_train.py --gmm_ncomponents $(FV_GMM_NCOMPONENTS) --vocab $(call vocab, $@) > $(log) 2>&1

$(fisher_vectors): $(features) $(vocabs)
	cat $(subst fisher_vectors,features, $@) | $(FV) --xpos 0 --ypos 1 --tpos 2 --knn $(FASTFV_KNN) $(FASTFV_QUANTIZER) $(FV_SPATIO_TEMPORAL_GRIDS) $(foreach g, $(vocabs), --vocab $(call vocab, $(g))) > $@ 2> $(log)

$(kernel) : $(fisher_vectors)
	../dot_kernel.py data/fisher_vectors $(clipFile) > $@

$(classification) : $(kernel)
	cat $(kernel) | ./classify.py $(evalDir) $(clipFile) > $@

$(report): $(classification)
	../report.py > $@

$(videoDir): | data/dataset
	wget http://www.rarlab.com/rar/rarlinux-3.9.1.tar.gz
	wget http://serre-lab.clps.brown.edu/wp-content/uploads/2013/10/hmdb51_org.rar
	wget http://serre-lab.clps.brown.edu/wp-content/uploads/2013/10/test_train_splits.rar
	mkdir -p $(videoDir)
	tar -C data/dataset -xf rarlinux-3.9.1.tar.gz
	data/dataset/rar/unrar x test_train_splits.rar data/dataset
	data/dataset/rar/unrar x hmdb51_org.rar $(videoDir)
	data/dataset/rar/unrar x '$(videoDir)/*.rar' $(videoDir)
	find $(videoDir) -name '*.avi' -exec bash -c 'mv "{}" $(videoDir)/$$(echo $$(basename "{}") | sed "s/[][;()&?!]/_/g")' \; 2> /dev/null
	rm rarlinux-3.9.1.tar.gz hmdb51_org.rar test_train_splits.rar $(videoDir)/*.rar

clean:
	rm -rf $(artefactDirs) $(features_sampled) $(kernel) $(classification) $(report)
