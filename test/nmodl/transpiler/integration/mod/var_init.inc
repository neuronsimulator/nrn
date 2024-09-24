COMMENT
    Example file to test INCLUDE keyword
    in NMODL. This file is included in
    cabpump.mod
ENDCOMMENT

FUNCTION var_init(var(mV)) (mV) {
    : Sets the value of the local variable `var` to `1`.
    : Because the arguments are passed by value, this
    : function has no effect at all.
    var = 1
}
