from neuron import h, gui
h.load_file("import3d.hoc")

tst_data = ['''
# One-point soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  3 0 10 0 1 1
3  3 0 20 0 1 2
4  3 0 -10 0 2 1
5  3 0 -20 0 2 4
6  3 10 0 0 .5 1
7  3 10 5 0 1 6
''','''
# Multi-point soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 2 0 0 10  1
3  1 4 0 0 10  2
4  3 2 10 0   1  2
5  3 2 18 0  1  4
''','''
# dendrites at ends of multisection soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 5 -1
2  1 10 0 0 3 1
3  1 -8 -6 0 3 1
4  1 -8 6 0 3 1
5  3 20 0 0 1 2
6  3 -16 -12 0 1 3
7  3 -16 12 0 1 4
''','''
#two section soma with dendrites at ends and off point 1
#n,type,x,y,z,radius,parent
1 1 0 0 0 5 -1
2 1 5 0 0 3 1
3 1 -5 0 0 3 1
4 3 10 0 0 1 2
5 3 -10 0 0 1 3
6 3 0 5 0 1 1
7 3 0 10 0 1 6
''','''
# Multipoint soma but not geometrically realizable
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 2 0 0 10  1
3  1 4 0 0 10  2
4  3 2 1 0  1  2
5  3 2 9 0  1  4
''','''
# 4pt soma with dendrite off pt 2
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 3 0 0 10  1
3  1 4 0 0 10  2
4  1 6 0 0 10  3
5  3 2 1 0  1  2
6  3 2 9 0  1  5
''','''
# no soma 3 dendrites in a Y-shaped morphology, with lengths 10, 10, and 20
#n,type,x,y,z,radius,parent
1  3 0  0  0 1 -1
2  3 10  0 0 1  1
3  3 20  0 0 1  2
4  3 10 20 0 1  2
''','''
# previous but with soma
#n,type,x,y,z,radius,parent
1  1 -11  0 0 5 -1
2  1 -1  0  0 5  1
3  3 0  0   0 1  2
4  3 10  0  0 1  3
5  3 20  0  0 1  4
6  3 10 20  0 1  4
''','''
# 3 dendrites in a Y-shaped morphology, with lengths 10, 10, and 20
#n,type,x,y,z,radius,parent (no soma)
1  3 0  0  0 1 -1
2  3 10  0 0 1  1
3  3 11  0 0 1  2
4  3 20  0 0 1  3
5  3 10  1 0 1  2
6  3 10 20 0 1  5
''']

def mkswc(swc_contents):
  print ("\n\n\n")
  f = open("temp.tmp", "w")
  f.write(swc_contents)
  f.close()
  swc = h.Import3d_SWC_read()
  swc.input("temp.tmp")
  ig = h.Import3d_GUI(swc)
  ig.instantiate(None)

  print (swc_contents)
  h.topology()
  for sec in h.allsec():
    r = [h.ref(0) for _ in range(3)]
    style = ""
    if sec.pt3dstyle():
      h.pt3dstyle(1, r[0], r[1], r[2], sec=sec)
      style = [i[0] for i in r]
    print ("%s L=%g %s" % (str(sec), sec.L, str(style)))
    for i in range(sec.n3d()):
      print(" %d   %g %g %g %g" % (i, sec.x3d(i), sec.y3d(i), sec.z3d(i), sec.diam3d(i)))
  return ig

def cleanup():
  global ig
  for sec in h.allsec():
    h.delete_section(sec=sec)
  if ig:
    ig.box.unmap()
    ig = None

def test_swc():
  global ig
  for i, dat in enumerate(tst_data):
    ig = mkswc(dat)
    cleanup()

def show(tdat):
  global ig
  cleanup()
  ig = mkswc(tdat)
  
if __name__ == "__main__":
  ig = None
  h.xpanel("Choose an swc example")
  for i, dat in enumerate(tst_data):
    s = str(i) + dat.split('\n')[1]
    h.xradiobutton(s, (show, dat))
  h.xpanel()
    
