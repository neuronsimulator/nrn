try:
  from xml.etree import cElementTree as etree
except ImportError:
  from xml.etree import ElementTree as etree

import xml2nrn

# module names derived from the namespace. Add new tags in proper namespace
import neuroml
import metadata
import morphml
import biophysics

# for each '{namespace}element' call the corresponding module.func
def walk_tree(x2n, node):
  tag = node.tag.split('}')
  # hopefully a namespace token corresponding to an imported module name
  ns = tag[0].split('/')[-2]
  tag = ns+'.'+tag[1] #namespace.element should correspond to module.func
  #print tag
  f = None
  try:
    f = eval(tag)
  except:
    pass
  if f:
    f(x2n, node) # handle the element when it opens
    for child in node.getchildren():
      walk_tree(x2n, child)
    f = None
    try:
      f = eval(tag+'_end')
    except:
      pass
    if (f):
      f(x2n, node) #perhaps some computing on the closing element tag
  else:
    print 'ignore', node.tag # no function to handle the element

def rdxml(fname, ho = None):
  doc = etree.parse(fname)
  root = doc.getroot()
  x2n = xml2nrn.XML2Nrn()
  walk_tree(x2n, root)
  if (ho):
    ho.parsed(x2n)

if __name__ == '__main__':
  rdxml('temp.xml')
