import sys


voidfun = []
intvar = [[],[]] #scalarint vectorint
fltvar = [[],[]] #scalarfloat, vectorfloat
dblvar = [[],[],[],[]] # scalar, vectordouble, arraydouble, threedim

def processfun(a, names):
  # each name has trailing '()'
  for i in names:
    a.append(i.strip('()'))

def processvar(a, names):
  # each name may have 0-3 trailing [%d]
  for i in names:
    sp = i.split('[')
    b = []
    for j in sp:
      b.append(j.strip(']'))
    a[len(b)-1].append(b)

types = {}
types['void'] = (voidfun, processfun)
types['int'] = (intvar, processvar)
types['float'] = (fltvar, processvar)
types['double'] = (dblvar, processvar)

def process(type, names):
  types[type][1](types[type][0], names)

for line in sys.stdin:
  names = line.replace(',',' ').replace(';',' ').split()
  if len(names) > 2:
    process(names[1], names[2:])
    

print '''
/* Functions */
static VoidFunc function[] = {
''',

for i in voidfun:
  print '"%s", %s,'%(i,i)
print '''0, 0
};

static struct {  /* Integer Scalars */
  const char *name;
  int  *pint;
} scint[] = {
''',

for i in intvar[0]:
  print '"%s", &%s,'%(i[0],i[0])
print '''0, 0
};

static struct {  /* Vector integers */
  const char *name;
  int  *pint;
  int  index1;
} vint[] = {
''',

for i in intvar[1]:
  print '"%s", %s, %s,'%(i[0],i[0],i[1])
print '''0,0
};

static struct {  /* Float Scalars */
  const char *name;
  float  *pfloat;
} scfloat[] = {
''',

for i in fltvar[0]:
  print '"%s", &%s,'%(i[0],i[0])
print '''0, 0
};

static struct {  /* Vector float */
  const char *name;
  float *pfloat;
  int index1;
} vfloat[] = {
''',

for i in fltvar[1]:
  print '"%s", %s, %s,'%(i[0],i[0],i[1])
print '''0,0,0
};

/* Double Scalars */
DoubScal scdoub[] = {
''',

for i in dblvar[0]:
  print '"%s", &%s,'%(i[0],i[0])
print '''0,0
};

/* Vectors */
DoubVec vdoub[] = {
''',

for i in dblvar[1]:
  print '"%s", %s, %s,'%(i[0],i[0],i[1])
print '''0, 0, 0
};

static struct {  /* Arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
} ardoub[] = {
''',

for i in dblvar[2]:
  print '"%s", %s, %s, %s,'%(i[0], i[0], i[1], i[2])
print '''0, 0, 0, 0
};

static struct {  /* triple dimensioned arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
  int index3;
} thredim[] = {
''',

for i in dblvar[3]:
  print '"%s", %s, %s, %s, %s,'%(i[0], i[0], i[1], i[2], i[3])
print '''0, 0, 0, 0, 0
};
'''
