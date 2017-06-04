import math
import numpy
import weakref
import functools
import sys
from .rxdException import RxDException
from . import initializer

def _vectorized(f, objs):
    if hasattr(objs, '__len__'):
        return numpy.array([f(obj) for obj in objs])
    else:
        return f(objs)
    

def _vectorized2(f, objs1, objs2):
    if hasattr(objs1, '__len__'):
        return numpy.array([f(objA, objB) for objA, objB in zip(objs1, objs2)])
    else:
        return f(objs1, objs2)

def _erf(objs):
    return _vectorized(math.erf, objs)

def _erfc(objs):
    return _vectorized(math.erfc, objs)


def _factorial(objs):
    return _vectorized(math.factorial, objs)

def _gamma(objs):
    return _vectorized(math.gamma, objs)

def _lgamma(objs):
    return _vectorized(math.lgamma, objs)

def _power(objs1, objs2):
    # TODO? assumes numpy arrays; won't work for lists
    return objs1 ** objs2

def _neg(objs):
    return -objs

def analyze_reaction(r):
    if not isinstance(r, _Reaction):
        print('%r is not a reaction' % r)
    else:
        print('%r is a reaction:' % r)
        print('   lhs: ', ', '.join('%s[%d]' % (sp, c) for sp, c in zip(list(r._lhs._items.keys()), list(r._lhs._items.values()))))
        print('   rhs: ', ', '.join('%s[%d]' % (sp, c) for sp, c in zip(list(r._rhs._items.keys()), list(r._rhs._items.values()))))
        print('   dir: ', r._dir)
        
# TODO: change this so that inputs are all automatically converted to numpy.array(s)
def _compile(arith):
    initializer._do_init()
    try:
        s = arith._semi_compile
        species_dict = {}
        arith._involved_species(species_dict)
    except AttributeError:
        species_dict = {}
        s = str(arith)
    all_names = ['numpy', 'rxdmath'] + list(species_dict.keys())
    command = 'lambda %s: %s ' % (', '.join(all_names), s)
    return (functools.partial(eval(command), numpy, sys.modules[__name__]), list(species_dict.values()))
    

def _ensure_arithmeticed(other):
    from . import species
    if isinstance(other, species._SpeciesMathable):
        other = _Arithmeticed(other)
    elif isinstance(other, _Reaction):
        raise RxDException('Cannot do arithmetic on a reaction')
    elif not isinstance(other, _Arithmeticed):
        other = _Arithmeticed(other, valid_reaction_term=False)
    return other

def _validate_reaction_terms(r1, r2):
    if not(r1._valid_reaction_term or r2._valid_reaction_term):
        raise RxDException('lhs=%r and rhs=%r not valid in a reaction' % (r1, r2))
    elif not r1._valid_reaction_term:
        raise RxDException('lhs=%r not valid in a reaction' % r1)
    elif not r2._valid_reaction_term:
        raise RxDException('rhs=%r not valid in a reaction' % r2)


class _Function:
    def __init__(self, obj, f, fname):
        self._obj = _ensure_arithmeticed(obj)
        self._f = f
        self._fname = fname
    def __repr__(self):
        return '%s(%r)' % (self._fname, self._obj)
    def _short_repr(self):
        try:
            return '%s(%s)' % (self._fname, self._obj._short_repr())
        except:
            return self.__repr__()
    @property
    def _semi_compile(self):
        return '%s(%s)' % (self._f, self._obj._semi_compile)
    def _involved_species(self, the_dict):
        self._obj._involved_species(the_dict)

class _Function2:
    def __init__(self, obj1, obj2, f, fname):
        self._obj1 = _ensure_arithmeticed(obj1)
        self._obj2 = _ensure_arithmeticed(obj2)
        self._f = f
        self._fname = fname
    def __repr__(self):
        return '%s(%r, %r)' % (self._fname, self._obj1, self._obj2)
    def _short_repr(self):
        try:
            return '%s(%s, %s)' % (self._fname, self._obj1._short_repr(), self._obj2._short_repr())
        except:
            return self.__repr__()
    @property
    def _semi_compile(self):
        return '%s(%s, %s)' % (self._f, self._obj1._semi_compile, self._obj2._semi_compile)
    def _involved_species(self, the_dict):
        self._obj1._involved_species(the_dict)
        self._obj2._involved_species(the_dict)


    
