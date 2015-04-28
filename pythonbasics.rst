Python Basics
=============

This page demonstrates some basic Python concepts and essentials. See the `Python Tutorial <http://docs.python.org/tutorial/index.html>`_ and `Reference <http://docs.python.org/library/index.html>`_ for more exhaustive resources.

This page provides a brief introduction to:

* Python syntax
* Variables
* Lists and Dicts
* For loops and iterators
* Functions
* Classes
* Importing modules
* Writing and reading files with Pickling.

If you are running Linux or OSX, you probably already have Python installed. To run it, simply open a terminal and type ``python``. (Many users prefer to use `IPython <http://ipython.org/>`_ instead. That should work with all of our examples without requiring any modifications.) On Windows, you will probably have to install Python yourself. You could get it directly from `python.org <http://python.org>`_ or install versions -- e.g. `Anaconda <https://store.continuum.io/cshop/anaconda/>`_, `Enthought Canopy <https://www.enthought.com/products/canopy/>`_, `Python(x,y) <https://code.google.com/p/pythonxy/>`_, etc --  that package Python with many additional libraries.

.. note::

    If you are not using a distribution that packages Python with many additional modules, you will need to install at least numpy and matplotlib to run some of the examples in this tutorial series.
    
    On Ubuntu and Debian, you can get these modules via:
    
    .. code-block::
        none
        
        sudo apt-get install python-numpy python-matplotlib

    On all platforms, you can use the ``pip`` tool to automatically download and install new modules.
        
    .. seealso::
    
        `Installing Python Modules <https://docs.python.org/2.7/installing/>`_

The following command simply prints "Hello". Run it, and then re-evaluate with a different string.

.. code-block::
    python
    
    print "Hello"

Variables: Strings, numbers, and dynamic type casting
-----------------------------------------------------

Variables are easily assigned:

.. code-block::
    python
    
    my_name = "Tom"
    my_age = 45
       	
Let's work with these variables.

.. code-block::
    python
    
    print my_name
    print my_age 
       	

Strings can be combined with the + operator.

.. code-block::
    python
    
    greeting = "Hello, " + my_name
    print greeting 
       	
Let's move on to numbers.

.. code-block::
    python
    
    print my_age 

If you try using the + operator on my_name and my_age:

.. code-block::
    python
       	
    print my_name + my_age 

You will get a :class:`TypeError`. What is wrong?

my_name is a string and my_age is a number. Adding in this context does not make any sense.

We can determine an object's type with the :func:`type` function.

.. code-block::
    python

    print type(my_name), type(my_age) 
       	
The function :func:`isinstance` is also useful.

.. code-block::
    python
    
    print isinstance(my_name, str)

Python also has a special object called *None*. This is one way you can specify whether or not an object is valid. After evaluating the following script block, set my_valid_var to a value and rerun the four lines beginning with the if statement. The first time, it will complain that the variable is None; the second time it will print its value.

.. code-block::
    python
    
    my_valid_var = None
    if my_valid_var is not None:
        print my_valid_var
    else:
        print "The variable is None!" 
       	
Note the differences in the following lines.

.. code-block::
    python
    
    my_int = my_age/2
    my_float = my_age/2.0
    print my_int
    print my_float 
       	
In this case, on the top line, my_age is an int and 2 is an int, so the variable that gets assigned is also an int. In the second line, 2.0 is a float, so the result of the calculation remains a float. If you are following our example and used an odd value for my_age, you will notice that the two numbers are not equal in Python 2.7 as the division of two integers is assumed to be an integer (the fractional part is discarded). In Python 3.0+ by contrast, the division of two integers will produce a float if they do not divide evenly.

Lists
-----

Lists are comma-separated values surrounded by square brackets:

.. code-block::
    python
    
    my_list = [1, 3, 5, 8, 13]
    print my_list 
       	
Lists are zero-indexed. That is, the first element is 0.

.. code-block::
    python
    
    print my_list[0] 
       	
