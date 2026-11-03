: Tests for exploring setdata
NEURON {
    RANGE a, b, c
    POINTER p
}

PARAMETER {
  a = 0
  b = 1
  c[3]
}

ASSIGNED { p }

INITIAL {
  a = 1
  b = 2
  c[1] = 3
}

PROCEDURE AA() { : prior to A to get some extra lines of coverage
  A(1,2,3)
}

PROCEDURE A(x,y,z) {
  a = x
  b = y
  c[1] = z
}

PROCEDURE Aexp() {
  printf("exp(a) is %g\n", exp(a))
}

PROCEDURE Aexp2(x) {
  LOCAL a
  a = x
  printf("exp(%g) is %g\n", a, exp(a))
}

FUNCTION Aexcept(x) { : can test exception
VERBATIM
  hoc_execerr_ext("exception in Aexcept, x=%g", _lx);
ENDVERBATIM
  Aexcept = x
}

FUNCTION C(i) {
  C = c[i]
}

PROCEDURE d(x) {
  e(x)
}

PROCEDURE e(x) {
  a = x
}

FUNCTION f(x) {
  if (x > 0) {
    f = g(x)
  }else{
    f = 0
  }
}

PROCEDURE g(x) {
  b = b + f(x-1)
}

FUNCTION h(x) {
VERBATIM
  _lh =  _lx*_lx;
ENDVERBATIM
}

FUNCTION k(x) {
  if (x > 0) {
    k = k(x-1) + x
  } else {
    k = 0
  }
}

PROCEDURE P() {
  a = p
}

FUNCTION foo(x) {
  foo = ft(x)
}

FUNCTION bar() {
  bar = ft(a)
}

FUNCTION_TABLE ft(arg)
