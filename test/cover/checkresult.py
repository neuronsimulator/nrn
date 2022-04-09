import json
from neuron import h


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
            print("opened {} d={}".format(fname, self.d))
        assert type(self.d) is dict

    def __call__(self, key, value, tol=0.0):
        """Assert value == dict[key] unless value is a hoc_Vector.
        For the Vector case, assert the maximum element difference is <= tol.
        If the key does not exist add {key:value} to the dict.
        """

        if key in self.d:
            if type(value) == type(h.Vector):  # actually hoc.HocObject
                m = h.Vector(self.d[key]).sub(value).abs().max()
                if m > tol:
                    print(key, " difference > ", tol)
                    print(m)
                assert m <= tol
                return
            if self.d[key] != value:
                print(key, " difference")
                print("std = ", self.d[key])
                print("\nval = ", value)
            assert self.d[key] == value
        else:
            print("{} added {}".format(self, key))
            if type(value) == type(h.Vector):  # actually hoc.HocObject
                self.d[key] = value.to_python()
            else:
                self.d[key] = value
            self.modified = True

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
