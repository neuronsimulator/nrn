: total current is ib + weighted events that decay with time constant taus
: total current is integrated and when passes 1, then fire
: i.e. taus * di/dt + i = ib  where if event occurs at time t1 then
:  i(t1) += w
: and taum*dm/dt + m = i where fire if m crosses 1 in a positive direction
: and after firing, m = 0.
: Require, taum < taus, ie synaptic current longer than membrane time constant.

NEURON {
	ARTIFICIAL_CELL IntFire2
	RANGE taus, taum, ib, i, m
}

PARAMETER {
	taus = 20 (ms)
	taum = 10 (ms)
	ib = 0
}

ASSIGNED {
	i  m
	t0 (ms)
}

FUNCTION M() {
	LOCAL x, f	
	f = taus/(taus - taum)
	x = (i - ib)*exp(-(t - t0)/taus)
	M = ib + f*x + (m - (ib + (i - ib)*f))*exp(-(t - t0)/taum)
}

FUNCTION I() {
	LOCAL x, f
	f = taus/(taus - taum)
	x = (i - ib)*exp(-(t - t0)/taus)
	I = x + ib
}

INITIAL {
	i = ib
	m = 0
	if (taus <= taum) {
		taus = taum + .1
	}
	t0 = t
	net_send(firetime(ib, 0, 0), 1)
}

NET_RECEIVE (w) {
	LOCAL x, inew, f, ibf
	f = taus/(taus - taum)
	x = (i - ib)*exp(-(t - t0)/taus)
	inew = ib + x
	
	if (flag == 1) { : time to fire
:printf("now firing t=%g inew=%g\n", t, inew)
		net_event(t)
		m = 0
		ibf = (inew - ib)*f
		net_send(firetime(ib, ibf,  0), 1)
	}else{
		m = ib + f*x + (m - (ib + (i - ib)*f))*exp(-(t - t0)/taum)
:printf("event w=%g i=%g m=%g\n", w, inew, m)
		inew = inew + w
		if (m >= 1) {
			net_move(t) : the time to fire is now
		}else{
			ibf = (inew - ib)*f
			net_move(firetime(ib, ibf, m) + t)
		}
	}
	t0 = t
	i = inew
}

FUNCTION firetime(a, b, c) (ms) {
	: find 1 = a + b*exp(-t/taus) + (c - a - b)*exp(-t/taum)
	: assert c < 1  taum < taus
	LOCAL r, cab, x, m, f

	r = taum/taus

	: for numerical convenience when b is not too negative solve
	: 1 = a + b*x^r + (c - a - b)*x where 0<x<1
	: starting at x = 1
	: if the slope at 1 points out of the domain ( b is too negative)
	: then switch to solving 1 = a + b*x + (c - a - b)*x^(1/r)

	if (a <= 1 && a + b <= 1) {
		firetime = 1e9
	}else if (a > 1 && b <= 0) {
		: must be one solution in the domain since a > 1
		: find starting point
		cab = c - a - b
		m = r*b + cab : slope at x=1
		if (m >= 0) {
			: switch to alternative
			firetime = -taus * log(newton1(a,cab,b,1/r,0))
		}else{
			:intercept at f=1  note: value at x=1 is c
			x = (1 - c + m)/m
			if (x <= 0) {
				:switch to alternative
				firetime = -taus*log(newton1(a,cab,b,1/r,0))
			}else{
				: newton starting at x
				firetime = -taum*log(newton1(a,b,cab,r,x))
			}
		}
	}else{
		: if a solution exists then newtons method starting from
		: x=1 will converge to the proper one.
		: a solution does not exist if f(tm) <= 1
		cab = c - a - b
		: max at
		x = (-cab/(r*b))^(1/(r-1))
		f = a + b*x^r + cab*x
:printf("max at x=%g val=%g\n", x, f)
		: believe it or not, there are cases where the max is
		: located at x > 1. e.g. a=0, b=1.1, c=.9, r=.5
		: so both parts of the test are essential
		if (x >= 1 || f <= 1) {
			firetime = 1e9
		}else{
			m = r*b + cab
			x = (1 - c + m)/m
			firetime = -taum*log(newton1(a,b,cab,r,x))
		}
	}
:printf("firetime=%g\n", firetime)
}

FUNCTION newton1(a,b,c,r,x) {
	: solve 1 = a + b*x^r + c*x starting from x
	: derivative is r*b*x^(r-1) + c
	: must stay in domain 0 < x <= 1
	LOCAL dx, f, df, iter, xm
	dx = 1
	f = 0
	iter = 0
:printf("newton a=%g b=%g c=%g r=%g x=%g\n", a, b, c, r, x)
	while( fabs(dx) > 1e-6 || fabs(f - 1) > 1e-6) {
		f = a + b*x^r + c*x
		df = r*b*x^(r-1) + c
		dx = (1 - f)/df
:printf("newton x=%g f=%g df=%g dx=%g\n", x, f, df, dx)
		x = x + dx
		if (x <= 0) {
			x = 1e-4
		}else if (x > 1) {
			x = 1
		}
		iter = iter + 1
		if (iter == 6 && r < 1) {
			xm = (-c/(r*b))^(1/(r-1))
:printf("max at x=%g val=%g\n", xm, (a + b*xm^r + c*xm))
		}
		if (iter >5) {
:printf("Intfire2 iter %g x=%g f=%g df=%g dx=%g a=%g b=%g c=%g r=%g\n",
:iter, x, f, df, dx, a, b, c, r)
		}
		if (iter > 10) {
printf("Intfire2 iter %g x=%g f=%g df=%g dx=%g a=%g b=%g c=%g r=%g\n",
iter, x, f, df, dx, a, b, c, r)
			f=1 dx=0
		}
	}
	newton1 = x
}
