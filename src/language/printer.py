from abc import ABCMeta, abstractmethod
import sys


class Writer(object):
    """ Utility class for writing to file

    Common methods for writing code are provided by this
    class. This handles opening/closing of file, indentation,
    newlines, buffering etc.
    """

    # tab is four whitespaces
    TAB = "    "

    def __init__(self, filepath):
        self.filepath = filepath
        self.num_tabs = 0
        self.lines = []
        try:
            self.fh = open(self.filepath, "w+")
        except IOError as e:
            print("Error :: I/O error while writing {0} : {1}".format(self.filepath, e.strerror))
            sys.exit(1)

    def print_gutter(self):
        for i in range(0, self.num_tabs):
            self.fh.write(self.TAB)

    def increase_gutter(self):
        self.num_tabs += 1

    def decrease_gutter(self):
        self.num_tabs -= 1

    def write(self, string):
        if string:
            self.print_gutter()
            self.fh.write(string)

    def write_line(self, string, newline, pre_gutter, post_gutter):
        self.num_tabs += pre_gutter
        self.write(string)
        for i in range(newline):
            self.fh.write("\n")
        self.num_tabs += post_gutter

    def add_line(self, string, newline, pre_gutter, post_gutter):
        self.num_tabs += pre_gutter
        if string:
            for i in range(self.num_tabs):
                string = self.TAB + string
            for i in range(newline):
                string = string + "\n"
            self.lines.append(string)
        self.num_tabs += post_gutter

    def num_buffered_lines(self):
        return len(self.lines)

    def flush_buffered_lines(self):
        for line in self.lines:
            self.fh.write(line)
        self.lines = []

    def __del__(self):
        if not self.fh.closed:
            self.fh.close()


class Printer(object):
    """ Base class for code printer classes"""
    __metaclass__ = ABCMeta

    def __init__(self, filepath, classname, nodes):
        self.writer = Writer(filepath)
        self.classname = classname
        self.nodes = nodes

    def write_line(self, string=None, newline=1, pre_gutter=0, post_gutter=0):
        self.writer.write_line(string, newline, pre_gutter, post_gutter)

    def add_line(self, string=None, newline=1, pre_gutter=0, post_gutter=0):
        self.writer.add_line(string, newline, pre_gutter, post_gutter)

    @abstractmethod
    def write(self):
        pass


class DeclarationPrinter(Printer):
    """ Utility class for writing declaration / header file (.hpp)

    Header file of the class typically consists of
        - headers include section
        - comment about the class
        - class name / class declaration
        - private member declarations
        - public members declaration
        - end of class declaration
        - post declaration, e.g. end of namespace
    """

    def guard(self):
        self.write_line("#pragma once", newline=2)

    @abstractmethod
    def headers(self):
        pass

    def class_comment(self):
        pass

    def class_name_declaration(self):
        self.write_line("class " + self.classname + " {")

    def declaration_start(self):
        self.guard()
        self.headers()
        self.write_line()
        self.class_comment()
        self.class_name_declaration()

    def private_declaration(self):
        pass

    def public_declaration(self):
        pass

    def body(self):
        self.writer.increase_gutter()
        self.write_line()
        self.private_declaration()
        self.public_declaration()
        self.writer.decrease_gutter()

    def declaration_end(self):
        self.write_line("};", pre_gutter=-1)

    def post_declaration(self):
        pass

    def write(self):
        self.declaration_start()
        self.body()
        self.declaration_end()
        self.post_declaration()


class DefinitionPrinter(Printer):
    """ Utility class for writing definition / implementation file (.cpp)

    Implementation file of the class typically consists of
        - headers include section (.hpp file)
        - class methods definition
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def headers(self):
        pass

    @abstractmethod
    def definitions(self):
        pass

    def write(self):
        self.headers()
        self.definitions()
