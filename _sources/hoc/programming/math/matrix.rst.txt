
.. _hoc_matrix:

         
Matrix
------



.. hoc:class:: Matrix


    Syntax:
        ``mobj = new Matrix(nrow, ncol)``

        ``mobj = new Matrix(nrow, ncol, type)``


    Description:
        A class for manipulation of two dimensional arrays of numbers. A companion 
        to the :hoc:class:`Vector` class, Matrix contains routines for m*x=b, v1=m*v2, etc.
         
        Individual element values are assigned and evaluated 
        using the syntax: 

        .. code-block::
            none

            	m.x[irow][icol] 

        which may appear anywhere in an expression or on the left hand side of 
        an assignment statement. irow can range from 0 to m.nrow-1 and icol 
        ranges from 0 to m.ncol-1 . (See :hoc:data:`~Matrix.x`)
         
        When possible, Matrix methods returning a Matrix use the form, 
        mobj = m.f(args, [mout]), where mobj is a newly constructed matrix (m 
        is unchanged) unless 
        the optional last mout argument is present, in which case the return value 
        is mout and mout is used to store the matrix.  This style seems most efficient 
        to me since many matrix operations cannot be done in-situ. Exceptions to 
        this rule, eg m.zero(), are noted in the individual methods. 
         
        Similarly, Matrix methods returning a Vector use the form, 
        vobj = m.f(args, [vout]), where vobj is a newly constructed Vector unless 
        the optional last vout is present for storage of the vector elements. 
        Use of vout is extremely useful in those cases where the vector is graphed 
        since the result of the matrix operation does not invalidate the pointers 
        to the vector elements. 
         
        Note that the return value allows these operations to be used as members 
        of a filter chain or arguments to other functions. 
         
        By default, a new Matrix is of type MFULL (= 1) and allocates storage for 
        all nrow*ncol elements. Scaffolding is in place for matrices of storage 
        type MSPARSE (=2) and MBAND (=3) but not many methods have been interfaced 
        to the eigen library at this time. If a method is called on a matrix type 
        whose method has not been implemented, an error message will be printed. 
        It is intended that implemented methods will be transparent to the user, eg 
        m*x=b (``x = m.solv(b)`` ) will solve the linear system 
        regardless of the type of m and 
        v1 = m*v2 (``v1 = m.mulv(v2)`` ) will perform the vector multiplication. 
         
        Matrix is implemented using the `eigen3 library <https://eigen.tuxfamily.org>`_ 
        which contains a large collection of routines for sparse, banded, and full matrices. 
        Many of the useful routines  have not been interfaced with the hoc 
        interpreter but can be easily added on request or you can add it yourself 
        by analogy with the code in ``nrn/src/ivoc/(matrix.c ocmatrix.[ch])`` 
        At this time the MFULL matrix type is complete enough to do useful work 
        and MSPARSE can be used to multiply a matrix by a vector and solve 
        Mx=b. 

         

----



.. hoc:data:: Matrix.x


    Syntax:
        ``val = m.x[irow][icol]``

        ``m.x[irow][icol] = val``

        ``expr(m.x[irow][icol])``

        ``&m.x[irow][icol]``


    Description:
        Individual element values are assigned and evaluated 
        using the syntax: 

        .. code-block::
            none

            	m.x[irow][icol] 

        which may appear anywhere in an expression or on the left hand side of 
        an assignment statement. irow can range from 0 to m.nrow-1 and icol 
        ranges from 0 to m.ncol-1 . 
         
        For functions that require the address of a double value one may write 

        .. code-block::
            none

            	&m.x[irow][icol] 

        but one must be on guard for the case in which matrix storage is freed 
        while another object holds a pointer to one of its elements. (Matrix 
        does not currently notify the interpreter when storage has been freed.) 
         
        For sparse matrices an invocation of x[i][j] will create it if the 
        element does not exist. Therefore if you wish to access every element 
        use :hoc:meth:`Matrix.getval` to avoid creating a very inefficient full matrix!

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(3,4) 
            for i=0,m.nrow-1 { 
            	for j=0, m.ncol-1 { 
            		m.x[i][j] = 10*i + j 
            		print i, j, m.x[i][j] 
            	} 
            } 
            m.printf 
            xpanel("m") 
            xvalue("m(1,3) interpret", "m.x[1][3]", 1, "m.printf") 
            xpvalue("m(1,3) address", &m.x[1][3], 1, "m.printf") 
            xpanel() 


    .. warning::
        When dealing with sparse matrices, be careful when using the m.x[][] notation 
        since the mere act of evaluating a zero element will create it if it does not 
        exist. In this case it is better to use the :hoc:func:`getval` function.
         
        In Python, the m.x[i][j] syntax does not work and one must use the 
        :hoc:func:`setval` function


