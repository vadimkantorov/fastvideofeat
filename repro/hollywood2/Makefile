#### Tools paths

FASTVIDEOFEAT = $(error Update Makefile with correct tools paths)
FASTFV = $(error Update Makefile with correct tools paths)
LIBS = $(error Update Makefile with correct libs path, or empty path if libs are already discoverable)

#### Parameter settings ####

FV_GMM_NCOMPONENTS = 256
FASTFV_FLANN_NTREES = 4
FASTFV_FLANN_NCOMPARISONS = 32
FASTFV_UPDATESPERDESCRIPTOR = 5
FASTFV_GRIDS = 1x1x1x 1x3x1x 1x1x2x 2x2x1x 2x2x2x 1x3x2x

#####

export LD_LIBRARY_PATH := $(LIBS):$(LD_LIBRARY_PATH)
export PYTHONPATH := $(LIBS):$(PYTHONPATH)

trainClips := $(shell seq 823 | xargs printf "actioncliptrain%05d.avi ")
testClips := $(shell seq 884 | xargs printf "actioncliptest%05d.avi ")
allClips := $(trainClips) $(testClips)

videoDir := data/dataset/Hollywood2/AVIClips
evalDir := data/dataset/Hollywood2/ClipSets
dataset := $(addprefix $(videoDir)/, $(allClips))
features := $(addprefix data/features/, $(addsuffix .txt, $(allClips)))
features_sampled := data/features_sampled.txt
vocabs := $(addprefix data/vocabs/, 10-105.hog.gmm 106-213.hog.gmm 214-309.mbhx.gmm 310-405.mbhy.gmm)
fisher_vectors := $(addprefix data/fisher_vectors/, $(addsuffix .txt, $(allClips)))
kernel := data/kernel.txt
vlfeat := vlfeat-0.9.19/toolbox/plotop/vl_pr.m
classification := data/classification.txt

log = $(subst data, logs, $@).log
cut = $(basename $(basename $(notdir $(1))))

all: $(classification)
	./report.py

$(features): $(dataset)
	$(FASTVIDEOFEAT) $(videoDir)/$(basename $(notdir $@)) > $@ 2> $(log)

$(features_sampled): $(features)
	awk '{ if (rand() <= .01) print }' data/features/actioncliptrain* > $@

$(vocabs): $(features_sampled)
	cat $(features_sampled) | ../../src/gmm_train.py --gmm_ncomponents $(FV_GMM_NCOMPONENTS) --vocab $(call cut, $@) $@ > $(log) 2>&1

#$(vocabs): $(features_sampled)
#	@cut -d'	' -f $(shell echo $@ | grep -o '[0-9]\+-[0-9]\+') $(features_sampled) | ./pca_train.py $@.pca.npz
#	@cut -d'	' -f $(shell echo $@ | grep -o '[0-9]\+-[0-9]\+') $(features_sampled) | ./pca_test.py $@.pca.npz 0.5 | python gmm_train.py 256 > $@

$(fisher_vectors): $(features) $(vocabs)
	#cat $(subst fisher_vectors,features, $@) | $(FASTFV) $(foreach g, $(vocabs), --vocab $(call cut, $(g)) $(g)) --xnpos 0 --ynpos 1 --tnpos 2 --grid 1x1x1x --grid 1x3x1x --grid 1x1x2x --flann_ntrees $(FASTFV_FLANN_NTREES) --flann_ncomparisons $(FASTFV_FLANN_NCOMPARISONS) --updatesperdescriptor $(FASTFV_UPDATESPERDESCRIPTOR) > $@ 2> $(log)
	cat $(subst fisher_vectors,features, $@) | ./yaelfv_grids_1x1x1_1x3x1_1x1x2.py $(foreach g, $(vocabs), --vocab $(call cut, $(g)) $(g)) --xnpos 0 --ynpos 1 --tnpos 2 > $@ 2> $(log)

$(kernel) : $(fisher_vectors)
	@./dot_kernel.py $(fisher_vectors) > $@

$(classification) : $(kernel) $(vlfeat)
	@cat $(kernel) | ./classify.py $(evalDir) $(basename $(allClips)) > $@

.PHONY: all get_hwd2 get_vlfeat clean

get_hwd2:
	wget ftp://ftp.irisa.fr/local/vistas/actions/Hollywood2-actions.tar.gz
	tar -C data/dataset -xf Hollywood2-actions.tar.gz
	rm Hollywood2-actions.tar.gz

get_vlfeat:
	wget https://github.com/vlfeat/vlfeat/archive/v0.9.19.tar.gz
	tar -xf v0.9.19.tar.gz
	rm v0.9.19.tar.gz

clean:
	rm -rf data/features/* data/features_sampled.txt data/vocabs/* data/fisher_vectors/* data/kernel.txt logs/features/* logs/fisher_vectors/* logs/vocabs/*
