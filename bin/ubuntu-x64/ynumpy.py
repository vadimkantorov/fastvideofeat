"""
This is a wrapper for yael's functions, so that all I/O of vectors and
matrices is done via numpy types. All errors are also raised as exceptions.

Unlike Yael, the matrices are row-major (numpy's support for
column-major is terrible).

"""

import pdb 

import numpy
import yael

####################################################
# Format checks



def _check_row_float32(a): 
    if a.dtype != numpy.float32: 
        raise TypeError('expected float32 matrix, got %s' % a.dtype)
    if not a.flags.c_contiguous:
        raise TypeError('expected C order matrix')

def _check_row_uint8(a): 
    if a.dtype != numpy.uint8: 
        raise TypeError('expected uint8 matrix, got %s' % a.dtype)
    if not a.flags.c_contiguous:
        raise TypeError('expected C order matrix')

def _check_row_int32(a): 
    if a.dtype != numpy.int32: 
        raise TypeError('expected int32 matrix, got %s' % a.dtype)
    if not a.flags.c_contiguous:
        raise TypeError('expected C order matrix')

####################################################
# NN function & related


def knn(queries, base, 
        nnn = 1, 
        distance_type = 2,
        nt = 1):
    _check_row_float32(base)
    _check_row_float32(queries)
    n, d = base.shape
    nq, d2 = queries.shape
    assert d == d2, "base and queries must have same nb of rows (got %d != %d) " % (d, d2)
    
    idx = numpy.empty((nq, nnn), dtype = numpy.int32)
    dis = numpy.empty((nq, nnn), dtype = numpy.float32)

    yael.knn_full_thread(distance_type, 
                         nq, n, d, nnn,
                         yael.numpy_to_fvec_ref(base),
                         yael.numpy_to_fvec_ref(queries), 
                         None, 
                         yael.numpy_to_ivec_ref(idx), 
                         yael.numpy_to_fvec_ref(dis), 
                         nt)
    return idx, dis
    
def cross_distances(a, b, distance_type = 12):
    _check_row_float32(a)
    na, d = a.shape
    _check_row_float32(b)
    nb, d2 = b.shape

    assert d2 == d

    dis = numpy.empty((nb, na), dtype = numpy.float32)

    yael.compute_cross_distances_alt_nonpacked(distance_type, d, na, nb,
                                               yael.numpy_to_fvec_ref(a), d,
                                               yael.numpy_to_fvec_ref(b), d,
                                               yael.numpy_to_fvec_ref(dis), na)
    
    return dis                                 


def kmeans(v, k,
           distance_type = 2, 
           nt = 1, 
           niter = 30,
           seed = 0, 
           redo = 1, 
           verbose = True,
           normalize = False, 
           init = 'random',
           output = 'centroids'):
    _check_row_float32(v)
    n, d = v.shape
    
    centroids = numpy.zeros((k, d), dtype = numpy.float32)
    dis = numpy.empty(n, dtype = numpy.float32)
    assign = numpy.empty(n, dtype = numpy.int32)
    nassign = numpy.empty(k, dtype = numpy.int32)
    
    flags = nt 
    if not verbose:          flags |= yael.KMEANS_QUIET

    if distance_type == 2:   pass # default
    elif distance_type == 1: flags |= yael.KMEANS_L1
    elif distance_type == 3: flags |= yael.KMEANS_CHI2

    if init == 'random':     flags |= yael.KMEANS_INIT_RANDOM # also default
    elif init == 'kmeans++': flags |= yael.KMEANS_INIT_BERKELEY 

    if normalize:            flags |= yael.KMEANS_NORMALIZE_CENTS

    qerr = yael.kmeans(d, n, k, niter, 
                       yael.numpy_to_fvec_ref(v), flags, seed, redo, 
                       yael.numpy_to_fvec_ref(centroids), 
                       yael.numpy_to_fvec_ref(dis), 
                       yael.numpy_to_ivec_ref(assign), 
                       yael.numpy_to_ivec_ref(nassign))

    if qerr < 0: 
        raise RuntimeError("kmeans: clustering failed. Is dataset diverse enough?")

    if output == 'centroids': 
        return centroids
    else: 
        return (centroids, qerr, dis, assign, nassign)


