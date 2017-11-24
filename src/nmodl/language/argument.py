class Argument:
    """Utility class for holding all arguments for node classes

    when force_prefix / force_suffix is true then prefix/suffix needs to be
    printed even if node itself is null
    """

    def __init__(self):
        self.base_class = ""
        self.class_name = ""
        self.nmodl_name = ""
        self.prefix = ""
        self.suffix = ""
        self.separator = ""
        self.typename = ""
        self.varname = ""
        self.is_vector = False
        self.is_optional = False
        self.add_method = False
        self.getname_method = False
        self.has_token = False
        self.force_prefix = False
        self.force_suffix = False