You may often find yourself wanting to know how many items are in a list.

.. code-block::
    python
    
    print len(my_list) 
       	
Python interprets negative indices as counting backwards from the end of the list. That is, the -1 index refers to the last item, the -2 index refers to the second-to-last item, etc.

.. code-block::
    python
    
    print my_list
    print my_list[-1] 
       	
"Slicing" is extracting particular sub-elements from the list in a particular range. However, notice that the right-side is excluded, and the left is included.

.. code-block::
    python
    
    print my_list
    print my_list[2:4] # Includes the range from index 2 to 3
    print my_list[2:-1] # Includes the range from index 2 to the element before -1
    print my_list[:2] # Includes everything before index 2
    print my_list[2:] # Includes everything from index 2 

To make a variable equal to a copy of a list, set it equal to ``list(the_old_list)``. For example:

.. code-block::
    python
    
    list_a = [1, 3, 5, 8, 13]
    list_b = list(list_a)
    list_b.reverse()
    print "list_a =", list_a
    print "list_b =", list_b 

Now replace the second line with ``list_b = list_a`` and rerun that code. In that case, ``list_b`` is the *same* list as ``list_a`` (as opposed to a copy), so when ``list_b`` was reversed so is ``list_a`` (since ``list_b`` *is* ``list_a``).

Lists can contain arbitrary data types, but if you find yourself doing this, you should probably consider making `classes <classes>`_ or `dictionaries <dictionaries>`_.

.. code-block::
    python
    
    confusing_list = ['abc', 1.0, 2, "another string"]
    print confusing_list
    print confusing_list[3] 
       	
range()
-------

:func:`range` is a function in Python that automatically generates a list of sequential integers. Note that the ending value is not included.

.. code-block::
    python
    
    print range(10)         # [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    print range(0, 10)      # [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    print range(3, 10)      # [3, 4, 5, 6, 7, 8, 9]
    print range(0, 10, 2)   # [0, 2, 4, 6, 8]
    print range(0, -10)     # []
    print range(0, -10, -1) # [0, -1, -2, -3, -4, -5, -6, -7, -8, -9]
    print range(0, -10, -2) # [0, -2, -4, -6, -8]
    
For non-integer ranges, use ``numpy.arange`` from the ``numpy`` module.
       	
For loops and iterators
-----------------------

We can iterate over elements in a list by following the format: "for element in list:" Notice that indentation is important in Python! After a colon, the block needs to be indented by 4 spaces. (Any consistent indentation will work, but the Python standard is 4).

.. code-block::
    python
    
    some_range = range(10)
    for elem in some_range:
        print "The value is", elem 
       	
Try substituting some of the previous lists that have been created instead of using some_range and re-evaluate the script block.

The `while loop <https://wiki.python.org/moin/WhileLoop>`_ is another type of loop that repeats until a condition is satisfied.

If you are ever stuck in a long loop (or any other Python code), try pressing :kbd:`Control-c` to break the loop by raising a :class:`KeyboardInterrupt` exception. Run the following code and stop it by pressing :kdb:`Control-c`:

.. code-block::
    python
    
    for i in xrange(100000000):
        pass


Here, ``pass`` means do nothing, and :func:`xrange` acts like :func:`range` in this example. The difference is that ``range`` constructs the list first (which requires memory and time) while ``xrange`` does not.

.. note::

    For Python 3+, replace :func:`xrange` with :func:`range`. (This is because ``range`` in Python 3 acts like ``xrange`` in Python 2).


Here we use
More advanced looping
~~~~~~~~~~~~~~~~~~~~~

Simulations across time mean that we deal with a lot of time-series data -- timestamps and their corresponding values. To simultaneously iterate over two lists of the same size, we can use :func:`zip`.

.. code-block::
    python
    
    y = ['a', 'b', 'c', 'd', 'e']
    x = range(len(y))
    print "x =", x
    print "y =", y
    print zip(x, y) 
       	
