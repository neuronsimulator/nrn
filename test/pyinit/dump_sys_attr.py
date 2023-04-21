from functools import reduce
import json
import sys

fname, attr = sys.argv[-2:]
data = reduce(getattr, [sys] + attr.split("."))
print("dumping sys.{} = {} to {}".format(attr, data, fname))
with open(fname, "w") as ofile:
    json.dump(data, ofile)
