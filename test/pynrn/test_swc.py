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
# dendrites at ends and in interior of multisection soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 5 -1
2  1 10 0 0 3 1
3  1 15 0 0 4 2
4  1 20 0 0 2 3
5  1 -8 -6 0 3 1
6  1 -8 6 0 4 1
7  1 -16 12 0 2 6
8  3 30 0 0 1 4
9  3 -16 -12 0 1 5
10  3 -24 18 0 1 7
11 3 0 5 0 1 1
12 3 0 15 0 1 11
13 3 10 3 0 1 2
14 3 10 15 0 1 13
15 3 15 4 0 1 3
16 3 15 15 0 1 15
17 3 -8 10 0 1 6
18 3 -8 15 0 1 17
''','''
# branched soma, dendrites at ends and in interior of multisection soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 5 -1
2  1 10 0 0 3 1
3  1 15 0 0 4 2
4  1 20 0 0 2 3

5  1 -8 -6 0 3 1

6  1 -8 6 0 3 1
7  1 -12 9 0 4 6
8  1 -16 12 0 3 7

9  1 -20 15 0 4 8
10 1 -24 18 0 5 9
11 1 -28 21 0 2 10

12 1 -20 9 0 4 8
13 1 -24 6 0 2 12

14  3 30 -5 0 1 4

15  3 -12 -12 0 1 5

16  3 -36 24 0 1 11

17  3 -28 0 0 1 13

18 3 0 5 0 1 1
19 3 0 15 0 1 18

20 3 10 3 0 1 2
21 3 7 15 0 1 20

22 3 15 4 0 1 3
23 3 18 15 0 1 22

24 3 -6 10 0 1 6
25 3 -6 15 0 1 24

26 3 20 3 0 1 4
27 3 20 13 0 1 26

28 3 -11 19 0 1 7

29 3 -17 15 0 1 8
30 3 -18 25 0 1 29

31 3 -20 19 0 1 9
32 3 -20 29 0 1 31

33 3 -30 31 0 1 11
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
  ig.box.unmap()
  h.doNotify()
  ig.box.map("Import3d", 650, 200, -1, -1)
  h.doNotify()
  ig.instantiate(None)

  print (swc_contents)
  h.topology()
  for sec in h.allsec():
    r = [h.ref(0) for _ in range(3)]
    pseg = sec.parentseg()
    px = ("parent %s(%g)"%(pseg.sec.name(),pseg.x)) if pseg != None else ""
    style = ""
    if sec.pt3dstyle():
      h.pt3dstyle(1, r[0], r[1], r[2], sec=sec)
      style = [i[0] for i in r]
    print ("%s L=%g %s %s" % (str(sec), sec.L, str(style), px))
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


shapebox=None
def show_nrnshape():
  global shapebox
  h.load_file("shapebox.hoc")
  if shapebox:
    shapebox.unmap()
    shapebox = None
  shapebox = h.VBox()
  shapebox.intercept(1)
  h.makeMenuExplore()
  shapebox.intercept(0)
  shapebox.map("NEURON ShapeName", 100, 500, -1, -1)

def show(tdat):
  global ig
  cleanup()
  ig = mkswc(tdat)
  show_nrnshape()
  
if __name__ == "__main__":
  ig = None
  h.xpanel("Choose an swc example")
  for i, dat in enumerate(tst_data):
    s = str(i) + dat.split('\n')[1]
    h.xradiobutton(s, (show, dat))
  h.xpanel(80, 200)
    