This is a list of tuples. Given a list of tuples, then we iterate with each tuple.

.. code-block::
    python
    
    for x_val, y_val in zip(x, y):
        print "idx", x_val, "=", y_val 
       	
Tuples are similar to lists, except they are immutable (cannot be changed). You can retrieve individual elements of a tuple, but once they are set upon creation, you cannot change them. Also, you cannot add or remove elements of a tuple.

.. code-block::
    python
    
    my_tuple = (1, 'two', 3)
    print my_tuple
    print my_tuple[1] 

Attempting to modify a tuple, e.g.

.. code-block::
    python
 	
    my_tuple[1] = 2

will cause a :class:`TypeError`.
       	
Because you cannot modify an element in a tuple, or add or remove individual elements of it, it can operate in python more efficiently than a list. A tuple can even serve as a key to a dictionary.

.. _dictionaries:

Dictionaries
------------

A dictionary (also called a dict or hash table) is a set of (key, value) pairs, denoted by curly brackets:

.. code-block::
    python
    
    about_me = {'name' : my_name, 'age' : my_age, 'height' : "5'8"}
    print about_me 
       	
You can obtain values by referencing the key:

.. code-block::
    python
    
    print about_me['height'] 
       	
Similarly, we can modify existing values by referencing the key.

.. code-block::
    python
    
    about_me['name'] = "Thomas"
    print about_me 
       	
We can even add new values.

.. code-block::
    python
    
    about_me['eye_color'] = "brown"
    print about_me 
       	
We can iterate keys, values or key-value value pairs in the dict. Here is an example of key-value pairs.

.. code-block::
    python
    
    for k, v in about_me.iteritems():
        print "key =", k, "  val =", v 
       	
To test for the presence of a key in a dict, we just ask:

.. code-block::
    python
    
    if 'hair_color' in about_me:
        print "Yes. 'hair_color' is a key in the dict"
    else:
        print "No. 'hair_color' is NOT a key in the dict" 
       	
Functions
---------

Functions are defined with a "def" keyword in front of them, end with a colon, and the next line is indented. Indentation of 4-spaces (again, any non-zero consistent amount will do) demarcates functional blocks.

.. code-block::
    python
    
    def print_hello():
        print "Hello" 
       	
Now let's call our function.

.. code-block::
    python
    
    print_hello() 
       	
We can also pass in an argument.

.. code-block::
    python
    
    def my_print(the_arg):
        print the_arg 
       	
Now try passing various things to the my_print() function.

.. code-block::
    python
    
    my_print("Hello") 
       	
We can even make default arguments.

.. code-block::
    python
    
    def my_print(the_arg="Hello"):
        print the_arg 
           	
    my_print()
    my_print(range(4)) 
       	
And we can also return values.

.. code-block::
    python
    
    def fib(n=5):
        """Get a Fibonacci series up to n."""
        a, b = 0, 1
        series = [a]
        while b < n:
            a, b = b, a + b
            series.append(a)
        return series 
       	
    print fib() 

Note the assignment line for a and b inside the while loop. That line says that a becomes the old value of b and that b becomes the old value of a plus the old value of b. The ability to calculate multiple values before assigning them allows Python to do things like swapping the values of two variables in one line while many other programming languages would require the introduction of a temporary variable.
   	
You may have noticed the triple-quoted strings. This enables a string to span multiple lines.

.. code-block::
    python
    
    multi_line_str = """This is the first line
    This is the second,
    and a third."""

    print multi_line_str 
       	
The importance of docstrings.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Docstrings are strings just under a function definition, and usually triple-quoted. At the very least, when they exist, they can be used to create beautiful documentation and they can also be available for help in real time. Better yet, they can provide clear examples of using the function and also be used in testing.

.. code-block::
    python
    
    help(fib) 

.. _classes

Classes
-------

