from neuron import h, gui

h.load_file("import3d.hoc")

tst_data = [
    """
# One-point soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  3 0 10 0 1 1
3  3 0 20 0 1 2
4  3 0 -10 0 2 1
5  3 0 -20 0 2 4
6  3 10 0 0 .5 1
7  3 10 5 0 1 6
""",
    """
# Multi-point soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 2 0 0 10  1
3  1 4 0 0 10  2
4  3 2 10 0   1  2
5  3 2 18 0  1  4
""",
    """
# dendrites at ends of multisection soma
#n,type,x,y,z,radius,parent
1  1 0 0 0 5 -1
2  1 10 0 0 3 1
3  1 -8 -6 0 3 1
4  1 -8 6 0 3 1
5  3 20 0 0 1 2
6  3 -16 -12 0 1 3
7  3 -16 12 0 1 4
""",
    """
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
""",
    """
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
""",
    """
#two section soma with dendrites at ends and off point 1
#n,type,x,y,z,radius,parent
1 1 0 0 0 5 -1
2 1 5 0 0 3 1
3 1 -5 0 0 3 1
4 3 10 0 0 1 2
5 3 -10 0 0 1 3
6 3 0 5 0 1 1
7 3 0 10 0 1 6
""",
    """
#three point uniform diameter soma with root in middle, treated as sphere
#n,type,x,y,z,radius,parent
1 1 0 0 0 5 -1
2 1 5 0 0 5 1
3 1 -5 0 0 5 1
4 3 5 0 0 1 1
5 3 10 0 0 1 4
6 3 -5 0 0 1 1
7 3 -10 0 0 1 6
8 3 0 5 0 1 1
9 3 0 10 0 1 8
""",
    """
# Multipoint soma but not geometrically realizable
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 2 0 0 10  1
3  1 4 0 0 10  2
4  3 2 1 0  1  2
5  3 2 9 0  1  4
""",
    """
# 4pt soma with dendrite off pt 2
#n,type,x,y,z,radius,parent
1  1 0 0 0 10 -1
2  1 3 0 0 10  1
3  1 4 0 0 10  2
4  1 6 0 0 10  3
5  3 2 1 0  1  2
6  3 2 9 0  1  5
""",
    """
# no soma 3 dendrites in a Y-shaped morphology, with lengths 10, 10, and 20
#n,type,x,y,z,radius,parent
1  3 0  0  0 1 -1
2  3 10  0 0 1  1
3  3 20  0 0 1  2
4  3 10 20 0 1  2
""",
    """
# previous but with soma
#n,type,x,y,z,radius,parent
1  1 -11  0 0 5 -1
2  1 -1  0  0 5  1
3  3 0  0   0 1  2
4  3 10  0  0 1  3
5  3 20  0  0 1  4
6  3 10 20  0 1  4
""",
    """
# 3 dendrites in a Y-shaped morphology, with lengths 10, 10, and 20
#n,type,x,y,z,radius,parent (no soma)
1  3 0  0  0 1 -1
2  3 10  0 0 1  1
3  3 11  0 0 1  2
4  3 20  0 0 1  3
5  3 10  1 0 1  2
6  3 10 20 0 1  5
""",
    """
# no soma 2 dendrites with root connection.
#n,type,x,y,z,radius,parent
1  3 0  0  0 3 -1
2  3 2  0 0 1  1
3  3 10  0 0 1  2
4  3 -2  0 0 1  1
5  3 -10  0 0 1  4
""",
    """
# two point soma (ie: single section), no dendrites
1 1 -11 0 0 5.0 -1
2 1  -1 0 0 5.0 1
""",
    """
# 1st test from https://github.com/neuronsimulator/nrn/issues/119
1 1 0.0 0.0 0.0 0.5 -1 
""",
    """
# 2nd test from https://github.com/neuronsimulator/nrn/issues/119
1 1 0.0 0.0 0.0 0.5 -1
2 1 0.0 -0.5 0.0 0.5 1
3 1 0.0 0.5 0.0 0.5 1
""",
]


def mkswc(swc_contents):
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

    print(swc_contents)
    h.topology()
    print(secinfo())
    print("\n\n\n")
    return ig


def secinfo():
    result = "\n"
    for sec in h.allsec():
        r = [h.ref(0) for _ in range(3)]
        pseg = sec.parentseg()
        px = ("parent %s(%g)" % (pseg.sec.name(), pseg.x)) if pseg is not None else ""
        style = ""
        if sec.pt3dstyle():
            h.pt3dstyle(1, r[0], r[1], r[2], sec=sec)
            style = [i[0] for i in r]
        result += "%s L=%g %s %s\n" % (str(sec), sec.L, str(style), px)
        for i in range(sec.n3d()):
            result += " %d   %g %g %g %g\n" % (
                i,
                sec.x3d(i),
                sec.y3d(i),
                sec.z3d(i),
                sec.diam3d(i),
            )
    return result