----



.. hoc:method:: Matrix.nrow


    Syntax:
        ``n = m.nrow``


    Description:
        returns the row dimension of the matrix. Row indices range from 0 to m.nrow-1 


----



.. hoc:method:: Matrix.ncol

        n = m.ncol 

    Description:
        returns the column dimension of the matrix. Column indices range 
        from 0 to m.ncol-1 

         

----



.. hoc:method:: Matrix.resize


    Syntax:
        ``mobj = msrcdest(nrow, ncol)``


    Description:
        Change the size of the matrix. As many as possible of the former elements 
        are preserved. New elements are assigned the value of 0. New memory may 
        not have to be allocated depending on the size history of the matrix. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(3,5) 
            m 
            for i=0,4 m.setcol(i,i) 
             
            m.printf 
            m.resize(7,7) 
            m.printf() 
            m.resize(4,2) 
            m.printf() 


    .. warning::
        Implemented only for full matrices. 

         

----



.. hoc:method:: Matrix.c


    Syntax:
        ``mdest = msrc.c()``


    Description:
        Copy the matrix. msrc is unchanged. 

    .. warning::
        Implemented only for full matrices. 

         

----



.. hoc:method:: Matrix.bcopy


    Syntax:
        ``mdest = msrc.bcopy(i0, j0, n, m [, mout])``

        ``mdest = msrc.bcopy(i0, j0, n, m, i1, j1 [, mout])``


    Description:
        Copy selected piece of a matrix. msrc is unchanged. 
        Copies the n x m submatrix with top-left (row i0, col j0) coordinates 
        to the corresponding submatrix of destination with top-left coordinates 
        (i1, j1). Out is resized if necessary. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,6) 
            for i=0,m.nrow-1 for j=0,m.ncol-1 m.x[i][j] = 1 + 10*i + j 
            m.printf 
            m.bcopy(1,2,2,3).printf 
            m.bcopy(1,2,2,3,2,3).printf 
            m.bcopy(1,2,2,3,2,3, new Matrix(8,8)).printf 


    .. warning::
        Implemented only for full matrices. 

         

----



.. hoc:method:: Matrix.getval


    Syntax:
        ``val = m.getval(irow, jcol)``


    Description:
        Returns the value of the matrix element. If m is sparse and the element 
        does not exist then 0 is returned without creating the element. 

         

----



.. hoc:method:: Matrix.setval


    Syntax:
        ``val = m.setval(irow, jcol, val)``


    Description:
        Sets the value of the matrix element. For sparse matrices, if the 
        element is 0, this method will create the element. This method was added 
        because m.x[irow][jcol] does not work in Python. 

         

----



.. hoc:method:: Matrix.sprowlen


    Syntax:
        ``n = m.sprowlen(i)``


    Description:
        Returns the number of existing(usually nonzero) 
        elements in the ith row of the sparse 
        matrix. Useful for iterating over a elements of a sparse matrix. 
        This function works only for sparse matrices. 
        See :hoc:meth:`Matrix.spgetrowval`

         

----



