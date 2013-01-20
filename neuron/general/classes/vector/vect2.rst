.. _vect2:

         
         
interpolate
-----------



.. class:: interpolate


    Syntax:
        :code:`obj = ysrcdest.interpolate(xdest, xsrc)`

        :code:`obj = ydest.interpolate(xdest, xsrc, ysrc)`


    Description:
        Linearly interpolate points from (xsrc,ysrc) to (xdest,ydest) 
        In the second form, xsrc and ysrc remain unchanged. 
        Destination points outside the domain of xsrc are set to 
        ysrc[0] or ysrc[ysrc.size-1] 

    Example:
        objref g 
        g = new Graph() 
        g.size(0,10,0,100) 
         

        .. code-block::
            none

            //... 
            objref xs, ys, xd, yd 
            xs = new Vector(10) 
            xs.indgen() 
            ys = xs.c.mul(xs) 
            ys.line(g, xs, 1, 0) // black reference line 
             
            xd = new Vector() 
             
            xd.indgen(-.5, 10.5, .1) 
            yd = ys.c.interpolate(xd, xs) 
            yd.line(g, xd, 3, 0) // blue more points than reference 
             
            xd.indgen(-.5, 13, 3) 
            yd = ys.c.interpolate(xd, xs) 
            yd.line(g, xd, 2, 0) // red fewer points than reference 


         

----



.. method:: interpolate.deriv


    Syntax:
        :code:`obj = vdest.deriv(vsrc)`

        :code:`obj = vdest.deriv(vsrc, dx)`

        :code:`obj = vdest.deriv(vsrc, dx, method)`

        :code:`obj = vsrcdest.deriv()`

        :code:`obj = vsrcdest.deriv(dx)`

        :code:`obj = vsrcdest.deriv(dx, method)`


    Description:
        The numerical Euler derivative or the central difference derivative of \ :code:`vec` 
        is placed in \ :code:`vdest`. 
        The variable *dx* gives the increment of the independent variable 
        between successive elements of \ :code:`vec`. 


        *method* = 1 = Euler derivative: 
            \ :code:`vec1[i] = (vec[i+1] - vec[i])/dx` 
 
            Each time this method is used, 
            the first element 
            of \ :code:`vec` is lost since *i* cannot equal -1.  Therefore, since the 
            \ :code:`integral` function performs an Euler 
            integration, the integral of \ :code:`vec1` will reproduce \ :code:`vec` minus the first 
            element. 

        *method* = 2 = Central difference derivative: 
            \ :code:`vec1[i] = ((vec[i+1]-vec[i-1])/2)/dx` 
 
            This method produces an Euler derivative for the first and last 
            elements of \ :code:`vec1`.  The central difference method maintains the 
            same number of elements in \ :code:`vec1` 
            as were in \ :code:`vec` and is a more accurate method than the Euler method. 
            A vector differentiated by this method cannot, however, be integrated 
            to reproduce the original \ :code:`vec`. 

         

    Example:

        .. code-block::
            none

            objref vec, vec1 
            vec = new Vector() 
            vec1 = new Vector() 
            vec.indgen(0, 5, 1) 
            func sq(){ 
            	return $1*$1 
            } 
            vec.apply("sq") 
            vec1.deriv(vec, 0.1) 

        creates \ :code:`vec1` with elements: 

        .. code-block::
            none

            10	20	 
            40	60	 
            80	90 

        Since *dx*=0.1, and there are eleven elements including 0, 
        the entire function exists between the values of 0 and 1, and the derivative 
        values are large compared to the function values. With *dx*=1,the vector 
        \ :code:`vec1` would consist of the following elements: 

        .. code-block::
            none

            1	2	 
            4	6	 
            8	9 

         
        The Euler method vs. the Central difference method:
 
        Beginning with the vector \ :code:`vec`: 

        .. code-block::
            none

            0	1	 
            4	9	 
            16	25 

        \ :code:`vec1.deriv(vec, 1, 1)` (Euler) would go about 
        producing \ :code:`vec1` by the following method: 

        .. code-block::
            none

            1-0   = 1	4-1  = 3		 
            9-4   = 5	16-9 = 7	 
            25-16 = 9 

        whereas \ :code:`vec1.deriv(vec, 1, 2)` (Central difference) would go about 
        producing \ :code:`vec1` as such: 

        .. code-block::
            none

            1-0      = 1		(4-0)/2  = 2	 
            (9-1)/2  = 4		(16-4)/2 = 6	 
            (25-9)/2 = 8		25-16    = 9 


         

