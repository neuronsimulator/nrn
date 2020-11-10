COMMENT
    Example file to test INCLUDE keyword
    in NMODL. This file is included in
    cabpump.mod and takes care of initializing
    the dummy variable named "var"
ENDCOMMENT

FUNCTION var_init(var(mV)) (mV) {
    var = 1
}
