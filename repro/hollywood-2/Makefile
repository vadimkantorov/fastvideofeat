# Parameter settings
FV_GMM_NCOMPONENTS = 256
FASTFV_KNN = 5
FASTFV_FLANN_NTREES = 4
FASTFV_FLANN_NCOMPARISONS = 32
# comment next line to disable flann
#FASTFV_QUANTIZER = --enableflann $(FASTFV_FLANN_NTREES) $(FASTFV_FLANN_NCOMPARISONS)
# comment next line to disable s-t grids
FV_SPATIO_TEMPORAL_GRIDS = --enablespatiotemporalgrids
# comment next line and uncomment the one after to use yael
FV = $(FASTFV)
#FV = ./yaelfv.py

# Tools paths
FASTVIDEOFEAT = ../../bin/fastvideofeat
FASTFV = ../../bin/fastfv
YAELDIR = ../../bin/dependencies/yael/yael
export PYTHONPATH := $(shell pwd)/$(YAELDIR):$(PYTHONPATH)

# repro below
trainClips = $(shell seq 823 | xargs printf "actioncliptrain%05d.avi ")
testClips = $(shell seq 884 | xargs printf "actioncliptest%05d.avi ")
allClips = $(trainClips) $(testClips)

videoDir = data/dataset/Hollywood2/AVIClips
evalDir = data/dataset/Hollywood2/ClipSets
dataset = $(addprefix $(videoDir)/, $(allClips))
features = $(addprefix data/features/, $(addsuffix .txt, $(allClips)))
features_sampled = data/features_sampled.txt
vocabs = $(addprefix data/vocabs/, 10-105.hog.gmm 106-213.hog.gmm 214-309.mbhx.gmm 310-405.mbhy.gmm)
fisher_vectors = $(addprefix data/fisher_vectors/, $(addsuffix .txt, $(allClips)))
kernel = data/kernel.txt
vlfeat = vlfeat-0.9.19/toolbox/plotop/vl_pr.m
classification = data/classification.txt
report = data/report.txt
artefactDirs = data/features data/vocabs data/fisher_vectors logs/features logs/fisher_vectors logs/vocabs 

log = $(subst data, logs, $@).log
vocab = $(basename $(basename $(notdir $(1)))) $(1)

all: $(report)
	cat $(report)

$(artefactDirs) data/dataset:
	mkdir -p $@

$(dataset): $(videoDir)

$(features): $(dataset) $(artefactDirs)
	$(FASTVIDEOFEAT) $(videoDir)/$(basename $(notdir $@)) > $@ 2> $(log)

$(features_sampled): $(features)
	awk '{ if (rand() <= .01) print }' data/features/actioncliptrain* > $@

$(vocabs): $(features_sampled)
	cat $(features_sampled) | ../../src/gmm_train.py --gmm_ncomponents $(FV_GMM_NCOMPONENTS) --vocab $(call vocab, $@) > $(log) 2>&1

$(fisher_vectors): $(features) $(vocabs)
	cat $(subst fisher_vectors,features, $@) | $(FV) --xpos 0 --ypos 1 --tpos 2 --knn $(FASTFV_KNN) $(FASTFV_QUANTIZER) $(FV_SPATIO_TEMPORAL_GRIDS) $(foreach g, $(vocabs), --vocab $(call vocab, $(g))) > $@ 2> $(log)

$(kernel) : $(fisher_vectors)
	./dot_kernel.py $(fisher_vectors) > $@

$(classification) : $(kernel) $(vlfeat)
	cat $(kernel) | ./classify.py $(evalDir) $(basename $(allClips)) > $@

$(report): $(classification)
	./report.py > $@

$(videoDir): data/dataset
	wget ftp://ftp.irisa.fr/local/vistas/actions/Hollywood2-actions.tar.gz
	tar -C data/dataset -xf Hollywood2-actions.tar.gz
	rm Hollywood2-actions.tar.gz

$(vlfeat):
	wget https://github.com/vlfeat/vlfeat/archive/v0.9.19.tar.gz
	tar -xf v0.9.19.tar.gz
	rm v0.9.19.tar.gz

clean:
	rm -rf $(artefactDirs) $(features_sampled) $(kernel) $(classification) $(report)
