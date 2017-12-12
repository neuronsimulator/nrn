import re

# convert string of the form "AabcDef" to "Abc_Def"
def camel_case_to_underscore(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    typename = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return typename

# convert string of the form "AabcDef" to "ABC_DEF"
def node_ast_type(name):
	return camel_case_to_underscore(name).upper()

# convert string of the form "AabcDef" to "abc_def"
def node_property_type(name):
	return camel_case_to_underscore(name).lower()