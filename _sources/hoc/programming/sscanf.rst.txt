
.. _hoc_sscanf_doc:

String Parsing (sscanf)
-----------------------



.. hoc:function:: sscanf


    Syntax:
        ``num = sscanf("string", "format", args)``


    Description:
        A subset of the c stdio sscanf function. 
        The formats understood are %s, %[...], %c, %lf, %f, %d, %i, %o, %x, along with 
        numbers and the * character. Explicit strings in the format are also understood. 
        A great deal of error checking is done to make sure the args match the 
        format string. The function returns the actual number of args that are 
        assigned values. 
         
        Note that %lf is always preferred to %f since the latter truncates to floating 
        point accuracy. Hence 

        .. code-block::
            none

            x=0 
            sscanf("0.3","%f",&x) 
            print x 
            prints the value 
            0.30000001 


    Example:

        .. code-block::
            none

            strdef s 
            double x[20] 
            y=0 
             
            sscanf("this is a test\n", "%s", s) 
            print s 
             
            sscanf("this is a test\n", "%[^\n]", s) 
            print s 
             
            sscanf("this is a test\n", "%*s%s", s) 
            print s 
             
            sscanf("this is a test\n", "%2s", s) 
            print s 
             
            sscanf("this is a test\n", "%10s", s) 
            print s 
             
            sscanf("this is a test\n", "%*3s%s", s) 
            print s 
             
            sscanf("this is a test\n", "%*s%s", s) 
            sscanf("0xff 010 25", "%i%i%i", &x[0], &x[1], &x[2]) 
            print x[0], x[1], x[2] 
             
            double x[20] 
            sscanf("ff 10 25", "%x%o%d", &x[0], &x[1], &x[2]) 
            print x[0], x[1], x[2] 
             
            sscanf("50.75", "%d.%d", &x[0], &x[1]) 
            print x[0], x[1] 
             
            sscanf("30.987654321", "%f", &y) 
            printf("%20.15g\n", y) 
             
            sscanf("30.987654321", "%lf", &y) 
            printf("%20.15g\n", y) 
             
            sscanf("A", "%c", &y) 
            printf("%c\n", y) 
             
            sscanf("1 2 3 4 5 6 7 8 9 10", "%f%f%f%f%f%f%f%f%f%f%f", &x[0], &x[1], &x[2],\ 
            	&x[3], &x[4], &x[5], &x[6], &x[7], &x[8], &x[9], &x[10], &x[11], &x[12]) 
            print "should only read ten of the following" 
            for i=0,12 print i, x[i] 
             
            print "error tests" 
             
            execute1("sscanf(\"string\", \"%\", s)") 
            execute1("sscanf(\"string\", \"%*\", s)") 
            execute1("sscanf(\"string\", \"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\", &x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x)") 
            execute1("sscanf(\"string\", \"%*5\", s)") 
             
            execute1("sscanf(\"string\", \"%l[st]\", s)") 
            execute1("sscanf(\"string\", \"%[\", s)") 
            execute1("sscanf(\"string\", \"%[]\", s)") 
            execute1("sscanf(\"string\", \"%[^]\", s)") 
            execute1("sscanf(\"string\", \"%ls\", s)") 
            execute1("sscanf(\"string\", \"%lc\", &y)") 
            execute1("sscanf(\"string\", \"%5c\", &y)") 
            execute1("sscanf(\"string\", \"%q\", &y)") 
             
            execute1("sscanf(\"string\", \"%s\")") 
             
            execute1("sscanf(\"string\", \"%s\", &y)") 
            execute1("sscanf(\"25\", \"%d\", s)") 
             
             
            execute1("sscanf(\"string\", \"%c%c%c%c%c%c%c%c%c%c%c%c%c\", &x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x,&x)") 
             
             



