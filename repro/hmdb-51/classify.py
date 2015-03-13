#! /usr/bin/env python

import os
import sys
import itertools
import numpy as np
from sklearn.svm import SVC

EVAL_DIR, allClips = sys.argv[1], map(lambda l: l[:-1], open(sys.argv[2]))
all_k = np.loadtxt(sys.stdin)

classLabels = sorted(set([f.split('_test_split')[0] for f in os.listdir(EVAL_DIR) if '_test_split' in f]))
def read_split(SPLIT_IND):
	fixClipName = lambda x: reduce(lambda acc, ch: acc.replace(ch, '_'), '][;()&?!', x)
	train, test = [], []
	for classLabel in classLabels:
		d = dict(map(str.split, open(os.path.join(EVAL_DIR, '%s_test_split%d.txt' % (classLabel, 1 + SPLIT_IND)))))
		train += [(fixClipName(k), classLabel) for k, v in d.items() if v == '1']
		test += [(fixClipName(k), classLabel) for k, v in d.items() if v == '2']

	return (train, test)

splits = map(read_split, range(3))
slice_kernel = lambda inds1, inds2: all_k[np.ix_(map(allClips.index, inds1), map(allClips.index, inds2))]
REG_C = 1.0

def svm_train_test(train_k, test_k, ytrain, REG_C):
	model = SVC(kernel = 'precomputed', C = REG_C, max_iter = 10000)
	model.fit(train_k, ytrain)

	flatten = lambda ls: list(itertools.chain(*ls))
	train_conf, test_conf = map(flatten, map(model.decision_function, [train_k, test_k]))
	return train_conf, test_conf

def one_vs_rest(SPLIT_IND):
	calc_accuracy = lambda chosen, true: sum([int(true[i] == chosen[i]) for i in range(len(chosen))]) / float(len(chosen))
	partition = lambda f, ls: (filter(f, ls), list(itertools.ifilterfalse(f, ls)))
	train, test = splits[SPLIT_IND]
	xtest, ytest = zip(*test)

	confs = []
	for i, classLabel in enumerate(classLabels):
		postrain, negtrain = partition(lambda x: x[1] == classLabel, train)
		xtrain = zip(*(postrain + negtrain))[0]
		ytrain = [1]*len(postrain) + [-1]*len(negtrain)
		train_k, test_k = map(lambda x: slice_kernel(x, xtrain), [xtrain, xtest])
		train_conf, test_conf = svm_train_test(train_k, test_k, ytrain, REG_C)
		confs.append(test_conf)
	
	ntest = len(test)
	chosen = [max([(confs[k][i], k) for k in range(len(classLabels))])[1] for i in range(ntest)]
	true = [classLabels.index(test[i][1]) for i in range(ntest)]
	return calc_accuracy(chosen, true)

aps = []
for SPLIT_IND in range(len(splits)):
	test_ap = one_vs_rest(SPLIT_IND)
	aps.append(test_ap)
	print '%-15s: %.4f' % ('split_%d' % SPLIT_IND, test_ap)

print '\nmean: %.4f' % np.mean(aps)