def cleanup():
    global ig
    for sec in h.allsec():
        h.delete_section(sec=sec)
    if ig:
        ig.box.unmap()
        ig = None


shapebox = None


def show_nrnshape():
    global shapebox
    h.load_file("shapebox.hoc")
    if shapebox:
        shapebox.unmap()
        shapebox = None
    shapebox = h.VBox()
    shapebox.intercept(True)
    h.makeMenuExplore()
    shapebox.intercept(False)
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
        s = str(i) + dat.split("\n")[1]
        h.xradiobutton(s, (show, dat))
    h.xpanel(80, 200)

tst_result = [
    """
soma[0] L=20  
 0   -10 0 0 20
 1   0 0 0 20
 2   10 0 0 20
dend[0] L=10 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   0 10 0 2
 1   0 20 0 2
dend[1] L=10 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   0 -10 0 4
 1   0 -20 0 4
dend[2] L=5 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   10 0 0 1
 1   10 5 0 2
""",
    """
soma[0] L=4  
 0   0 0 0 20
 1   2 0 0 20
 2   4 0 0 20
dend[0] L=8 [2.0, 0.0, 0.0] parent soma[0](0.5)
 0   2 10 0 2
 1   2 18 0 2
""",
    """
soma[0] L=10  
 0   0 0 0 10
 1   10 0 0 6
soma[1] L=10  parent soma[0](0)
 0   0 0 0 10
 1   -8 -6 0 6
soma[2] L=10  parent soma[0](0)
 0   0 0 0 10
 1   -8 6 0 6
dend[0] L=10  parent soma[0](1)
 0   10 0 0 2
 1   20 0 0 2
dend[1] L=10  parent soma[1](1)
 0   -8 -6 0 2
 1   -16 -12 0 2
dend[2] L=10  parent soma[2](1)
 0   -8 6 0 2
 1   -16 12 0 2
""",
    """
soma[0] L=20  
 0   0 0 0 10
 1   10 0 0 6
 2   15 0 0 8
 3   20 0 0 4
soma[1] L=10  parent soma[0](0)
 0   0 0 0 10
 1   -8 -6 0 6
soma[2] L=20  parent soma[0](0)
 0   0 0 0 10
 1   -8 6 0 8
 2   -16 12 0 4
dend[0] L=10  parent soma[0](1)
 0   20 0 0 2
 1   30 0 0 2
dend[1] L=10  parent soma[1](1)
 0   -8 -6 0 2
 1   -16 -12 0 2
dend[2] L=10  parent soma[2](1)
 0   -16 12 0 2
 1   -24 18 0 2
dend[3] L=10 [0.0, 0.0, 0.0] parent soma[0](0)
 0   0 5 0 2
 1   0 15 0 2
dend[4] L=12 [10.0, 0.0, 0.0] parent soma[0](0.5)
 0   10 3 0 2
 1   10 15 0 2
dend[5] L=11 [15.0, 0.0, 0.0] parent soma[0](0.5)
 0   15 4 0 2
 1   15 15 0 2
dend[6] L=5 [-8.0, 6.0, 0.0] parent soma[2](0.5)
 0   -8 10 0 2
 1   -8 15 0 2
""",
    """
soma[0] L=20  
 0   0 0 0 10
 1   10 0 0 6
 2   15 0 0 8
 3   20 0 0 4
soma[1] L=10  parent soma[0](0)
 0   0 0 0 10
 1   -8 -6 0 6
soma[2] L=20  parent soma[0](0)
 0   0 0 0 10
 1   -8 6 0 6
 2   -12 9 0 8
 3   -16 12 0 6
soma[3] L=15  parent soma[2](1)
 0   -16 12 0 6
 1   -20 15 0 8
 2   -24 18 0 10
 3   -28 21 0 4
soma[4] L=10  parent soma[2](1)
 0   -16 12 0 6
 1   -20 9 0 8
 2   -24 6 0 4
dend[0] L=11.1803  parent soma[0](1)
 0   20 0 0 2
 1   30 -5 0 2
dend[1] L=7.2111  parent soma[1](1)
 0   -8 -6 0 2
 1   -12 -12 0 2
dend[2] L=8.544  parent soma[3](1)
 0   -28 21 0 2
 1   -36 24 0 2
dend[3] L=7.2111  parent soma[4](1)
 0   -24 6 0 2
 1   -28 0 0 2
dend[4] L=10 [0.0, 0.0, 0.0] parent soma[0](0)
 0   0 5 0 2
 1   0 15 0 2
dend[5] L=12.3693 [10.0, 0.0, 0.0] parent soma[0](0.5)
 0   10 3 0 2
 1   7 15 0 2
dend[6] L=11.4018 [15.0, 0.0, 0.0] parent soma[0](0.5)
 0   15 4 0 2
 1   18 15 0 2
dend[7] L=5 [-8.0, 6.0, 0.0] parent soma[2](0.5)
 0   -6 10 0 2
 1   -6 15 0 2
dend[8] L=13  parent soma[0](1)
 0   20 0 0 2
 1   20 3 0 2
 2   20 13 0 2
dend[9] L=10.0499  parent soma[2](0.5)
 0   -12 9 0 2
 1   -11 19 0 2
dend[10] L=10.0499 [-16.0, 12.0, 0.0] parent soma[2](1)
 0   -17 15 0 2
 1   -18 25 0 2
dend[11] L=10 [-20.0, 15.0, 0.0] parent soma[3](0.5)
 0   -20 19 0 2
 1   -20 29 0 2
dend[12] L=10.198  parent soma[3](1)
 0   -28 21 0 2
 1   -30 31 0 2
""",
    """
soma[0] L=5  
 0   0 0 0 10
 1   5 0 0 6
soma[1] L=5  parent soma[0](0)
 0   0 0 0 10
 1   -5 0 0 6
dend[0] L=5  parent soma[0](1)
 0   5 0 0 2
 1   10 0 0 2
dend[1] L=5  parent soma[1](1)
 0   -5 0 0 2
 1   -10 0 0 2
dend[2] L=5 [0.0, 0.0, 0.0] parent soma[0](0)
 0   0 5 0 2
 1   0 10 0 2
""",
    """
soma[0] L=10  
 0   -5 0 0 10
 1   0 0 0 10
 2   5 0 0 10
dend[0] L=5 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   5 0 0 2
 1   10 0 0 2
dend[1] L=5 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   -5 0 0 2
 1   -10 0 0 2
dend[2] L=5 [0.0, 0.0, 0.0] parent soma[0](0.5)
 0   0 5 0 2
 1   0 10 0 2
""",
    """
soma[0] L=4  
 0   0 0 0 20
 1   2 0 0 20
 2   4 0 0 20
dend[0] L=8 [2.0, 0.0, 0.0] parent soma[0](0.5)
 0   2 1 0 2
 1   2 9 0 2
""",
    """
soma[0] L=6  
 0   0 0 0 20
 1   3 0 0 20
 2   4 0 0 20
 3   6 0 0 20
dend[0] L=8 [3.0, 0.0, 0.0] parent soma[0](0.5)
 0   2 1 0 2
 1   2 9 0 2
""",
    """
dend[0] L=10  
 0   0 0 0 2
 1   10 0 0 2
dend[1] L=10  parent dend[0](1)
 0   10 0 0 2
 1   20 0 0 2
dend[2] L=20  parent dend[0](1)
 0   10 0 0 2
 1   10 20 0 2
""",
    """
soma[0] L=10  
 0   -11 0 0 10
 1   -1 0 0 10
dend[0] L=11  parent soma[0](1)
 0   -1 0 0 2
 1   0 0 0 2
 2   10 0 0 2
dend[1] L=10  parent dend[0](1)
 0   10 0 0 2
 1   20 0 0 2
dend[2] L=20  parent dend[0](1)
 0   10 0 0 2
 1   10 20 0 2
""",
    """
dend[0] L=10  
 0   0 0 0 2
 1   10 0 0 2
dend[1] L=10  parent dend[0](1)
 0   10 0 0 2
 1   11 0 0 2
 2   20 0 0 2
dend[2] L=20  parent dend[0](1)
 0   10 0 0 2
 1   10 1 0 2
 2   10 20 0 2
""",
    """
dend[0] L=10  
 0   0 0 0 6
 1   2 0 0 2
 2   10 0 0 2
dend[1] L=10  parent dend[0](0)
 0   0 0 0 6
 1   -2 0 0 2
 2   -10 0 0 2
""",
    """
soma[0] L=10  
 0   -11 0 0 10
 1   -1 0 0 10
""",
    """
soma[0] L=1  
 0   -0.5 0 0 1
 1   0 0 0 1
 2   0.5 0 0 1
""",
    """
soma[0] L=1  
 0   -0.5 0 0 1
 1   0 0 0 1
 2   0.5 0 0 1
""",
]


def test_swc():
    global ig
    ig = None
    cleanup()
    for i, dat in enumerate(tst_data):
        print("test %d" % i)
        ig = mkswc(dat)
        assert secinfo() == tst_result[i]
        cleanup()


if __name__ == "__main__":
    test_swc()