----



.. method:: interpolate.integral


    Syntax:
        :code:`obj = vdest.integral(vsrc)`

        :code:`obj = vdest.integral(vsrc, dx)`

        :code:`obj = vsrcdest.integral()`

        :code:`obj = vsrcdest.integral(dx)`


    Description:
        Places a numerical Euler integral of the vsrc elements in vdest. 
        *dx* sets the size of the discretization. 
         
        \ :code:`vdest[i+1] = vdest[i] + vsrc[i+1]` and the first element of \ :code:`vdest` is always 
        equal to the first element of \ :code:`vsrc`. 

    Example:

        .. code-block::
            none

            objref vec, vec1 
            vec = new Vector() 
            vec1 = new Vector() 
            vec.indgen(0, 5, 1)	//vec will have 6 values from 0 to 5, with increment=1 
            vec.apply("sq")		//sq() squares an element  
            			//and is defined in the example for .deriv 
            vec1.integral(vec, 1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            vec1.printf() 

        will print the following elements in \ :code:`vec1` to the screen: 

        .. code-block::
            none

            0	1	5	 
            14	30	55 

        In order to make the integral values more accurate, it is necessary to increase 
        the size of the vector and to decrease the size of *dx*. 

        .. code-block::
            none

            objref vec2 
            vec2 = new Vector(6) 
            vec.indgen(0, 5.1, 0.1)	//vec will have 51 values from 0 to 5, with increment=0.1 
            vec.apply("sq")		//sq() squares an element  
            			//and is defined in the example for .deriv 
            vec1.integral(vec, 0.1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            for i=0,5{vec2.x[i] = vec1.x[i*10]}  //put the value of every 10th index in vec2 
            vec2.printf() 

        will print the following elements in \ :code:`vec2` (which are the elements of 
        \ :code:`vec1` corresponding to the integers 0-5) to the screen: 

        .. code-block::
            none

            0	0.385	2.87 
            9.455	22.14	42.925 

        The integration naturally becomes more accurate as 
        *dx* is reduced and the size of the vector is increased.  If the vector 
        is taken to 501 elements from 0-5 and *dx* is made to equal 0.01, the integrals 
        of the integers 0-5 yield the following (compared to their continuous values 
        on their right). 

        .. code-block::
            none

            0.00000 -- 0.00000	0.33835 --  0.33333	2.6867  --  2.6666 
            9.04505 -- 9.00000	21.4134 -- 21.3333	41.7917 -- 41.6666 


         

----



.. method:: interpolate.median


    Syntax:
        :code:`median = vsrc.median()`


    Description:
        Find the median value of \ :code:`vec`. 

         

----



.. method:: interpolate.medfltr


    Syntax:
        :code:`obj = vdest.medfltr(vsrc)`

        :code:`obj = vdest.medfltr(vsrc, points)`

        :code:`obj = vsrcdest.medfltr()`

        :code:`obj = vsrcdest.medfltr( points)`


    Description:
        Apply a median filter to vsrc, producing a smoothed version in vdest. 
        Each point is replaced with the median value of the *points* on 
        either side. 
        This is typically used for eliminating spikes from data. 

         

----



.. method:: interpolate.sort


    Syntax:
        :code:`obj = vsrcdest.sort()`


    Description:
        Sort the elements of \ :code:`vec1` in place, putting them in numerical order. 

         

----



.. method:: interpolate.sortindex


    Syntax:
        :code:`vdest = vsrc.sortindex()`

        :code:`vdest = vsrc.sortindex(vdest)`


    Description:
        Return a new vector of indices which sort the vsrc elements in numerical 
        order. That is vsrc.index(vsrc.sortindex) is equivalent to vsrc.sort(). 
        If vdest is present, use that as the destination vector for the indices. 
        This, if it is large enough, avoids the destruct/construct of vdest. 

    Example:

        .. code-block::
            none

            objref a, r, si 
            r = new Random() 
            r.uniform(0,100) 
            a = new Vector(10) 
            a.setrand(r) 
            a.printf 
             
            si = a.sortindex 
            si.printf 
            a.index(si).printf 

         

         

----



.. method:: interpolate.reverse


    Syntax:
        :code:`obj = vsrcdest.reverse()`


    Description:
        Reverses the elements of \ :code:`vec` in place. 

         

----



.. method:: interpolate.rotate


    Syntax:
        :code:`obj = vsrcdest.rotate(value)`

        :code:`obj = vsrcdest.rotate(value, 0)`


    Description:
        A negative *value* will move elements to the left.  A positive argument 
        will move elements to the right.  In both cases, the elements shifted off one 
        end of the vector will reappear at the other end. 
        If a 2nd arg is present, 0 values get shifted in and elements shifted off 
        one end are lost. 

    Example:

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec.rotate(3) 

        orders the elements of \ :code:`vec` as follows: 

        .. code-block::
            none

            8  9  10  1  2  3  4  5  6  7 

        whereas, 

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec.rotate(-3) 

        orders the elements of \ :code:`vec` as follows: 

        .. code-block::
            none

            4  5  6  7  8  9  10  1  2  3 


        .. code-block::
            none

            objref vec 
            vec = new Vector() 
            vec.indgen(1,5,1) 
            vec.printf 
            vec.c.rotate(2).printf 
            vec.c.rotate(2, 0).printf 
            vec.c.rotate(-2).printf 
            vec.c.rotate(-2, 0).printf 


         

----



.. method:: interpolate.rebin


    Syntax:
        :code:`obj = vdest.rebin(vsrc,factor)`

        :code:`obj = vsrcdest.rebin(factor)`


    Description:
        Compresses length of vector \ :code:`vsrc` by an integer *factor*.  The sum of 
        elements is conserved, unless the *factor* produces a remainder, 
        in which case the remainder values are truncated from \ :code:`vdest`. 

    Example:

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec1.rebin(vec, 2) 

        produces \ :code:`vec1`: 

        .. code-block::
            none

            3  7  11  15  19 

        where each pair of \ :code:`vec ` elements is added together into one element. 
         
        But, 

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec1.rebin(vec, 3) 

        adds trios \ :code:`vec` elements and gets rid of the value 10, producing 
        \ :code:`vec1`: 

        .. code-block::
            none

            6  15  24 


         

----



.. method:: interpolate.pow


    Syntax:
        :code:`obj = vdest.pow(vsrc, power)`

        :code:`obj = vsrcdest.pow(power)`


    Description:
        Raise each element to some power. A power of -1, 0, .5, 1, or 2 
        are efficient. 

         

----



.. method:: interpolate.sqrt


    Syntax:
        :code:`obj = vdest.sqrt(vsrc)`

        :code:`obj = vsrcdest.sqrt()`


    Description:
        Take the square root of each element. No domain checking. 

         

----



.. method:: interpolate.log


    Syntax:
        :code:`obj = vdest.log(vsrc)`

        :code:`obj = vsrcdest.log()`


    Description:
        Take the natural log of each element. No domain checking. 

         

----



.. method:: interpolate.log10


    Syntax:
        :code:`obj = vdest.log10(vsrc)`

        :code:`obj = vsrcdest.log10()`


    Description:
        Take the logarithm to the base 10 of each element. No domain checking. 

         

----



.. method:: interpolate.tanh


    Syntax:
        :code:`obj = vdest.tanh(vsrc)`

        :code:`obj = vsrcdest.tanh()`


    Description:
        Take the hyperbolic tangent of each element. 

         

----



.. method:: interpolate.abs


    Syntax:
        :code:`obj = vdest.abs(vsrc)`

        :code:`obj = vsrcdest.abs()`


    Description:
        Take the absolute value of each element. 

    Example:

        .. code-block::
            none

            objref v1 
            v1 = new Vector() 
            v1.indgen(-.5, .5, .1) 
            v1.printf() 
            v1.abs.printf() 


    .. seealso::
        :func:`math`

         

----



.. method:: interpolate.index


    Syntax:
        :code:`obj = vdest.index(vsrc,  indices)`


    Description:
        The values of the vector \ :code:`vsrc` indexed by the vector *indices* are collected 
        into \ :code:`vdest`. 
         

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2, vec3 
            vec = new Vector() 
            vec1 = new Vector() 
            vec2 = new Vector() 
            vec3 = new Vector(6) 
            vec.indgen(0, 5.1, 0.1)	//vec will have 51 values from 0 to 5, with increment=0.1 
            vec1.integral(vec, 0.1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            vec2.indgen(0, 50,10) 
            vec3.index(vec1, vec2)  //put the value of every 10th index in vec2 

        makes \ :code:`vec3` with six elements corresponding to the integrated integers from 
        \ :code:`vec`. 

         

----



.. method:: interpolate.min


    Syntax:
        :code:`x = vec.min()`

        :code:`x = vec.min(start, end)`


    Description:
        Return the minimum value. 

         

----



.. method:: interpolate.min_ind


    Syntax:
        :code:`i = vec.min_ind()`

        :code:`i = vec.min_ind(start, end)`


    Description:
        Return the index of the minimum value. 

         

----



.. method:: interpolate.max


    Syntax:
        :code:`x = vec.max()`

        :code:`x = vec.max(start, end)`


    Description:
        Return the maximum value. 

         

----



.. method:: interpolate.max_ind


    Syntax:
        :code:`i = vec.max_ind()`

        :code:`i = vec.max_ind(start, end)`


    Description:
        Return the index of the maximum value. 

         

----



.. method:: interpolate.sum


    Syntax:
        :code:`x = vec.sum()`

        :code:`x = vec.sum(start, end)`


    Description:
        Return the sum of element values. 

         

----



.. method:: interpolate.sumsq


    Syntax:
        :code:`x = vec.sumsq()`

        :code:`x = vec.sumsq(start, end)`


    Description:
        Return the sum of squared element values. 

         

----



.. method:: interpolate.mean


    Syntax:
        :code:`x =  vec.mean()`

        :code:`x =  vec.mean(start, end)`


    Description:
        Return the mean of element values. 

         

----



.. method:: interpolate.var


    Syntax:
        :code:`x = vec.var()`

        :code:`x = vec.var(start, end)`


    Description:
        Return the variance of element values. 

         

----



.. method:: interpolate.stdev


    Syntax:
        :code:`vec.stdev()`

        :code:`vec.stdev(start,end)`


    Description:
        Return the standard deviation of the element values. 

         

----



.. method:: interpolate.stderr


    Syntax:
        :code:`x = vec.stderr()`

        :code:`x = vec.stderr(start, end)`


    Description:
        Return the standard error of the mean (SEM) of the element values. 

         

----



.. method:: interpolate.dot


    Syntax:
        :code:`x = vec.dot(vec1)`


    Description:
        Return the dot (inner) product of \ :code:`vec` and *vec1*. 

         

----



.. method:: interpolate.mag


    Syntax:
        :code:`x = vec.mag()`


    Description:
        Return the vector length or magnitude. 

         

----



.. method:: interpolate.add


    Syntax:
        :code:`obj = vsrcdest.add(scalar)`

        :code:`obj = vsrcdest.add(vec1)`


    Description:
        Add either a scalar to each element of the vector or add the corresponding 
        elements of *vec1* to the elements of \ :code:`vsrcdest`. 
        \ :code:`vsrcdest` and *vec1* must have the same size. 

         

----



.. method:: interpolate.sub


    Syntax:
        :code:`obj = vsrcdest.sub(scalar)`

        :code:`obj = vsrcdest.sub(vec1)`


    Description:
        Subtract either a scalar from each element of the vector or subtract the 
        corresponding elements of *vec1* from the elements of \ :code:`vsrcdest`. 
        \ :code:`vsrcdest` and *vec1* must have the same size. 

         

----



.. method:: interpolate.mul


    Syntax:
        :code:`obj = vsrcdest.mul(scalar)`

        :code:`obj = vsrcdest.mul(vec1)`


    Description:
        Multiply each element of \ :code:`vsrcdest` either by either a scalar or the 
        corresponding elements of *vec1*.  \ :code:`vsrcdest` 
        and *vec1* must have the same size. 

         

----



.. method:: interpolate.div


    Syntax:
        :code:`obj = vsrcdest.div(scalar)`

        :code:`obj = vsrcdest.div(vec1)`


    Description:
        Divide each element of \ :code:`vsrcdest` either by a scalar or by the 
        corresponding elements of *vec1*.  \ :code:`vsrcdest` 
        and *vec1* must have the same size. 

         

----



.. method:: interpolate.scale


    Syntax:
        :code:`scale = vsrcdest.scale(low, high)`


    Description:
        Scale values of the elements of a vector to lie within the given range. 
        Return the scale factor used. 

         

----



.. method:: interpolate.eq


    Syntax:
        :code:`boolean = vec.eq(vec1)`


    Description:
        Test equality of vectors.  Returns 1 if all elements of vec == 
        corresponding elements of *vec1* (to within :func:`float_epsilon` ). 
        Otherwise it returns 0. 

         

----



.. method:: interpolate.meansqerr


    Syntax:
        :code:`x = vec.meansqerr(vec1)`

        :code:`x = vec.meansqerr(vec1, weight_vec)`


    Description:
        Return the mean squared error between values of the elements of \ :code:`vec` and 
        the corresponding elements of *vec1*.  \ :code:`vec` and *vec1* must have the 
        same size. 
         
        If the second vector arg is present, it also must have the same size and the 
        return value is sum of w[i]*(v1[i] - v2[i])^2 / size 

         

----



.. method:: interpolate.Fourier

        The following routines are based on the fast fourier transform (FFT) 
        and are implemented using code from Numerical Recipes in C (2nd ed.) 
        Refer to this source for further information. 
         

----



.. method:: interpolate.correl


    Syntax:
        :code:`obj = vdest.correl(src)`

        :code:`obj = vdest.correl(src, vec2)`


    Description:
        Compute the cross-correlation function of *src* and *vec2* (or the 
        autocorrelation of *src* if *vec2* is not present). 

         

----



.. method:: interpolate.convlv


    Syntax:
        :code:`obj = vdest.convlv(src,filter)`

        :code:`obj = vdest.convlv(src,filter, sign)`


    Description:
        Compute the convolution of *src* with *filter*.  If <sign>=-1 then 
        compute the deconvolution. 
        Assumes filter is given in "wrap-around" order, with countup 
        \ :code:`t=0..t=n/2` followed by countdown \ :code:`t=n..t=n/2`.  The size of *filter* 
        should be an odd <= the size of *v1*>. 

    Example:

        .. code-block::
            none

            objref v1, v2, v3 
            v1 = new Vector(16) 
            v2 = new Vector(16) 
            v3 = new Vector() 
            v1.x[5] = v1.x[6] = 1 
            v2.x[3] = v2.x[4] = 3 
            v3.convlv(v1, v2) 
            v1.printf() 
            v2.printf() 
            v3.printf() 


         

----



.. method:: interpolate.spctrm


    Syntax:
        :code:`obj = vdest.spctrm(vsrc)`


    Description:
        Return the power spectral density function of vsrc. 

         

----



.. method:: interpolate.filter


    Syntax:
        :code:`obj = vdest.filter(src,filter)`

        :code:`obj = vsrcdest.filter(filter)`


    Description:
        Digital filter implemented by taking the inverse fft of 
        *filter* and convolving it with *vec1*.  *vec* and *vec1* 
        are in the time 
        domain and *filter* is in the frequency domain. 

         

----



.. method:: interpolate.fft


    Syntax:
        :code:`obj = vdest.fft(vsrc, sign)`

        :code:`obj = vsrcdest.fft(sign)`


    Description:
        Compute the fast fourier transform of the source data vector.  If 
        *sign*=-1 then compute the inverse fft. 
         
        If vsrc.size() is not an integral power of 2, it is padded with 0's to 
        the next power of 2 size. 
         
        The complex frequency domain is represented in the vector as pairs of 
        numbers --- except for the first two numbers. 
        vec.x[0] is the amplitude of the 0 frequency cosine (constant) 
        and vec.x[1] is the amplitude of the highest (N/2) frequency cosine 
        (ie. alternating 1,-1's in the time domain) 
        vec.x[2, 3] is the amplitude of the cos(2*PI*i/n), sin(2*PI*i/n) components 
        (ie. one whole wave in the time domain) 
        vec.x[n-2, n-1] is the amplitude of the cos(PI*(n-1)*i/n), sin(PI*(n-1)*i/n) 
        components. The following example of a pure time domain sine wave 
        sampled at 16 points should be played with to see where 
        the specified frequency appears in the frequency domain vector (note that if the 
        frequency is greater than 8, aliasing will occur, ie sampling makes it appear 
        as a lower frequency) 
        Also note that the forward transform does not produce the amplitudes of 
        the frequency components that goes up to make the time domain function but 
        instead each element is the integral of the product of the time domain 
        function and a specific pure frequency. Thus the 0 and highest frequency 
        cosine are N times the amplitudes and all others are N/2 times the amplitudes. 
         
        objref box, g1, g2, g3 
        objref v1, v2, v3 
         
        proc setup_gui() { 
        box = new VBox() 
        box.intercept(1) 
        xpanel("", 1) 
        xradiobutton("sin   ", "c=0  p()") 
        xradiobutton("cos   ", "c=1  p()") 
        xvalue("freq (waves/domain)", "f", 1, "p()") 
        xpanel() 
        g1 = new Graph() 
        g2 = new Graph() 
        g3 = new Graph() 
        box.intercept(0) 
        box.map() 
        g1.size(0,N, -1, 1) 
        g2.size(0,N, -N, N) 
        g3.size(0,N, -N, N) 
        } 
        @code...	//define a gui for this example 
         
        N=16	// should be power of 2 
        c=1	// 0 -> sin   1 -> cos 
        f=1	// waves per domain, max is N/2 
        setup_gui() // construct the gui for this example 
         
        proc p() { 
        v1 = new Vector(N) 
        v1.sin(f, c*PI/2, 1000/N) 
        v1.plot(g1) 
         
        v2 = new Vector() 
        v2.fft(v1, 1)		// forward 
        v2.plot(g2) 
         
        v3 = new Vector() 
        v3.fft(v2, -1)		// inverse 
        v3.plot(g3)		// amplitude N/2 times the original 
        } 
         
        p() 

         
        The inverse fft is mathematically almost identical 
        to the forward transform but often 
        has a different operational interpretation. In this 
        case the result is a time domain function which is merely the sum 
        of all the pure sinusoids weighted by the (complex) frequency function 
        (although, remember, points 0 and 1 in the frequency domain are special, 
        being the constant and the highest alternating cosine, respectively). 
        The example below shows the index of a particular frequency and phase 
        as well as the time domain pattern. Note that index 1 is for the higest 
        frequency cosine instead of the 0 frequency sin. 
         
        Because the frequency domain representation is something only a programmer 
        could love, and because one might wish to plot the real and imaginary 
        frequency spectra, one might wish to encapsulate the fft in a function 
        which uses a more convenient representation. 
         
        Below is an alternative FFT function where the frequency 
        values are spectrum amplitudes (no need to divide anything by N) 
        and the real and complex frequency components are 
        stored in separate vectors (of length N/2 + 1). 
         
        Consider the functions 

    Syntax:
        :code:`FFT(1, vt_src, vfr_dest, vfi_dest)`

        :code:`FFT(-1, vt_dest, vfr_src, vfi_src)`


    Description:
         
        The forward transform (first arg = 1) requires 
        a time domain source vector with a length of N = 2^n where n is some positive 
        integer. The resultant real (cosine amplitudes) and imaginary (sine amplitudes) 
        frequency components are stored in the N/2 + 1 
        locations of the vfr_dest and vfi_dest vectors respectively (Note: 
        vfi_dest.x[0] and vfi_dest.x[N/2] are always set to 0. The index i in the 
        frequency domain is the number of full pure sinusoid waves in the time domain. 
        ie. if the time domain has length T then the frequency of the i'th component 
        is i/T. 
         
        The inverse transform (first arg = -1) requires two freqency domain 
        source vectors for the cosine and sine amplitudes. The size of these 
        vectors must be N/2+1 where N is a power of 2. The resultant time domain 
        vector will have a size of N. 
         
        If the source vectors are not a power of 2, then the vectors are padded 
        with 0's til vtsrc is 2^n or vfr_src is 2^n + 1. The destination vectors 
        are resized if necessary. 
         
        This function has the property that the sequence 

        .. code-block::
            none

            FFT(1, vt, vfr, vfi) 
            FFT(-1, vt, vfr, vfi) 

        leaves vt unchanged. Reversal of the order would leave vfr and vfi unchanged. 
         
        The implementation is:
 

        .. code-block::
            none

            proc FFT() {local n, x 
                    if ($1 == 1) { // forward 
                            $o3.fft($o2, 1) 
                            n = $o3.size() 
                            $o3.div(n/2) 
                            $o3.x[0] /= 2	// makes the spectrum appear discontinuous 
                            $o3.x[1] /= 2	// but the amplitudes are intuitive 
             
                            $o4.copy($o3, 0, 1, -1, 1, 2)   // odd elements 
                            $o3.copy($o3, 0, 0, -1, 1, 2)   // even elements 
                            $o3.resize(n/2+1) 
                            $o4.resize(n/2+1) 
                            $o3.x[n/2] = $o4.x[0]           //highest cos started in o3.x[1 
                            $o4.x[0] = $o4.x[n/2] = 0       // weights for sin(0*i)and sin(PI*i) 
            	}else{ // inverse 
                            // shuffle o3 and o4 into o2 
                            n = $o3.size() 
                            $o2.copy($o3, 0, 0, n-2, 2, 1) 
                            $o2.x[1] = $o3.x[n-1] 
                            $o2.copy($o4, 3, 1, n-2, 2, 1) 
                            $o2.x[0] *= 2 
                            $o2.x[1] *= 2  
                            $o2.fft($o2, -1) 
                    } 
            } 

        If you load the previous example so that FFT is defined, the following 
        example shows the cosine and sine spectra of a pulse. 
 
        objref v1, v2, v3, v4 
        objref box, g1, g2, g3, g4, b1 
         
        proc setup_gui() { 
        box = new VBox() 
        box.intercept(1) 
        xpanel("") 
        xvalue("delay (points)", "delay", 1, "p()") 
        xvalue("duration (points)", "duration", 1, "p()") 
        xpanel() 
        g1 = new Graph() 
        b1 = new HBox() 
        b1.intercept(1) 
        g2 = new Graph() 
        g3 = new Graph() 
        b1.intercept(0) 
        b1.map() 
        g4 = new Graph() 
        box.intercept(0) 
        box.map() 
        g1.size(0,N, -1, 1) 
        g2.size(0,N/2, -1, 1) 
        g3.size(0,N/2, -1, 1) 
        g4.size(0,N, -1, 1) 
        } 
        @code... 
        N=128 
        delay = 0 
        duration = N/2 
        setup_gui() 
        proc p() { 
        v1 = new Vector(N) 
        v1.fill(1, delay, delay+duration-1) 
        v1.plot(g1) 
         
        v2 = new Vector() 
        v3 = new Vector() 
        FFT(1, v1, v2, v3) 
        v2.plot(g2) 
        v3.plot(g3) 
         
        v4 = new Vector() 
        FFT(-1, v4, v2, v3) 
        v4.plot(g4) 
        } 
        p() 
         


    .. seealso::
        :func:`fft`, :func:`spctrm`


