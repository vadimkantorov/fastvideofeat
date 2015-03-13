#! /usr/bin/env python

import os
import sys
import itertools
import numpy as np
import numpy.linalg as linalg

IN = [os.path.join(sys.argv[1], x[:-1] + '.txt') for x in open(sys.argv[2])]

skipComments = lambda path: itertools.ifilter(lambda x: not x.startswith('#'), open(path))
ks = [None]*len(list(skipComments(IN[0])))
streams = map(skipComments, IN)

def fv_norm(fv):
	fv = np.clip(fv, -1000, 1000)
	fv = np.sign(fv) * np.sqrt(np.abs(fv))
	fv /= (1e-4 + linalg.norm(fv))
	return fv

for i in range(len(ks)):
	x = np.vstack(tuple([fv_norm(np.fromstring(s.next(), dtype = np.float32, sep = '\t')) for s in streams]))
	ks[i] = np.dot(x, x.T)

res = reduce(np.add, ks)
np.savetxt(sys.stdout, res, fmt = '%.6f', delimiter = '\t')