.. hoc:method:: Matrix.spgetrowval


    Syntax:
        ``x = m.spgetrowval(i, jx, &j)``


    Description:
        Returns the existing element value and the column index (third pointer arg) 
        of the ith row and jx item. The latter ranges from 0 to m.sprowlen(i)-1 
        This function works only for sparse matrices (created with a third argument 
        of 2) 

    Example:
        To print the elements of a sparse matrix. 

        .. code-block::
            none

            proc sparse_print() { local i, j, jx, x 
            	print $o1 
            	for i=0, $o1.nrow-1 { 
            		printf("%d  ", i) 
            		for jx = 0, $o1.sprowlen(i)-1 { 
            			x = $o1.spgetrowval(i, jx, &j) 
            			printf("  %d:%g", j, x) 
            		} 
            		printf ("\n") 
            	} 
            } 
             
            objref m 
            m = new Matrix(4, 5, 2) 
            m.x[0][2] = 1.2 
            m.x[0][4] = 2.4 
            m.x[1][1] = 3.1 
            for i=0, 4 { m.x[3][i] = i/10 } 
            sparse_print(m) 



----



.. hoc:method:: Matrix.printf


    Syntax:
        ``0 = m.printf``

        ``0 = m.printf("element_format")``

        ``0 = m.printf("element_format", "row_format")``


    Description:
        Print the matrix to the standard output with a default %-8g element format 
        and a default "\n" row format. 

    .. warning::
        Needs a separate implementation for sparse and banded matrices. Prints sparse 
        as though it was full. 


----



.. hoc:method:: Matrix.fprint


    Syntax:
        ``0 = m.fprint(fileobj)``

        ``0 = m.fprint(fileobj, "element_format")``

        ``0 = m.fprint(fileobj, "element_format", "row_format")``

        ``0 = m.fprint(0, fileobj [,...])``


    Description:
        Same as :hoc:func:`printf` but prints to the File object (must be open for writing)
        with a first line consisting of the two integers, nrow ncol. 
        Print the matrix to the open file object with a default %-8g element format 
        and a default "\n" row format. 
        Because of the "nrow ncol" first line, such a file can be read with :hoc:func:`scanf` .
        If the first arg is a 0, then the nrow ncol pair of numbers will not 
        be printed. 

    .. warning::
        Needs a separate implementation for sparse and banded matrices. 


----



.. hoc:method:: Matrix.scanf


    Syntax:
        ``0 = m.scanf(File_object)``

        ``0 = m.scanf(File_object, nrow, ncol)``


    Description:
        Read a file, including sizes, into a Matrix. The File_object is 
        an object of type :hoc:class:`File` and must be opened for reading prior to
        the scanf. If nrow,ncol arguments are not present, 
        the first two numbers in the file must be nrow and mcol 
        respectively. In either case those values are used to resize the matrix. 
        The following nrow*mcol 
        numbers are row streams, eg it is often natural to have one row on a single line 
        or else to organize the file as a list of row vectors with only one number 
        per line. Strings in the file that cannot be parsed as numbers are ignored. 
         

        .. code-block::
            none

            objref m, f 
            f = new File("filename") 
            f.ropen() 
            m = new Matrix() 
            m.scanf(f) 
            print m.nrow, m.ncol 


    .. warning::
        Works only for full matrix types 

    .. seealso::
        :hoc:meth:`Vector.scanf`, :hoc:func:`fscan`


----



