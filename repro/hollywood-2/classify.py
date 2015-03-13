#! /usr/bin/env python

import os
import re
import sys
import itertools
import numpy as np
from sklearn.cross_validation import StratifiedKFold
from sklearn.svm import SVC
from oct2py import octave

EVAL_DIR, allClipsNoExt = sys.argv[1], map(lambda l: os.path.splitext(l[:-1])[0], open(sys.argv[2]))
all_k = np.loadtxt(sys.stdin)

CS = np.logspace(-6, 2, 10)

classLabels = sorted(set([x.split('_')[0] for x in os.listdir(EVAL_DIR) if re.match('[A-Z].+_(train|test)\.txt', x)]))
hwd2_labels = lambda pref, classLabel: map(np.array, zip(*np.genfromtxt(os.path.join(EVAL_DIR, '%s_%s.txt' % (classLabel, pref)), dtype = None)))
slice_kernel = lambda inds1, inds2: all_k[np.ix_(map(allClipsNoExt.index, inds1), map(allClipsNoExt.index, inds2))]

octave.addpath('vlfeat-0.9.19/toolbox/misc')
octave.addpath('vlfeat-0.9.19/toolbox/plotop')

def svm_train_test(train_k, test_k, ytrain, ytest, REG_C):
	ytrain = list(ytrain)

	model = SVC(kernel = 'precomputed', C = REG_C, max_iter = 10000)
	model.fit(train_k, ytrain)

	flatten = lambda ls: list(itertools.chain(*ls))
	train_conf, test_conf = map(flatten, map(model.decision_function, [train_k, test_k]))
	
	rec, prec, trainInfo = octave.vl_pr(ytrain, train_conf)
	rec, prec, testInfo = octave.vl_pr(ytest, test_conf)

	return trainInfo['auc_pa08'], testInfo['auc_pa08']

aps = []
for CLS_LAB in classLabels:
	xtrain_orig, ytrain_orig = hwd2_labels('train', CLS_LAB)
	xtest_orig, ytest_orig = hwd2_labels('test', CLS_LAB)

	skf = StratifiedKFold(ytrain_orig, n_folds = 5)

	val_aps = np.zeros((len(skf), len(CS)))
	for i, (train_ind, val_ind) in enumerate(skf):
		xtrain, xval = xtrain_orig[train_ind], xtrain_orig[val_ind]
		ytrain, yval = ytrain_orig[train_ind], ytrain_orig[val_ind]
		
		train_k, val_k = slice_kernel(xtrain, xtrain), slice_kernel(xval, xtrain)
		for j, REG_C in enumerate(CS):
			train_ap, val_ap = svm_train_test(train_k, val_k, ytrain, yval, REG_C)
			val_aps[i, j] = val_ap

	val_map = val_aps.mean(0)

	REG_C = CS[val_map.argmax()]
	train_k_orig, test_k_orig = slice_kernel(xtrain_orig, xtrain_orig), slice_kernel(xtest_orig, xtrain_orig)
	train_ap, test_ap = svm_train_test(train_k_orig, test_k_orig, ytrain_orig, ytest_orig, REG_C)
	aps.append(test_ap)
	print '%-15s: %.4f' % (CLS_LAB, test_ap)

print '\nmean: %.4f' % np.mean(aps)
