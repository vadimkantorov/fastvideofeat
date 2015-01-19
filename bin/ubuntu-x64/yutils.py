# Put various functions in this file that will go in Yael at some point
import sys, os, types
import yael


# Read a file in siftgeo format
def siftgeo_read(filename, outfmt='bvec'):
    # I/O via double pointers (too lazy to make proper swig interface)
    v_out = yael.BytePtrArray(1)
    meta_out = yael.FloatPtrArray(1)
    d_out = yael.ivec(2)

    n = yael.bvecs_new_from_siftgeo(filename, d_out, v_out.cast(),     
                                    d_out.plus(1), meta_out.cast())
    
    if n < 0: 
        raise IOError("cannot read " + filename)
    if n == 0: 
        v = None
        meta = None
        return v, meta, n

    d = d_out[0]
    d_meta = d_out[1]
    assert d_meta == 9

    v = yael.bvec.acquirepointer(v_out[0])

    if outfmt == 'fvec': 
        v = yael.bvec2fvec (v_out[0], n * d)
        v = yael.fvec.acquirepointer(v)

    meta = yael.fvec.acquirepointer(meta_out[0])
    return v, meta, n


def vecfile_stats (fname, d, fmt):

    sz = os.stat(fname).st_size

    if fmt=='fvecs':     vecsize = 4*d+4
    elif fmt=='bvecs':   vecsize = d+4
    elif fmt=='rawf':    vecsize = 4*d
    elif fmt=='rawb':    vecsize = d
    elif fmt=='siftgeo': vecsize = 168

    assert sz % vecsize == 0
    ninfile = sz / vecsize

    return (ninfile, vecsize, sz)


def load_vectors_fmt(fname,fmt,d,nuse=None,off=0,verbose=True):

    (ninfile, vecsize, sz) = vecfile_stats (fname, d, fmt)
    
    if not nuse or nuse==0: nuse = ninfile
    if verbose:
        print 'load %s: use %d/%d vectors (d=%d,% d bytes,fmt=%s,start=%d))' % (fname, nuse, ninfile, 
                                                                                d, sz, fmt, off)
    f = open (fname, 'r')
    f.seek (off * vecsize)
    n = 0

    if fmt=='fvecs':
        v = yael.fvec (nuse * long (d))
        n = yael.fvecs_fread (f, v, nuse, d)

    elif fmt=='bvecs': 
        v = yael.bvec (nuse * long(d))
        n = yael.bvecs_fread (f, v, nuse, d)

    elif fmt=='rawf':
        v = yael.fvec (nuse * long(d))
        n = yael.fvec_fread_raw(f, v, nuse * long(d)) / d
        
    elif fmt=='rawb':
        v = yael.bvec (nuse * long(d))
        n = yael.bvec_fread_raw(f, v, nuse * long(d)) / d

    elif fmt=='siftgeo':
        v, meta, n = siftgeo_read (fname)
        assert d2==d,"dim: expected %d, got %d"%(d,d2)
        n=pts.n

    elif fmt=='spfvecs':
        (n, v)=yael.fvecs_new_read_sparse(fname,d)
        nuse = n
    else:
         assert False             

    f.close()

    assert n == nuse
    return (v,n)


def prepare_dir(fname):
  "make sure the enclosing directory of this file exists"
  dirname=fname[:fname.rfind('/')]
  if os.access(dirname,os.W_OK): 
    return
  try:
    os.makedirs(dirname)
  except OSError,e:
    if e.errno!=errno.EEXIST:
      raise


def parse_as_type(ty,sval):
  """ interpret string sval as the same type as vty """
  if ty==types.BooleanType:
    if sval.lower() in ("0","false"): return False
    if sval.lower() in ("1","true"): return True
    raise ValueError("cannot interpret %s as boolean"%sval)
  else: return ty(sval)
