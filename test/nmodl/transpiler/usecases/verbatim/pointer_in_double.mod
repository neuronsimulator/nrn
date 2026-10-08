NEURON {
    SUFFIX pointer_in_double
    THREADSAFE
}

ASSIGNED {
  ptr
}

VERBATIM
struct SomeDouble {
  SomeDouble(double _value) : _value(_value) {}

  double value() const {
    return _value;
  }

  double _value;
};

static std::vector<SomeDouble> some_doubles;
ENDVERBATIM

INITIAL {
VERBATIM
  some_doubles.reserve(10);
  some_doubles.push_back(SomeDouble(double(some_doubles.size())));
  *reinterpret_cast<SomeDouble**>(&ptr) = &some_doubles.back();
ENDVERBATIM
}

FUNCTION use_pointer() {
VERBATIM
  return (*reinterpret_cast<SomeDouble**>(&ptr))->value();
ENDVERBATIM
}