.. hoc:method:: Matrix.mulv


    Syntax:
        ``vobj = msrc.mulv(vin)``

        ``vobj = msrc.mulv(vin, vout)``


    Description:
        Multiplication of a Matrix by a Vector, vobj = msrc*vin. 
        Returns a new vector of dimension msrc.nrow. Optional Vector 
        vout is used for storage of the result. Vector 
        vin must have dimension msrc.ncol. vin and vout can be the same vector 
        if the matrix is square. 

    Example:
        objref m, v1 
        v1 = new Vector(4) 
        v1.indgen(1,1) 
        m = new Matrix(3, 4) 
        for i=0,2 for j=0,2 m.x[i][j]=i*10 + j 

        .. code-block::
            none

            print "v1", v1 
            v1.printf 
            print "m", m 
            m.printf 
            print "m*v1" 
            m.mulv(v1).printf 

        A sparse example 

        .. code-block::
            none

            objref m, v1 
            v1 = new Vector(100) 
            v1.indgen(1,1) 
            m = new Matrix(100, 100, 2) // sparse matrix 
            // reverse permutation 
            for i=0, 99 { 
            	m.x[i][99 - i] = 1 
            } 
            m.mulv(v1).printf 


    .. warning::
        Implemented only for full and sparse matrices. 


----



.. hoc:method:: Matrix.getrow


    Syntax:
        ``vobj = msrc.getrow(i)``

        ``vobj = msrc.getrow(i, vout)``


    Description:
        Return the i'th row of the matrix in a new vector (or use the storage 
        in vout if that arg is present). Range of i is from 0 to msrc.nrow-1. 

    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.getcol


    Syntax:
        ``vobj = msrc.getcol(i)``

        ``vobj = msrc.getcol(i, vout)``


    Description:
        Return the i'th column of the matrix in a new vector (or use the storage 
        in vout if that arg is present). Range of i is from 0 to msrc.ncol-1. 

    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.getdiag


    Syntax:
        ``vobj = msrc.getdiag(i)``

        ``vobj = msrc.getdiag(i, vout)``


    Description:
        Return the i'th diag of the matrix in a new vector (or use the storage 
        in vout if that arg is present) of size msrc.nrow. 
        Range is from -(msrc.nrow-1) to msrc.ncol-1 
        with 0 being the main diagonal, positive i refers to upper diagonals, and 
        negative i refers to lower diagonals. Upper diagonals fill the Vector 
        starting at position 0 and remaining elements are unused. 
        Lower diagonals fill the Vector ending at msrc.nrow-1 and the first 
        elements are unused. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,5) 
            for i=0, m.nrow-1 for j=0, m.ncol-1 m.x[i][j] = 1 + 10*j + 100*i 
            m.printf 
            for i=-m.nrow+1, m.ncol-1 { 
            	printf("diagonal %d: ", i) 
            	m.getdiag(i).printf 
            } 


    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.solv


    Syntax:
        ``vx = msrc.solv(vb)``

        ``vx = msrc.solv(vb, vout and/or 1 in either order)``


    Description:
        Solves the linear system msrc*vx = vb by LU factorization. msrc must be 
        a square matrix and vb must have size equal to msrc.nrow. The answer 
        will be returned in a new Vector of size msrc.nrow. 
        msrc is not changed. 
        The LU factorization is stored in case it 
        is desired for later reuse with a different vb. Re-use of the LU factorization 
        will actually take place only if the second or third argument is 1 and 
        msrc has not changed in size. 
         
        Note: if the LUfactor is used, changes to the actual values of msrc would 
        not affect the solution on subsequent calls to solv. 
         

    Example:

        .. code-block::
            none

            objref m, b 
            b = new Vector(3) 
            b.indgen(1,1) 
            m = new Matrix(3, 3) 
            for i=0, m.nrow-1 for j=0, m.ncol-1 m.x[i][j] = i*j + 1 
            print "b" 
            b.printf 
            print "m" 
            m.printf 
            print "solution of m*x = b" 
            m.solv(b).printf 


        .. code-block::
            none

            objref m, b, x 
             
            m = new Matrix(1000, 1000, 2) // sparse type 
            m.setdiag(0, 3) 
            m.setdiag(-1, -1) 
            m.setdiag(1, -1) 
            b = new Vector(1000) 
            b.x[500] = 1 
            x = m.solv(b) 
            x.printf("%8.3f", 475, 525) 
             
            b.x[500] = 0 
            b.x[499] = 1 
            m.solv(b,1).printf("%8.3f", 475, 535) 


    .. warning::
        Implemented only for full and sparse matrices. 


