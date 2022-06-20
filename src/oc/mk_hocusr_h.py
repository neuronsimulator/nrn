import sys
import re

voidfun = []
intvar = [[], []]  # scalarint vectorint
fltvar = [[], []]  # scalarfloat, vectorfloat
dblvar = [[], [], [], []]  # scalar, vectordouble, arraydouble, threedim


def processfun(a, names):
    # each name has trailing '()'
    for i in names:
        a.append(i.strip("()"))


def processvar(a, names):
    # each name may have 0-3 trailing [%d]
    for i in names:
        sp = i.split("[")
        b = []
        for j in sp:
            b.append(j.strip("]"))
        a[len(b) - 1].append(b)


def remove_multiline_comments(string):
    # remove all occurance comments (/*COMMENT */) from string
    string = re.sub(re.compile("/\*.*?\*/", re.DOTALL), "", string)
    return string


types = {}
types["void"] = (voidfun, processfun)
types["int"] = (intvar, processvar)
types["float"] = (fltvar, processvar)
types["double"] = (dblvar, processvar)


def process(type, names):
    types[type][1](types[type][0], names)


# read preprocessor output file into buffer
text = sys.stdin.read()

# the PGI preprocessor output i.e. 'pgcc -Mcpp -E'
# has multi-line comments (unless we specify -Mcpp=nocomment).
# we need to remove these multi-line comments
text = remove_multiline_comments(text)

# the pgcc 18.4 compiler prepends with a multiline typedef and several
# extern void lines that need to be skipped. Our first relevant line
# contains spatial_method or neuron2nemo or node_data

skip = 1
for line in text.splitlines():
    if "spatial_method" in line or "neuron2nemo" in line or "node_data" in line:
        skip = 0
    names = line.replace(",", " ").replace(";", " ").split()
    if not skip and len(names) > 2:
        print(line)  # entire filtered neuron.h file without pgcc added lines.
        assert names[0] == "extern"
        names.pop(0)
        if names[0] in {'"C"', '"C++"'}:
            names.pop(0)
        process(names[0], names[1:])


print(
    """
/* Functions */
static VoidFunc functions[] = {
"""
)

for i in voidfun:
    if i:
        prefix = "nrnhoc_"
        j = i[len(prefix) :] if i.startswith(prefix) else i
        print('"%s", %s,' % (j, i))
print(
    """0, 0
};

static struct {  /* Integer Scalars */
  const char *name;
  int  *pint;
} scint[] = {
"""
)

for i in intvar[0]:
    print('"%s", &%s,' % (i[0], i[0]))
print(
    """0, 0
};

static struct {  /* Vector integers */
  const char *name;
  int  *pint;
  int  index1;
} vint[] = {
"""
)

for i in intvar[1]:
    print('"%s", %s, %s,' % (i[0], i[0], i[1]))
print(
    """0,0
};

static struct {  /* Float Scalars */
  const char *name;
  float  *pfloat;
} scfloat[] = {
"""
)

for i in fltvar[0]:
    print('"%s", &%s,' % (i[0], i[0]))
print(
    """0, 0
};

static struct {  /* Vector float */
  const char *name;
  float *pfloat;
  int index1;
} vfloat[] = {
"""
)

for i in fltvar[1]:
    print('"%s", %s, %s,' % (i[0], i[0], i[1]))
print(
    """0,0,0
};

/* Double Scalars */
DoubScal scdoub[] = {
"""
)

for i in dblvar[0]:
    print('"%s", &%s,' % (i[0], i[0]))
print(
    """0,0
};

/* Vectors */
DoubVec vdoub[] = {
"""
)

for i in dblvar[1]:
    print('"%s", %s, %s,' % (i[0], i[0], i[1]))
print(
    """0, 0, 0
};

static struct {  /* Arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
} ardoub[] = {
"""
)

for i in dblvar[2]:
    print('"%s", %s, %s, %s,' % (i[0], i[0], i[1], i[2]))
print(
    """0, 0, 0, 0
};

static struct {  /* triple dimensioned arrays */
  const char *name;
  double *pdoub;
  int index1;
  int index2;
  int index3;
} thredim[] = {
"""
)

for i in dblvar[3]:
    print('"%s", %s, %s, %s, %s,' % (i[0], i[0], i[1], i[2], i[3]))
print(
    """0, 0, 0, 0, 0
};
"""
)
