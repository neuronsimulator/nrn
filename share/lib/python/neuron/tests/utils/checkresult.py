import json
import math
from neuron import h, hoc


class Chk:
    """A test helper that compares stored results with test results
    chk = Chk(filename)

    chk("name", value) normally asserts that the value == dict[name]
    But, if dict[name] does not exist, the value is added to the dict.
    """

    def __init__(self, fname, must_exist=True):
        """JSON with name, fname, must be a dict.
        Assert file exists by default.
        A temporary must_exist=False provides a simple way for the test
        author to bootstrap the file.
        """
        import os

        self.fname = fname
        self.d = {}
        self.modified = False
        if must_exist:
            assert os.path.exists(fname)
        else:
            if not os.path.exists(fname):
                return

        with open(fname, "rb") as f:
            self.d = json.load(f)
        assert type(self.d) is dict

    def __call__(self, key, value, tol=0.0):
        """Assert value == dict[key] unless value is a hoc_Vector.
        For the Vector case, assert the maximum relative element difference is <= tol.
        If the key does not exist add {key:value} to the dict.
        """

        if key in self.d:
            if isinstance(value, hoc.Vector):
                # Convert to list to keep the `equal` method below simple
                value = list(value)
            # Hand-rolled comparison that uses `tol` for arithmetic values
            # buried inside lists of lists.
            def equal(a, b):
                assert type(a) == type(b)
                if type(a) in (float, int):
                    match = math.isclose(a, b, rel_tol=tol)
                    if not match:
                        print(a, b, "diff", abs(a - b) / max(abs(a), abs(b)), ">", tol)
                    return match
                elif type(a) == str:
                    match = a == b
                    if not match:
                        print("strdiff", a, b)
                    return match
                elif type(a) == list:
                    # List comprehension avoids short-circuit, so the "diff"
                    # message just above is printed for all elements
                    return all([equal(aa, bb) for aa, bb in zip(a, b)])
                elif type(a) == dict:
                    assert a.keys() == b.keys()
                    return all([equal(a[k], b[k]) for k in a.keys()])
                raise Exception(
                    "Don't know how to compare objects of type " + str(type(a))
                )

            match = equal(value, self.d[key])
            if not match:
                print(key, "difference")
                print("std = ", self.d[key])
                print("val = ", value)
            assert match
        else:
            print("{} added {}".format(self, key))
            if isinstance(value, hoc.Vector):
                self.d[key] = value.to_python()
            else:
                self.d[key] = value
            self.modified = True

    def get(self, key, default=None):
        """Get an element from the dict. Returns default if it doesn't exist."""
        return self.d.get(key, default)

    def rm(self, key):
        """Remove key from dict.
        Only needed temporarily when a key value needs updating.
        """
        if key in self.d:
            del self.d[key]
            self.modified = True

    def save(self):
        """If dict has been modified, save back to original filename
        This is typically called at the end of the test and normally
        does nothing unless the author adds a new chk(...) to the test.
        """
        if self.modified:
            with open(self.fname, "w") as f:
                json.dump(self.d, f, indent=2)
                f.write("\n")