----



.. hoc:method:: Matrix.det


    Syntax:
        ``mantissa = m.det(&base10exponent)``


    Description:
        Determinant of matrix m. Returns mantissa in range from -1 to 1 and 
        integer base10exponent. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(2,2) 
            m.x[0][1] = 20 
            m.x[1][0] = 30 
            m.printf() 
            ex = 0 
            mant = m.det(&ex) 
            print mant*10^ex 



----



.. hoc:method:: Matrix.mulm


    Syntax:
        ``mobj = msrc.mulm(m)``

        ``mobj = msrc.mulm(m, mout)``


    Description:
        Multiplication of a Matrix by a Matrix, mobj = msrc*m. msrc and m are 
        unchanged. A new matrix is returned with size msrc.nrow x m.ncol. 
        msrc.ncol and m.nrow must be the same. If mout is present, that storage is 
        used for the result. 

    Example:

        .. code-block::
            none

            objref m1, m2, v1 
            m1 = new Matrix(6, 6) 
            for i=-1,1 { 
            	if (i == 0) { 
            		m1.setdiag(i, 2) 
            	}else{ 
            		m1.setdiag(i, -1) 
            	} 
            } 
            m2 = m1.inverse() 
            print "m1" 
            m1.printf 
            print "m2" 
            m2.printf(" %8.5f") 
            print "m1*m2" 
            m1.mulm(m2).printf(" %8.5f") 


    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.add


    Syntax:
        ``mobj = m1srcdest.add(m2src)``


    Description:
        Return m1srcdest + m2src. The matrices must have the same rank. 
        This is one of those functions that modifies the source matrix (unless the 
        last optional mout arg is present) instead of 
        putting the result in a new destination matrix. 

    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.muls


    Syntax:
        ``mobj = msrcdest.muls(scalar)``


    Description:
        Multiply the matrix by a scalar in place and return the matrix reference. 
        This is one of those functions that modifies the source matrix instead of 
        putting the result in a new destination matrix. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,4) 
            m.ident() 
            m.muls(-10) 
            m.printf 


    .. warning::
        Implemented only for full and sparse matrices. 


----



.. hoc:method:: Matrix.setrow


    Syntax:
        ``mobj = msrcdest.setrow(i, vin)``

        ``mobj = msrcdest.setrow(i, scalar)``


    Description:
        Fill the ith row of the msrcdest matrix with the values of the Vector vin. 
        The vector must have size msrcdest.ncol 
         
        Otherwise fill the matrix row with a constant. 

    .. warning::
        Implemented only for full matrices and sparse. 


----



.. hoc:method:: Matrix.setcol


    Syntax:
        ``mobj = msrcdest.setcol(i, vin)``

        ``mobj = msrcdest.setcol(i, scalar)``


    Description:
        Fill the ith column of the msrcdest matrix with the values of the Vector vin. 
        The vector must have size msrcdest.mrow 
         
        Otherwise fill the matrix column with a constant. 

    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.setdiag


    Syntax:
        ``mobj = msrcdest.setdiag(i, vin)``

        ``mobj = msrcdest.setdiag(i, scalar)``


    Description:
        Fill the ith diagonal of the msrcdest matrix with the values of the 
        Vector vin. The vector must have size msrcdest.mrow. The ith diagonal 
        ranges from -(mrow-1) to mcol-1. For positive diagonals, the starting 
        position of vector elements is 0 and trailing elements are ignored. 
        For negative diagonals, the ending position of the vector elements is 
        nrow-1 and beginning elements are ignored. 
         
        Otherwise fill the matrix diagonal with a constant. 

    Example:

        .. code-block::
            none

            objref v1, m 
            m = new Matrix(5,7) 
            v1 = new Vector(5) 
            for i=-4,6 { 
            	m.setdiag(i, i) 
            } 
            m.printf 
            for i=-4,6 { 
            	v1.indgen(1,1) 
            	m.setdiag(i, v1) 
            } 
            m.printf 


    .. warning::
        Implemented only for full and sparse matrices. 


