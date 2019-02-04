# Complete list of tokens tests
## Legend

The following symbols will be used in the document:

✅ = Token is already present in an indipendent and simple unit test\
❌ = Token not present in any form in any of the tests\
❓ = Unclear if token is present or not\
✓ = Token present in tests but not in a standalone statement, e.g. in another, possibly more complex token test

| Token | Test Situation |
| ----- | -------------- |
|END | ❓ |
|MODEL | Deprecated? |
|CONSTANT | ✅ |
|INDEPENDENT | ✅ |
|DEPENDENT (ASSIGNED) | ✅ |
|STATE | ✅ |
|INITIAL1 | ✅ |
|DERIVATIVE | ✅ |
|SOLVE | ✅ | 
|USING | ✅ |
|WITH | ✓ |
|STEPPED | ✅ |
|DISCRETE | ✅ |
|FROM | ✅ |
|FORALL1 | ✅ |
|TO | ✓ |
|BY | ✓ |
|WHILE | ✅ |
|IF | ✅ |
|ELSE | ✅ |
|START1 | ✓ |
|STEP | ✓ |
|SENS | ✅ |
|SOLVEFOR | ✓ |
|PROCEDURE | ✅ |
|PARTIAL | ✅ |
|DEFINE1 | ✅ |
|IFERROR | ✅ |
|PARAMETER | ✅ |
|DERFUNC | ❓ | 
|EQUATION | ✅ |
|TERMINAL | ✅ |
|LINEAR | ✅ |
|NONLINEAR | ✅ |
|FUNCTION1 | ✅ |
|LOCAL | ✅ |
|LIN1 (~) | ✅ |
|NONLIN1 (~+) | ❌ | 
|PUTQ | ✅ |
|GETQ | ✅ |
|TABLE | ✅ |
|DEPEND | ✓ |
|BREAKPOINT | ✅ |
|INCLUDE1 | ✅ |
|FUNCTION_TABLE | ✅ |
|PROTECT | ✅ |
|NRNMUTEXLOCK | ✅ |
|NRNMUTEXUNLOCK | ✅ |
|OR | ❌ | 
|AND | ✓ |
|GT | ✅ |
|LT | ✅ |
|LE | ❌ |
|EQ | ✓ |
|NE | ❌ |
|NOT | ❌ |
|GE | ❌ |
|PLOT | ✅ |
|VS | ✅ |
|LAG | ✅ |
|RESET | ✅ |
|MATCH | ✅ |
|MODEL_LEVEL | Deprecated? |
|SWEEP | ✓ |
|FIRST | ✓ |
|LAST | ✓ |
|KINETIC | ✅ |
|CONSERVE | ✅ |
|REACTION | ✅ |
|REACT1 | ✅ |
|COMPARTMENT | ✅ |
|UNITS | ✅ |
|UNITSON | ✅ |
|UNITSOFF | ✅ |
|LONGDIFUS | ✅ |
|NEURON | ✅ |
|NONSPECIFIC | ✓ |
|READ | ✓ |
|WRITE | ✓ |
|USEION | ✓ |
|THREADSAFE | ✅ |
|GLOBAL | ✓ |
|SECTION | ❌ |
|RANGE | ✓ |
|POINTER | ✓ |
|BBCOREPOINTER | ✓ |
|EXTERNAL | ✓ |
|BEFORE | ✅ |
|AFTER | ✅ |
|WATCH | ✅ |
|ELECTRODE_CURRENT | ✓ |
|CONSTRUCTOR | ✓ |
|DESTRUCTOR | ✓ |
|NETRECEIVE | ✅ |
|FOR_NETCONS | ✅ |
|CONDUCTANCE | ✅ |
|REAL | ❓ |
|INTEGER | ❓ |
|DEFINEDVAR | ❓ |
|NAME | ❓ |
|METHOD | ✅ |
|SUFFIX | ✓ |
|VALENCE | ✓ |
|DEL | ✓ |
|DEL2 | ❌ |
|PRIME | ❓ |
|VERBATIM | ✅ |
|BLOCK_COMMENT | ✅ |
|LINE_COMMENT | ❓ |
|LINE_PART | ❓ |
|STRING | ❓ |
|OPEN_BRACE | ✅ |
|CLOSE_BRACE | ✅ |
|OPEN_PARENTHESIS | ✅ |
|CLOSE_PARENTHESIS | ✅ |
|OPEN_BRACKET | ✅ |
|CLOSE_BRACKET | ✅ |
|AT | ❓ |
|ADD | ✅ |
|MULTIPLY | ✅ |
|MINUS | ✅ |
|DIVIDE | ✅ |
|EQUAL | ✅ |
|CARET | ✓ |
|COLON | ❓ |
|COMMA | ✅ |
|TILDE | ✅ |
|PERIOD | ✅ |
|UNKNOWN | ❓ |
|INVALID_TOKEN | ❓ |
|UNARYMINUS | ❓ |