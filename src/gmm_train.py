#! /usr/bin/env python

import sys
import argparse
import numpy as np
import yael

parser = argparse.ArgumentParser()
parser.add_argument('--gmm_ncomponents', type = int, required = True)
parser.add_argument('--vocab', nargs = 2, required = True)
args = parser.parse_args()
cutFrom, cutTo = map(int, args.vocab[0].split('-'))

data = np.loadtxt(sys.stdin, dtype = np.float32, usecols = range(cutFrom, 1 + cutTo))

npoints, nfeatures = data.shape
niter = 50
nthreads = 1
seed = 0
redo = 1
flags = yael.GMM_FLAGS_MU | yael.GMM_FLAGS_SIGMA | yael.GMM_FLAGS_NO_NORM
gmm = yael.gmm_learn(nfeatures, npoints, args.gmm_ncomponents, niter, yael.FloatArray.acquirepointer(yael.numpy_to_fvec(data)), nthreads, seed, redo, flags)

yael.gmm_write(gmm, open(args.vocab[1], 'w'))
