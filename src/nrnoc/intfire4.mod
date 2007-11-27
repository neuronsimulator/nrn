COMMENT

Artificial cell with fast exponential decay excitatory current and
slower alpha function like inhibitory current
integrated by even slower membrane. Fires when membrane state = 1
On firing, only the membrane state returns to 0

Although the four states, e, i1, i2, m are available, plotting will be
more pleasing if one uses the function E, I, and M

Fast excitatory current, E(t), has single exponential decay with time constant, taue.

Slower inhibitory current, I2(t), is alpha function like and has two time contants,
taui1 and  taui2.

The leaky integrator membrane has the slowest time constant of all, taum.
The total current is the input to this integrator.

Cell fires when state m passes 1
and after firing, m = 0.

Require taum > taui2 > taui1 > taue
or in other words ke > ki1 > ki2 > km

States are normalized so that an excitatory weight of 1 produces
a maximum e of 1 and a maximum m of 1
Also an inhibitory weight of -1 produces a maximum i2 of -1
and a maximum m of -1.

Basic equations.

Note:
The equation dy(t)/dt = -k*y(t) + a*exp(-k1*t)
has the solution
y(t) = y0*exp(-k*t) + b * (exp(-k*t) - exp(-k1*t))
where b = a/(k1 - k)

The excitatory current is governed by dE(t)/dt = -ke E(t)
so the excitatory current E(t) = e*exp(-ke*t)
At an excitatory event, e <- e + w  (w > 0)

The inhibitory current is derived from the scheme
  ki1    ki2
i1 -> i2 -> bath   where ki1 > ki2
At an inhibitory event, i1 <- i1 + w  (w < 0)

dI1(t)/dt = - ki1*I1(t)
so  I1(t) = i1*exp(-ki1*t)