def partial_pca(mat, nev = 6, nt = 1):
    _check_row_float32(mat)
    n, d = mat.shape

    avg = mat.mean(axis = 0)
    mat = mat - avg[numpy.newaxis, :]
    
    singvals = numpy.empty(nev, dtype = numpy.float32)
    # pdb.set_trace()

    pcamat = yael.fmat_new_pca_part(d, n, nev,
                                    yael.numpy_to_fvec_ref(mat),
                                    yael.numpy_to_fvec_ref(singvals))
    assert pcamat != None
    
    #print "SVs", singvals
    pcamat = yael.fvec.acquirepointer(pcamat)
    pcamat = yael.fvec_to_numpy(pcamat, (nev, d))

    return avg, singvals, pcamat
    

####################################################
# I/O

def fvecs_fsize(filename): 
    (fsize, d, n) = yael.fvecs_fsize(filename)
    if n < 0 and d < 0: 
        return IOError("fvecs_fsize: cannot read " + filename)
    # WARN: if file is empty, (d, n) = (-1, 0)
    return (n, d)

def fvecs_read(filename, nmax = -1, c_contiguous = True):   
    if nmax < 0:
        fv = numpy.fromfile(filename, dtype = numpy.float32)
        if fv.size == 0:
            return numpy.zeros((0,0))            
        dim = fv.view(numpy.int32)[0] 
        assert dim>0
        fv = fv.reshape(-1,1+dim)
        if not all(fv.view(numpy.int32)[:,0]==dim):
            raise IOError("non-uniform vector sizes in " + filename)
        fv = fv[:,1:]
        if c_contiguous:
            fv = fv.copy()
        return fv
    (fvecs, n, d) = yael.fvecs_new_fread_max(open(filename, "r"), nmax)
    if n == -1: 
        raise IOError("could not read " + filename)
    elif n == 0: d = 0    
    fvecs = yael.fvec.acquirepointer(fvecs)
    # TODO find a way to avoid copy
    a = yael.fvec_to_numpy(fvecs, n * d)
    return a.reshape((n, d))

def ivecs_read(filename):
    (fvecs, n, d) = yael.ivecs_new_read(filename)
    if n == -1: 
        raise IOError("could not read " + filename)
    elif n == 0: d = 0    
    ivecs = yael.ivec.acquirepointer(fvecs)
    # TODO find a way to avoid copy
    a = yael.ivec_to_numpy(fvecs, n * d)
    return a.reshape((n, d))



def fvecs_write(filename, matrix): 
    _check_row_float32(matrix)
    n, d = matrix.shape
    ret = yael.fvecs_write(filename, d, n, yael.numpy_to_fvec_ref(matrix))
    if ret != n:
        raise IOError("write error" + filename)


def fvecs_fwrite(fd, matrix): 
    _check_row_float32(matrix)
    n, d = matrix.shape
    ret = yael.fvecs_fwrite(fd, d, n, yael.numpy_to_fvec_ref(matrix))
    if ret != n:
        raise IOError("write error")


def bvecs_write(filename, matrix):
    _check_row_uint8(matrix)
    n, d = matrix.shape
    ret = yael.bvecs_write(filename, d, n, yael.numpy_to_bvec_ref(matrix))
    if ret != n:
        raise IOError("write error" + filename)
    

def ivecs_write(filename, matrix): 
    assert False, "not implemented"

def siftgeo_read(filename):

    # I/O via double pointers (too lazy to make proper swig interface)
    v_out = yael.BytePtrArray(1)
    meta_out = yael.FloatPtrArray(1)
    d_out = yael.ivec(2)

    n = yael.bvecs_new_from_siftgeo(filename, d_out, v_out.cast(),     
                                    d_out.plus(1), meta_out.cast())
    
    if n < 0: 
        raise IOError("cannot read " + filename)
    if n == 0: 
        v = numpy.array([[]], dtype = numpy.uint8)
        meta = numpy.array([[]*9], dtype = numpy.float32)
        return v, meta

    v_out = yael.bvec.acquirepointer(v_out[0])
    meta_out = yael.fvec.acquirepointer(meta_out[0])

    d = d_out[0]
    d_meta = d_out[1]
    assert d_meta == 9

    v = yael.bvec_to_numpy(v_out, n * d)
    v = v.reshape((n, d))
    
    meta = yael.fvec_to_numpy(meta_out, n * d_meta)
    meta = meta.reshape((n, d_meta))

    return v, meta

####################################################
# GMM & Fisher manipulation


# In numpy, we represent gmm's as 3 matrices (like in matlab)
# when a gmm is needed, we build a "fake" yael gmm struct with 
# 3 vectors