Objects are instances of a "class". They are useful for encapsulating ideas, and mostly for having multiple instances of a structure. Usually you will have an ``__init__()`` method. Also note that every method of the class will have "self" as the first argument. While "self" has to be listed in the argument list of a class's method, you do not pass a "self" argument when calling any of the class's methods; instead, you refer to those methods as ``self.method_name``.

.. code-block::
    python
    
    class Contact(object):
        """A given person for my database of friends."""
        
        def __init__(self, first_name=None, last_name=None, email=None, phone=None):
            self.first_name = first_name
            self.last_name = last_name
            self.email = email
            self.phone = phone
        
        def print_info(self):
            """Print all of the information of this contact."""
            my_str = "Contact info:"
            if self.first_name:
                my_str += " " + self.first_name
            if self.last_name:
                my_str += " " + self.last_name
            if self.email:
                my_str += " " + self.email
            if self.phone:
                my_str += " " + self.phone     
            print my_str 
       	
By convention, the first letter of a class name is capitalized.
Notice in the class definition above that the object can contain fields, which are used within the class as "self.field". This field can be another method in the class, or another object of another class.

Let's make a couple instances of Contact.

.. code-block::
    python
    
    bob = Contact('Bob','Smith')
    joe = Contact(email='someone@somewhere.com') 
       	
Notice that in the first case, if we are filling each argument, we do not need to explicitly denote "first_name" and "last_name". However, in the second case, since "first" and "last" are omitted, the first parameter passed in would be assigned to the first_name field so we have to explicitly set it to "email".

Let's set a field.

.. code-block::
    python
    
    joe.first_name = "Joe" 
       	
Similarly, we can retrieve fields from the object.

.. code-block::
    python
    
    the_name = joe.first_name
    print the_name 
       	
And we call methods of the object using the format instance.method().

.. code-block::
    python
    
    joe.print_info() 
       	
Remember the importance of docstrings!

.. code-block::
    python
    
    help(Contact)
    
Importing modules
-----------------

Extensions to core Python are made by importing modules, which may contain more variables, objects, methods, and functions. Many modules come with Python, but are not part of its core. Other packages and modules have to be installed.

We previously used :func:`zip` to simultaneously iterate over two lists of the same length. Merging the values into a list of tuples, however, is computational overhead. More streamlined iterators are available in the :mod:`itertools` module. In particular, a faster way of simultaneously iterating two lists is to use :func:`izip` from the :mod:`itertools` module.

.. code-block::
    python
    
    from itertools import izip
    y = ['a', 'b', 'c', 'd', 'e']
    x = range(len(y))
    for (x_val, y_val) in izip(x, y):
        print "idx", x_val, "=", y_val 
       	
As another example, the numpy package contains a function called arange() that is similar to Python's range() function, but permits non-integer steps.

.. code-block::
    python
    
    import numpy
    my_vec = numpy.arange(0, 1, 0.1)
    print my_vec 
       	
.. note::

    The itertools module is a standard part of Python but numpy is a separate module. If the ``import numpy`` line gave an error message, you either do not have numpy installed or Python cannot find it for some reason. You should resolve this issue before proceeding because we will use numpy in some of the examples in other parts of the tutorial. The standard tool for installing Python modules is called ``pip``; other options may be available depending on your platform.

Pickling objects
----------------

There are various file io operations in Python, but one of the easiest is ":mod:`Pickling <pickle>`", which attempts to save a Python object to a file for later restoration with the load command.

.. code-block::
    python
    
    import pickle
    contacts = [joe, bob] # Make a list of contacts

    with open('contacts.p', 'w') as pickle_file: # Make a new file
        pickle.dump(contacts, pickle_file)       # Write contact list

    with open('contacts.p', 'r') as pickle_file: # Open the file for reading
        contacts2 = pickle.load(pickle_file)     # Load the pickled contents
        
    for elem in contacts2:
        elem.print_info() 

.. note::

    Many NEURON objects *cannot* be pickled. However, the data *values* can often be pickled and restored.

The next part of this tutorial introduces basic NEURON commands.

