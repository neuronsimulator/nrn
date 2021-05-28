try:
    from xml.etree import cElementTree as etree
except ImportError:
    from xml.etree import ElementTree as etree

from . import xml2nrn

# module names derived from the namespace. Add new tags in proper namespace
from . import neuroml
from . import metadata
from . import morphml
from . import biophysics


class FileWrapper:
    def __init__(self, source):
        self.source = source
        self.lineno = 0

    def read(self, bytes):
        s = self.source.readline()
        self.lineno += 1
        return s


# for each '{namespace}element' call the corresponding module.func
def handle(x2n, fw, event, node):
    tag = node.tag.split("}")
    # hopefully a namespace token corresponding to an imported module name
    ns = tag[0].split("/")[-2]
    tag = ns + "." + tag[1]  # namespace.element should correspond to module.func
    f = None
    try:
        if event == "start":
            f = eval(tag)
        elif event == "end":
            f = eval(tag + "_end")
    except:
        pass
    if f:
        x2n.locator.lineno = fw.lineno
        try:
            f(x2n, node)  # handle the element when it opens
        except:
            print(tag, " failed at ", x2n.locator.getLineNumber())
    elif event == "start":
        print("ignore", node.tag)  # no function to handle the element
        return 0
    return 1


def rdxml(fname, ho=None):
    f = FileWrapper(open(fname))
    x2n = xml2nrn.XML2Nrn()
    ig = None
    for event, elem in etree.iterparse(f, events=("start", "end")):
        if ig != elem:
            if handle(x2n, f, event, elem) == 0:
                ig = elem
    if ho:
        ho.parsed(x2n)


if __name__ == "__main__":
    rdxml("temp.xml")