def _gmm_to_numpy(gmm): 
    d, k = gmm.d, gmm.k
    w = yael.fvec_to_numpy(gmm.w, k)
    mu = yael.fvec_to_numpy(gmm.mu, d * k)
    mu = mu.reshape((k, d))
    sigma = yael.fvec_to_numpy(gmm.sigma, d * k)
    sigma = sigma.reshape((k, d))
    return w, mu, sigma

def _gmm_del(gmm): 
    gmm.mu = gmm.sigma = gmm.w = None
    yael.gmm_delete(gmm)
    # yael._yael.delete_gmm_t(gmm)


def _numpy_to_gmm((w, mu, sigma)):
    # produce a fake gmm from 3 numpy matrices. They should not be
    # deallocated while gmm in use
    _check_row_float32(mu)
    _check_row_float32(sigma)
    
    k, d = mu.shape
    assert sigma.shape == mu.shape
    assert w.shape == (k,)

    gmm = yael.gmm_t()
    gmm.d = d
    gmm.k = k
    gmm.w = yael.numpy_to_fvec_ref(w)
    gmm.mu = yael.numpy_to_fvec_ref(mu)
    gmm.sigma = yael.numpy_to_fvec_ref(sigma)
    gmm.__del__ = _gmm_del
    return gmm


def gmm_learn(v, k,
              nt = 1,
              niter = 30,
              seed = 0,
              redo = 1,
              use_weights = True): 
    _check_row_float32(v)
    n, d = v.shape
    
    flags = 0
    if use_weights: flags |= yael.GMM_FLAGS_W

    gmm = yael.gmm_learn(d, n, k, niter, 
                         yael.numpy_to_fvec_ref(v), nt, seed, redo, flags)
    
    gmm_npy = _gmm_to_numpy(gmm) 

    yael.gmm_delete(gmm)    
    return gmm_npy

def gmm_read(filename):
    gmm = yael.gmm_read(open(filename, "r"))
    gmm_npy = _gmm_to_numpy(gmm) 

    yael.gmm_delete(gmm)    
    return gmm_npy

        

def fisher(gmm_npy, v, 
           include = 'mu'): 

    _check_row_float32(v)
    n, d = v.shape

    gmm = _numpy_to_gmm(gmm_npy)
    assert d == gmm.d
    
    flags = 0

    if 'mu' in include:    flags |= yael.GMM_FLAGS_MU
    if 'sigma' in include: flags |= yael.GMM_FLAGS_SIGMA
    if 'w' in include:     flags |= yael.GMM_FLAGS_W

    d_fisher = yael.gmm_fisher_sizeof(gmm, flags)

    fisher_out = numpy.zeros(d_fisher, dtype = numpy.float32)    

    yael.gmm_fisher(n, yael.numpy_to_fvec_ref(v), gmm, flags, yael.numpy_to_fvec_ref(fisher_out))

    return fisher_out

####################################################
# Fast versions of slow Numpy operations

    
def extract_lines(a, indices):
    " returns a[indices, :] from a matrix a (this operation is slow in numpy) "
    _check_row_float32(a)
    _check_row_int32(indices)
    n, d = a.shape
    assert indices.size == 0 or indices.min() >= 0 and indices.max() < n
    out = numpy.empty((indices.size, d), dtype = numpy.float32)
    yael.fmat_get_columns(yael.numpy_to_fvec_ref(a),
                          d, indices.size,
                          yael.numpy_to_ivec_ref(indices),
                          yael.numpy_to_fvec_ref(out))

    return out
    
def extract_rows_cols(K, subset_rows, subset_cols):
    " returns K[numpy.ix_(subset_rows, subset_cols)] (also slow in pure numpy)"
    _check_row_float32(K)
    _check_row_int32(subset_rows)
    _check_row_int32(subset_cols)
    nr = subset_rows.size
    nc = subset_cols.size
    assert subset_rows.min() >= 0 and subset_rows.max() < K.shape[0]
    assert subset_cols.min() >= 0 and subset_cols.max() < K.shape[1]    
    Ksub = numpy.empty((nr, nc), dtype = numpy.float32)
    yael.fmat_get_rows_cols(yael.numpy_to_fvec_ref(K),
                            K.shape[0],
                            nc, yael.numpy_to_ivec_ref(subset_cols),
                            nr, yael.numpy_to_ivec_ref(subset_rows),
                            yael.numpy_to_fvec_ref(Ksub))
    return Ksub


