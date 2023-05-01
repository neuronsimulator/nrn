from functools import reduce
import json
import sys

fname, attr = sys.argv[-2:]
data = reduce(getattr, [sys] + attr.split("."))
if attr == "path" and sys.argv[-3] == "override_sys_path_0_to_be_empty":
    data[0] = ""
print("dumping sys.{} = {} to {}".format(attr, data, fname))
with open(fname, "w") as ofile:
    json.dump(data, ofile)