# wrappers for the functions in module math from python 2.7
def acos(obj):
    return _Arithmeticed(_Function(obj, 'numpy.arccos', 'acos'), valid_reaction_term=False)
def acosh(obj):
    return _Arithmeticed(_Function(obj, 'numpy.arccosh', 'acosh'), valid_reaction_term=False)
def asin(obj):
    return _Arithmeticed(_Function(obj, 'numpy.arcsin', 'asin'), valid_reaction_term=False)
def asinh(obj):
    return _Arithmeticed(_Function(obj, 'numpy.arcsinh', 'asinh'), valid_reaction_term=False)
def atan(obj):
    return _Arithmeticed(_Function(obj, 'numpy.arctan', 'atan'), valid_reaction_term=False)
def atan2(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.arctan2', 'atan2'), valid_reaction_term=False)
def ceil(obj):
    return _Arithmeticed(_Function(obj, 'numpy.ceil', 'ceil'), valid_reaction_term=False)
def copysign(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.copysign', 'copysign'), valid_reaction_term=False)
def cos(obj):
    return _Arithmeticed(_Function(obj, 'numpy.cos', 'cos'), valid_reaction_term=False)
def cosh(obj):
    return _Arithmeticed(_Function(obj, 'numpy.cosh', 'cosh'), valid_reaction_term=False)
def degrees(obj):
    return _Arithmeticed(_Function(obj, 'numpy.degrees', 'degrees'), valid_reaction_term=False)
def erf(obj):
    return _Arithmeticed(_Function(obj, 'rxdmath._erf', 'erf'), valid_reaction_term=False)
def erfc(obj):
    return _Arithmeticed(_Function(obj, 'rxdmath._erfc', 'erfc'), valid_reaction_term=False)
def exp(obj):
    return _Arithmeticed(_Function(obj, 'numpy.exp', 'exp'), valid_reaction_term=False)
def expm1(obj):
    return _Arithmeticed(_Function(obj, 'numpy.expm1', 'expm1'), valid_reaction_term=False)
def fabs(obj):
    return _Arithmeticed(_Function(obj, 'numpy.abs', 'fabs'), valid_reaction_term=False)
def factorial(obj):
    return _Arithmeticed(_Function(obj, 'rxdmath._factorial', 'factorial'), valid_reaction_term=False)
def floor(obj):
    return _Arithmeticed(_Function(obj, 'numpy.floor', 'floor'), valid_reaction_term=False)
def fmod(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.fmod', 'fmod'), valid_reaction_term=False)
def frexp(obj):
    raise RxDException('frexp not supported in this context')
def fsum(obj):
    raise RxDException('fsum not supported in this context')
def gamma(obj):
    return _Arithmeticed(_Function(obj, 'rxdmath._gamma', 'gamma'), valid_reaction_term=False)
def hypot(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.hypot', 'hypot'), valid_reaction_term=False)
def isinf(obj):
    raise RxDException('isinf not supported in this context')
def isnan(obj):
    raise RxDException('isnan not supported in this context')
def ldexp(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.ldexp', 'ldexp'), valid_reaction_term=False)
def lgamma(obj):
    return _Arithmeticed(_Function(obj, 'rxdmath.lgamma', 'lgamma'), valid_reaction_term=False)
def log(obj):
    return _Arithmeticed(_Function(obj, 'numpy.log', 'log'), valid_reaction_term=False)
def log10(obj):
    return _Arithmeticed(_Function(obj, 'numpy.log10', 'log10'), valid_reaction_term=False)
def log1p(obj):
    return _Arithmeticed(_Function(obj, 'numpy.log1p', 'log1p'), valid_reaction_term=False)
def modf(obj):
    raise RxDException('modf not supported in this context')
# this seems to be okay; just have to avoid using pow in any other context
def pow(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'rxdmath._power', 'pow'), valid_reaction_term=False)
def radians(obj1, obj2):
    return _Arithmeticed(_Function2(obj1, obj2, 'numpy.radians', 'radians'), valid_reaction_term=False)
def sin(obj):
    return _Arithmeticed(_Function(obj, 'numpy.sin', 'sin'), valid_reaction_term=False)
def sinh(obj):
    return _Arithmeticed(_Function(obj, 'numpy.sinh', 'sinh'), valid_reaction_term=False)
def sqrt(obj):
    return _Arithmeticed(_Function(obj, 'numpy.sqrt', 'sqrt'), valid_reaction_term=False)
def tan(obj):
    return _Arithmeticed(_Function(obj, 'numpy.tan', 'tan'), valid_reaction_term=False)
def tanh(obj):
    return _Arithmeticed(_Function(obj, 'numpy.tanh', 'tanh'), valid_reaction_term=False)
def trunc(obj):
    return _Arithmeticed(_Function(obj, 'numpy.trunc', 'trunc'), valid_reaction_term=False)


class _Product:
    def __init__(self, a, b):
        self._a = a
        self._b = b
    def __repr__(self):
        return '(%r)*(%r)' % (self._a, self._b)
    @property
    def _semi_compile(self):
        return '(%s)*(%s)' % (self._a._semi_compile, self._b._semi_compile)
    def _involved_species(self, the_dict):
        self._a._involved_species(the_dict)
        self._b._involved_species(the_dict)

class _Quotient:
    def __init__(self, a, b):
        self._a = a
        self._b = b
    def __repr__(self):
        return '(%r)/(%r)' % (self._a, self._b)
    @property
    def _semi_compile(self):
        return '(%s)/(%s)' % (self._a._semi_compile, self._b._semi_compile)
    def _involved_species(self, the_dict):
        self._a._involved_species(the_dict)
        self._b._involved_species(the_dict)


class _Reaction:
    def __init__(self, lhs, rhs, direction):
        self._lhs = lhs
        self._rhs = rhs
        self._dir = direction
    def __repr__(self):
        return '%s%s%s' % (self._lhs._short_repr(), self._dir, self._rhs._short_repr())
    def __bool__(self):
        return False


class _Arithmeticed:
    def __init__(self, item, valid_reaction_term=True):
        if isinstance(item, dict):
            self._items = dict(item)
        elif isinstance(item, _Reaction):
            raise RxDException('Cannot do arithmetic on a reaction')
        else:
            self._items = {item: 1}
        self._valid_reaction_term = valid_reaction_term
        self._compiled_form = None
    
    def _evaluate(self, location):
        if self._compiled_form is None:
            self._compiled_form = _compile(self)
        if len(location) != 3:
            raise RxDException('_evaluate needs a (region, section, normalized position) triple')
        region, sec, x = location
        concentrations = [numpy.array(sp()[region].nodes(sec)(x).concentration) for sp in self._compiled_form[1]]
        value = self._compiled_form[0](*concentrations)
        if len(value) != 1:
            # this could happen in 3D
            raise RxDException('found %d values; expected 1.' % len(value))
        return value[0]
    
    def _short_repr(self):
        from . import species
        items = []
        counts = []
        for item, count in zip(list(self._items.keys()), list(self._items.values())):
            if count:
                if isinstance(item, species._SpeciesMathable):
                    items.append(str(item))
                    counts.append(count)
                else:
                    items.append(repr(item))
                    counts.append(count)
        result = ''
        for i, c in zip(items, counts):
            try:
                short_i = '%s' % i._short_repr()
            except:
                short_i = '%s' % i
            if result and c > 0:
                result += '+'
            if c == -1:
                result += '-(%s)' % short_i
            elif c != 1:
                result += '%d*(%s)' % (c, short_i)
            elif c == 1:
                result += i
        if not result:
            result = '0'
        return result
                    
    def __repr__(self):
        from . import species
        items = []
        counts = []
        for item, count in zip(list(self._items.keys()), list(self._items.values())):
            if count:
                if isinstance(item, species._SpeciesMathable):
                    items.append(str(item))
                    counts.append(count)
                else:
                    items.append(repr(item))
                    counts.append(count)
        result = ''
        for i, c in zip(items, counts):
            if result and c > 0:
                result += '+'
            if c == -1:
                result += '-(%s)' % i
            elif c != 1:
                result += '%d*(%s)' % (c, i)
            elif c == 1:
                result += i
        if not result:
            result = '0'
        return result
    
    @property
    def _semi_compile(self):
        items = []
        counts = []
        items_append = items.append
        counts_append = counts.append
        for item, count in zip(list(self._items.keys()), list(self._items.values())):
            if count:
                try:
                    items_append(item._semi_compile)
                except AttributeError:
                    items_append('%s' % item)
                counts_append(count)
        result = ''
        for i, c in zip(items, counts):
            if result and c > 0:
                result += '+'
            if c == -1:
                result += '-(%s)' % i
            elif c != 1:
                result += '%d*(%s)' % (c, i)
            elif c == 1:
                result += i
        if not result:
            result = '0'
        return result

    def _involved_species(self, the_dict):        
        for item, count in zip(list(self._items.keys()), list(self._items.values())):
            if count:
                try:
                    item._involved_species(the_dict)
                except AttributeError:
                    pass

            
    def _do_mul(self, other):
        if isinstance(other, int):
            items = dict(self._items)
            for i in items:
                items[i] *= other
            return _Arithmeticed(items, self._valid_reaction_term)
        else:
            other = _ensure_arithmeticed(other)
            return _Arithmeticed(_Product(self, other), False)

    def __mul__(self, other):
        return self._do_mul(other)
    
    def __rmul__(self, other):
        return self._do_mul(other)
    
    def __abs__(self):
        return _Arithmeticed(_Function(self, 'numpy.abs', 'abs'), valid_reaction_term=False)

    def __pos__(self):
        return self

    def __neg__(self):
        return _Arithmeticed(_Function(self, 'rxdmath._neg', '-'), valid_reaction_term=False)    

    def __div__(self, other):
        other = _ensure_arithmeticed(other)
        return _Arithmeticed(_Quotient(self, other), False)
    
    def __rdiv__(self, other):
        other = _ensure_arithmeticed(other)
        return other / self

    def __truediv__(self, other):
        other = _ensure_arithmeticed(other)
        return _Arithmeticed(_Quotient(self, other), False)

    def __rtruediv__(self, other):
        other = _ensure_arithmeticed(other)
        return other / self

    def __pow__(self, other):
        return pow(self, other)
        
    def __ne__(self, other):
        other = _ensure_arithmeticed(other)
        _validate_reaction_terms(self, other)
        return _Reaction(self, other, '<>')
    
    def __gt__(self, other):
        other = _ensure_arithmeticed(other)
        _validate_reaction_terms(self, other)
        return _Reaction(self, other, '>')

    def __lt__(self, other):
        other = _ensure_arithmeticed(other)
        _validate_reaction_terms(self, other)
        return _Reaction(self, other, '<')
            
    
    def __add__(self, other):
        other = _ensure_arithmeticed(other)
        new_items = dict(self._items)
        for oitem in other._items:
            if oitem not in new_items:
                new_items[oitem] = other._items[oitem]
            else:
                new_items[oitem] += other._items[oitem]
        return _Arithmeticed(new_items, self._valid_reaction_term and other._valid_reaction_term)
    
    def __radd__(self, other):
        return self + other

    
    def __sub__(self, other):
        other = _ensure_arithmeticed(other)
        new_items = dict(self._items)
        for oitem in other._items:
            if oitem not in new_items:
                new_items[oitem] = -other._items[oitem]
            else:
                new_items[oitem] -= other._items[oitem]
        return _Arithmeticed(new_items, False)
    

    def __rsub__(self, other):
        other = _ensure_arithmeticed(other)
        return other.__sub__(self)
        

