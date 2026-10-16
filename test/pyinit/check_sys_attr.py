from functools import reduce
import json
import sys

fname, attr = sys.argv[-2:]
data = reduce(getattr, [sys] + attr.split("."))
print("checking sys.{} = {} against reference file {}".format(attr, data, fname))
with open(fname) as ifile:
    ref_data = json.load(ifile)
print("reference value is {}".format(ref_data))
assert data == ref_data