----



.. hoc:method:: Matrix.zero


    Syntax:
        ``mobj = msrcdest.zero()``


    Description:
        Fills the matrix with 0. 

    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.ident


    Syntax:
        ``mobj = msrcdest.ident()``


    Description:
        Fills the principal diagonal with 1. All other elements are set to 0. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,6) 
            m.ident() 
            m.printf() 


    .. warning::
        Implemented only for full matrices. 


----



.. hoc:method:: Matrix.exp


    Syntax:
        ``mobj = msrc.exp()``

        ``mobj = msrc.exp(mout)``


    Description:
        Returns a new matrix which is e^msrc. ie 1 + m + m*m/2 + m*m*m/6 + ... 

    Example:

        .. code-block::
            none

            objref m, v1 
            m = new Matrix(8,8) 
            v1 = new Vector(8) 
            for i=-1,1 { v1.fill(2 - 3*abs(i))  m.setdiag(i, v1) } 
             
            m.exp().printf 


    .. warning::
        Implemented only for full matrices. But doesn't really make sense for 
        any other type since the result would normally be full. 


----



.. hoc:method:: Matrix.pow


    Syntax:
        ``mobj = msrc.pow(i)``

        ``mobj = msrc.pow(i, mout)``


    Description:
        Raise a matrix to a non-negative integer power. 
        Returns a new matrix which is msrc^i. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(6, 6) 
            m.ident 
            m.x[0][5] = m.x[5][0] = 1 
            for i=0, 5 { 
            	print i 
            	m.pow(i).printf 
            } 


    .. warning::
        Implemented only for full matrices. But doesn't really make sense for 
        any other type since the result would normally be full. 


----



.. hoc:method:: Matrix.inverse


    Syntax:
        ``mobj = msrc.inverse()``

        ``mobj = msrc.inverse(mout)``


    Description:
        Return 1/msrc in a new matrix. mobj*msrc = msrc*mobj = identity 

    Example:

        .. code-block::
            none

            objref m, v1, minv 
            m = new Matrix(7,7) 
            v1 = new Vector(7) 
            for i=-1,1 { v1.fill(2 - 3*abs(i))  m.setdiag(i, v1) } 
            minv = m.inverse() 
            m.printf 
            minv.printf 
            m.mulm(minv).printf 


    .. warning::
        Implemented only for full matrices. But doesn't really make sense for 
        any other type since the result would normally be full. 

         

----



.. hoc:method:: Matrix.svd


    Syntax:
        ``dvec = msrc.svd()``

        ``dvec = msrc.svd(umat, vmat)``


    Description:
        Singular value decomposition of a rectangular n x m matrix. 
        On return ut*d*v = m where u is an orthogonal n x n matrix, 
        v is an orthogonal m x m matrix, and d is a diagonal n x m matrix 
        (represented as a vector) whose elements are non-negative and sorted 
        by decreasing value. 
        Note that if m*x = b  then 
        vmat.mulv(x).mul(dvec) = umat.mulv(b) 

    Example:

        .. code-block::
            none

            objref a, umat, vmat, dvec, dmat 
             
            proc svdtest() { 
            	umat = new Matrix() 
            	vmat = new Matrix() 
            	dvec = $o1.svd(umat, vmat) 
            	dmat = new Matrix($o1.nrow, $o1.ncol) 
            	dmat.setdiag(0, dvec) 
            	print "dvec"  dvec.printf 
            	print "dmat"  dmat.printf 
            	print "umat"  umat.printf 
            	print "vmat"  vmat.printf 
            	print "input ", $o1 $o1.printf() 
            	print "ut*d*v" 
            	umat.transpose.mulm(dmat).mulm(vmat).printf 
            } 
             
            a = new Matrix(5, 3) 
            a.setdiag(0, a.getdiag(0).indgen.add(1)) 
            svdtest(a) 
             
            a = new Matrix(6, 6) 
            objref r 
            r = new Random() 
            r.discunif(1,10) 
            for i=0, a.nrow-1 { 
            	a.setrow(i, a.getrow(i).setrand(r)) 
            } 
            svdtest(a) 
             
            a = new Matrix(2,2) 
            a.setrow(0, 1) 
            a.setrow(1, 2) 
            svdtest(a) 
             


    .. warning::
        Implemented only for full matrices. umat and vmat are also full. 

         