and dI2(t)/dt =  -ki2*I2(t) + ai1*I1(t)
where ai1 will be chosen so that if i1=1 then the max of I2(t) is 1
so I2(t) = i2*exp(-ki2*t) + bi1*i1*(exp(-ki2*t) - exp(-ki1*t)
where bi1 = ai1/(ki1 - ki2)

The membrane voltage is
dM(t)/dt = -km*M(t) + ae*E(t) + ai2*I2(t)
where ae and ai2 will be chosen so that if e=1 then the max of M(t) is 1
and if the max of I2(t) is 1 (i1=1, e=0, i2=0, m=0 ) then the max of M(t)
is 1

A spike event is generated when M(t) = 1
at which time m is set to 0 and E, I1, and I2 remain continuous.
i.e. if m == 1 then m = 0 and e,i1,i2 are unchanged.

So the equations are

 dE(t)/dt  = -ke*E(t)
 dI1(t)/dt = -ki1*I1(t)
 dI2(t)/dt = -ki2*I2(t) + ai1*I1(t)
 dM(t)/dt  = -km*M(t) + ae*E(t) + ai2*I2(t)

The initial conditions are e, i1, i2, and m

The analytic solution is

E(t) = e*exp(-ke*t)
I1(t) = i1*exp(-ki1*t)
I2(t) = i2*exp(-ki2*t) + bi1*i1*(exp(-ki2*t) - exp(-ki1*t))
M(t) = m * exp(-km*t)
	+ be*e*(exp(-km*t) - exp(-ke*t))
	+ bi2*i2*(exp(-km*t) - exp(-ki2*t))
	+ bi2*bi1*i1 * (exp(-km*t) - exp(-ki2*t))
	- bi2*bi1*i1*(ki2 - km)/(ki1 - km)*(exp(-km*t) - exp(-ki1*t))

The derivative of this function at t=0 is
m' = dM/dt(0) = -km*m + ae*e + ai2*i2
as expected by the formula for dM(t)/dt
and note that the derivative (at t=0) of the last two terms of M(t) add up to 0

The algorithm uses self events to successively approximate the firing
time. On an event, e, i1, i2, m are calculated analytically.
if m > 1 - epsilon, a net_event is generated (the cell fires)
and m is set to 0. The self event is then moved or reissued
to a new approximate firing time. The approximate
firing time is computed from the present value of m and the slope of m, i.e
m + m'*t = 1. If m' is negative then the firing time set to t=1e9.

We need to prove that the approximate firing time is never
later than the true firing time. Otherwise the simulation would be in error.
It is also practically important for the successive approximation to
converge rapidly to the true firing time to avoid the overhead of
a large number of self events. Since the slope approximation to the
function is equivalent to the Newton method for finding M(t) = 1
we only expect slow convergence when M(t) has its maximum value close
to 1. The use of a sequence of self-events is superior to
carrying out a complete newton method solution of the firing time
because it is most often the case that
external input events occur in the interval between firing times and these
events invalidate the previous computation of the next firing time.
It is an experimental question how many iterations should be carried out
per self event since the self-event overhead partly depends on the
number of outstanding events in the event queue.
A single newton iteration generally takes longer than
the overhead associated with self-events.

The proof takes place in two major parts. First we show that if m' <= 0
then M(t) < 1. Next we show that if m' > 0 then m + m'*t = 1 underestimates
the firing time. This latter part is divided into the cases m <= 0
and m > 0.

Lemma:
If
	k1 > k2 > k
	f1(t) = exp(-k*t) - exp(-k1*t)
	f2(t) = exp(-k*t) - exp(-k2*t)
Then
	f1(t)/(k1 - k) <= f2(t)/(k2 - k)
	For all t>=0
	(We can also express the result as f1(t)*(k2 - k)/(k1 - k) <= f2(t) )
Proof:
	First note that f1(0) = f2(0) = 0 so the lemma holds at t=0.
	Also note that df1/dt(0) = k1 - k
		  and  df2/dt(0) = k2 - k
	so we have scaled the equation so each side has the same slope=1

	Now consider t > 0.
	exp(-k*t) > exp(-k1*t) > exp(-k2*t)
	so it is safe to divide by (exp(-k*t) - exp(-k2*t))
	and we can write
	f1(t)/(k1 - k) - f2(t)/(k2 - k)
	= (k2-k)/f2(t) * ( f1(t)/f2(t) * (k2 - k)/(k1 - k) - 1)
	The first term is positive
	Also (k2 - k)/(k1 - k) < 1
	Also f1 and f2 are positive and because e(-k1*t) > e(-k2*t) then
	f1/f2 < 1. Thus the second term is negative. Therefore the entire
	expression < 0
QED.

Remark: In the limit as k2 approaches k we have
	f1(t)/(k1 - k) <= t*exp(-k*t)

We now try to establish that if m' is negative then the firing time
is infinity. That is, we prove that if m < 1 and m' <= 0 then
M(t) < 1 regardless of e, i1, or i2.

From the lemma we see that the last two major terms of M(t) add up to a quantity
that is < 0. i.e.
	bi2*bi1*i1*[
		  (exp(-km*t) - exp(-ki2*t))
		- (exp(-km*t) - exp(-ki1*t))*(ki2 - km)/(ki1 - km)
		]
is negative since
i1 is negative and since ki1 > ki2 > km the term in the brackets is positive.
Thus
M(t) <= m*exp(-km*t)
	+ be*e*(exp(-km*t) - exp(-ke*t))
	+ bi2*i2*(exp(-km*t) - exp(-ki2*t))

Now the last term is negative and we can use our lemma again to
replace it with a term that is not so negative. i.e
M(t) <= m*exp(-km*t)
	+ be*e*(exp(-km*t) - exp(-ke*t))
	+ bi2*i2*(ki2 - km)/(ke - km)*(exp(-km*t) - exp(-ke*t))
Rewriting as
M(t) <= m*exp(-km*t)
	+ 1/(ke - km) * [ ae*e + ai2*i2 ] * (exp(-km*t) - exp(-ke*t))
we note that the term in brackets is  m' + km*m
so
M(t) <= m*exp(-km*t)
	+ km/(ke - km)*m*(exp(-km*t) - exp(-ke*t))
	+ m' / (ke - km) * (exp(-km*t) - exp(-ke*t))

Since m' <= 0 the last term is <= 0 and we can remove it and write
M(t) <= m*[exp(-km*t) + km/(ke - km)*(exp(-km*t) - exp(-ke*t))]
Since m < 1 all we need to do is prove that the term is brackets is <=1.

Factoring the brackets term gives
ke/(ke - km)*exp(-km*t) - km/(ke - km)*exp(-ke*t)

Note, that at t=0, the term in brackets = 1.
Also note that the derivative of that term is
-ke*km/(ke - km)*exp(-km*t) + ke*km/(ke - km)*exp(-ke*t)
or
-ke*km/(ke - km)* (exp(-km*t) - exp(-ke*t))
which is negative. A function which is 1 at t=0 and has a negative derivative
everywhere must be <= 1 everywhere.

The last thing to prove is that the newton iteration with
m' > 0 underestimates the firing time.

We start from the place in the last proof where
M(t) <= m*exp(-km*t)
	+ km/(ke - km)*m*(exp(-km*t) - exp(-ke*t))
	+ m' / (ke - km) * (exp(-km*t) - exp(-ke*t))

only now, m' > 0 so the last term is positive and we can replace it
(from our lemma) with the larger term, m'*t*exp(-km*t) to get
M(t) <= m*exp(-km*t)
	+ km/(ke - km)*m*(exp(-km*t) - exp(-ke*t))
	+ m'*t*exp(-km*t)

Now consider the case where m > 0.
The first two lines add up to a function that is less than or equal to m.
I.e. for m > 0
M(t) <= m + m'*t
and so the newton iteration underestimates the true firing time.

Now consider case where m < 0.

The second line of the above function is negative so we can throw it
out and write
M(t) <= (m + m'*t)*exp(-km*t)

But when (m + m'*t) is positive, the left hand side
is less than m + m'*t which is the newton iteration
line we use to calculate
m + m' * t = 1. Thus the newton iteration underestimates the firing time.

ENDCOMMENT

NEURON {
	ARTIFICIAL_CELL IntFire4
	RANGE taue, taui1, taui2, taum, e, i1, i2, m
	RANGE nself, nexcite, ninhibit
	GLOBAL eps
}

PARAMETER {
	taue = 5 (ms)
	taui1 = 10 (ms)
	taui2 = 20 (ms)
	taum = 50 (ms)
	ib = 0
	eps = 1e-6
}

ASSIGNED {
	e i1 i2 m
	enew i1new i2new mnew
	t0 (ms)
	nself nexcite ninhibit

	ke (1/ms) ki1 (1/ms) ki2 (1/ms) km (1/ms)
	ae (1/ms) ai1 (1/ms) ai2 (1/ms)
	be bi1 bi2
	a b
}

PROCEDURE newstates(d(ms)) {
	LOCAL ee, ei1, ei2, em
	
	ee = exp(-ke*d)
	ei1 = exp(-ki1*d)
	ei2= exp(-ki2*d)
	em = exp(-km*d)
	enew = e*ee
	i1new = i1*ei1
	i2new = i2*ei2 + bi1*i1*(ei2 - ei1)
	mnew = m*em
		+ be*e*(em - ee)
		+ (bi2*i2 + a*i1)*(em - ei2)
		- b*i1*(em - ei1)
}

FUNCTION M() {
	newstates(t - t0)
	M = mnew
}

FUNCTION E() {
	newstates(t - t0)
	E = enew
}

FUNCTION I() {
	newstates(t - t0)
	I = i2new
}

PROCEDURE update() {
	e = enew
	i1 = i1new
	i2 = i2new
	m = mnew
	t0 = t
}

PROCEDURE factors() {
	LOCAL tp
	: force all exponential solution ( no x*exp(-x) )
	: and force assertion for correctness of algorithm
	: i.e. taue is smallest and only one that is excitatory
	if (taue >= taui1) { taui1 = taue + .01 }
	if (taui1 >= taui2) { taui2 = taui1 + .01 }
	if (taui2 >= taum) { taum = taui2 + .01 }

	ke=1/taue  ki1=1/taui1  ki2=1/taui2  km=1/taum

	: normalize so the peak magnitude of m is 1 when single e of 1
	tp = log(km/ke)/(km - ke)
	be = 1/(exp(-km*tp) - exp(-ke*tp))
	ae = be*(ke - km)

	: normalize so the peak magnitude of i2 is 1 when single i of 1
	tp = log(ki2/ki1)/(ki2 - ki1)
	bi1 = 1/(exp(-ki2*tp) - exp(-ki1*tp))
	ai1 = bi1*(ki1 - ki2)

	: normalize so the peak magnitude of m is 1 when single i of 1
	: first set up enough so we can use newstates()
	e = 0
	i1 = 1
	i2 = 0
	m = 0
	bi2 = 1
	ai2 = bi2*(ki2 - km)
	a = bi2*bi1
	b = a*(ki2 - km)/(ki1 - km)
	:find the 0 of dm/dt
	tp = search()
	: now compute normalization factor and reset constants that depend on it
	newstates(tp)
	bi2 = 1/mnew
	ai2 = bi2*(ki2 - km)
	a = bi2*bi1
	b = a*(ki2 - km)/(ki1 - km)
	: now newstates(tp) should set mnew=1
	newstates(tp)
:	printf("INITIAL bi2=%g tp=%g mnew=%g\n", bi2, tp, mnew)
	i1 = 0
}

FUNCTION deriv(d(ms)) (/ms2) { : proportional, not exact derivative
	deriv = -km*(ki1 - ki2)*exp(-km*d)
		+ ki2*(ki1 - km)*exp(-ki2*d)
		- ki1*(ki2 - km)*exp(-ki1*d)
}

FUNCTION search() (ms) {
	LOCAL x, t1, t2
	: should only do this when tau's change
	: chord search
	: left side is 0 without slow km
	t1 = -log(ki1*(ki2 - km)/ki2/(ki1 - km))/(ki1 - ki2)
:printf("search greedy test t1=%g x1=%g\n", t1, deriv(t1))
	if (deriv(t1) < 0) { : but it failed so be conservative
		: and choose the peak location of i2
		t1 = log(ki1/ki2)/(ki1 - ki2)
	}
	: right side is 0 without fast ki1
	t2 = log(km*(ki1 - ki2)/ki2/(ki1 - km))/(km - ki2)
:printf("search  t1=%g x1=%g t2=%g x2=%g\n", t1, deriv(t1), t2, deriv(t2))
	: now do search
	while (t2 - t1 > 1e-6) {
		search = (t1+t2)/2
		x = deriv(search)
:printf("search=%g x=%g\n", search, x)
		if (x > 0) {
			t1 = search
		}else{
			t2 = search
		}
	}			
}

INITIAL {
	factors()
	e = 0
	i1 = 0
	i2 = 0
	m = 0
	t0 = t
	net_send(firetimebound(), 1)

	nself=0
	nexcite=0
	ninhibit=0
}

NET_RECEIVE (w) {
	newstates(t-t0)
	update()	
:printf("event %g t=%g e=%g i1=%g i2=%g m=%g\n", flag, t, e, i1, i2, m)
	if (m > 1-eps) { : time to fire
:printf("fire\n")
		net_event(t)
		m = 0
	}
	if (flag == 1) { :self event
		nself = nself + 1
		net_send(firetimebound(), 1)
	}else{
		if (w > 0) {
			nexcite = nexcite + 1
			e = e + w
		}else{
			ninhibit = ninhibit + 1
			i1 = i1 + w
		}
:printf("w=%g e=%g i1=%g\n", w, e, i1)
		net_move(firetimebound() + t)
	}
}

FUNCTION firetimebound() (ms) {
	LOCAL slope
	slope = -km*m + ae*e + ai2*i2
	if (slope <= 1e-9) {
		firetimebound = 1e9
	}else{
		firetimebound = (1 - m)/slope
	}
}
