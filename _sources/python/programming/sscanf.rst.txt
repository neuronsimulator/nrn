.. _sscanf_doc:

String Parsing (sscanf)
-----------------------



.. function:: sscanf


    Syntax:
        ``num = h.sscanf("string", "format", args)``


    Description:
        A subset of the c stdio sscanf function. 
        The formats understood are %s, %[...], %c, %lf, %f, %d, %i, %o, %x, along with 
        numbers and the * character. Explicit strings in the format are also understood. 
        A great deal of error checking is done to make sure the args match the 
        format string. The function returns the actual number of args that are 
        assigned values. The arguments must be ``h.ref`` objects.
         
        Note that %lf is always preferred to %f since the latter truncates to floating 
        point accuracy. Hence 

        .. code-block::
            python
            
            from neuron import h
            x = h.ref(0)
            h.sscanf('0.3', '%f', x)
            print(x[0])

        prints: ``0.300000011921``


    Example:

        .. code-block::
            python

            from neuron import h
            s = h.ref('')
            x = [h.ref(0) for i in range(20)]
            y = h.ref(0)

            count = h.sscanf("this is a test\n", "%s", s)
            print(s[0])  # this

            count = h.sscanf("this is a test\n", "%[^\n]", s) 
            print(s[0])  # this is a test

            count = h.sscanf("this is a test\n", "%*s%s", s) 
            print(s[0])  # is
             
            count = h.sscanf("this is a test\n", "%2s", s) 
            print(s[0])  # th
             
            count = h.sscanf("this is a test\n", "%10s", s) 
            print(s[0])  # this
             
            count = h.sscanf("this is a test\n", "%*3s%s", s) 
            print(s[0])  # s
             
            count = h.sscanf("this is a test\n", "%*s%s", s) 
            print(s[0])  # is

            count = h.sscanf("0xff 010 25", "%i%i%i", x[0], x[1], x[2]) 
            print(x[0][0], x[1][0], x[2][0]) # 255.0 8.0 25.0

            count = h.sscanf("50.75", "%d.%d", x[0], x[1])
            print(x[0][0], x[1][0]) # 50.0 75.0

            count = h.sscanf("30.987654321", "%f", y)
            print(y[0])  # 30.9876537323

            count = h.sscanf("30.987654321", "%lf", y)
            print(y[0])  # 30.987654321

            count = h.sscanf("A", "%c", y)
            print(y[0])  # 65.0 (ASCII code for the letter A)
            print(chr(int(y[0])))  # A

            count = h.sscanf("1 2 3 4 5 6 7 8 9 10", "%f%f%f%f%f%f%f%f%f%f%f", x[0], x[1], x[2],
                x[3], x[4], x[5], x[6], x[7], x[8], x[9], x[10], x[11], x[12])
            print('Should only have non-zero values for x indices 0 - 9')
            for i in range(13):
                print(i, x[i][0])

            # the following are invalid and therefore raise a RuntimeError exception
            h.sscanf('string', '%', s) # invalid format string
            h.sscanf('string', '%*', s) # invalid format string
            h.sscanf('string', '%c%c%c%c%c', x[0], x[1], x[2]) # error due to insufficient number of args
            h.sscanf('25', '%d', s) # last entry not a pointer to a string             
             

.. note::

    The Python standard library does not provide a direct equivalent for ``h.sscanf``, but consider using the Regular Expressions module ``re`` instead, which can also be used for string parsing, albeit with a different specification.