----



.. hoc:method:: Matrix.transpose


    Syntax:
        ``mdest = msrc.transpose()``


    Description:
        Return new matrix which is the transpose of the source matrix. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(1,5) 
            for i=0, 4 m.x[0][i] = i 
            m.printf 
            m.transpose.printf 
            m.transpose.mulm(m).printf 
            m.mulm(m.transpose).printf 


    .. warning::
        Implemented only for full matrices. 

         

----



.. hoc:method:: Matrix.symmeig


    Syntax:
        ``veigenvalues = msrc.symmeig(eigenvectors)``


    Description:
        Returns the eigenvalues and eigenvectors of a real symmetric matrix. 
        On exit the eigenvalues are returned  in a new vector and the 
        eigenvectors are returned as an orthogonal matrix. 
        Note that the i'th column of the eigenvector matrix is the eigenvector 
        for the i'th element of the eigenvalue vector. 

    Example:

        .. code-block::
            none

            objref m, q, e 
            m = new Matrix(5,5) 
            m.setdiag(0, 2) 
            m.setdiag(-1, -1) 
            m.setdiag(1, -1) 
            m.printf 
             
            q = new Matrix(1,1) 
            e = m.symmeig(q) 
            print "eigenvectors" 
            q.printf 
             
            print "eigenvalues" 
            e.printf 
             
            print "qt*m*q" 
            q.transpose.mulm(m).mulm(q).printf 
             
            print "qt*q" 
            q.transpose.mulm(q).printf 
             

         

    .. warning::
        Implemented only for full matrices. 
         
        msrc must be symmetric but that fact is not checked. 

         

----



.. hoc:method:: Matrix.to_vector


    Syntax:
        ``vobj = msrc.to_vector()``

        ``vobj = msrc.to_vector(vout)``


    Description:
        Copies the matrix elements into a vector in column order. 
        i.e the jth column starts 
        at vobj.x[msrc.nrow*j] . 
        The vector is sized to nrow*ncol. 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,5) 
            m.from_vector(m.to_vector().indgen).printf 


    .. warning::
        Works for sparse matrices but the output vector will still be size 
        nrow*ncol. 
        Not very efficient since vobj and msrc do not share memory. 

         

----



.. hoc:method:: Matrix.from_vector


    Syntax:
        ``mobj = msrcdest.from_vector(vec)``


    Description:
        Copies the vector elements into the matrix in column order. I.e 
        m[i][j] = v[j*nrow + i]. 
        The size of vec must be equal to msrcdest.nrow()*msrcdest.ncol(). 

    Example:

        .. code-block::
            none

            objref m 
            m = new Matrix(4,5) 
            m.from_vector(m.to_vector().indgen).printf 


    .. warning::
        Works for sparse matrices but all elements will exist so not really sparse. 

         

----



.. hoc:method:: Matrix.cholesky_factor


    Syntax:
        ``mc = msrcdest.cholesky_factor()``


    Description:
        Cholesky factorization in place. msrcdest must be a symmetric positive 
        definite matrix. On return, it is a lower triangular matrix, L, such that 
        L*Ltranspose = msrc 

    Example:

        .. code-block::
            none

            objref m, cf  
            m = new Matrix(3,3) 
            for i=0,2 for j=0,2 m.x[i][j] = (i+j)*(i+j) 
            m.printf 
            cf = m.c.cholesky_factor() 
            cf.mulm(cf.transpose()).printf 

    .. seealso::
        
        cholesky_solve 

         

