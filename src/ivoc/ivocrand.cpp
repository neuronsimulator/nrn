#include <../../nrnconf.h>

// definition of random number generation from the g++ library

#include <stdio.h>
#include <stdlib.h>
#include "random1.h"

#include <InterViews/resource.h>
#include "classreg.h"
#include "oc2iv.h"
#include "nrnisaac.h"

#include <OS/list.h>
#include <ocnotify.h>
#include "ocobserv.h"
#include <nrnran123.h>

#include <RNG.h>
#include <ACG.h>
#include <MLCG.h>
#include <Random.h>
#include <Poisson.h>
#include <Normal.h>
#include <Uniform.h>
#include <Binomial.h>
#include <DiscUnif.h>
#include <Erlang.h>
#include <Geom.h>
#include <LogNorm.h>
#include <NegExp.h>
#include <RndInt.h>
#include <HypGeom.h>
#include <Weibull.h>

#if HAVE_IV
#include "ivoc.h"
#endif

#undef dmaxuint
#define dmaxuint 4294967295.

extern "C" void nrn_random_play();

class RandomPlay : public Observer, public Resource {
public:
	RandomPlay(Rand*, double*);
	virtual ~RandomPlay();
	void play();
	void list_remove();
	virtual void update(Observable*);
private:
	Rand* r_;
	double* px_;
};

declarePtrList(RandomPlayList,RandomPlay)
implementPtrList(RandomPlayList, RandomPlay)
static RandomPlayList* random_play_list_;

extern "C" {
//double nrn_random_pick(Rand* r);
void nrn_random_reset(Rand* r);
//Rand* nrn_random_arg(int);
long nrn_get_random_sequence(Rand* r);
void nrn_set_random_sequence(Rand* r, long seq);
//int nrn_random_isran123(Rand* r, uint32_t* id1, uint32_t* id2, uint32_t* id3);
} // extern "C"

#include <mcran4.h>

class NrnRandom123 : public RNG {
public:
	NrnRandom123(uint32_t id1, uint32_t id2, uint32_t id3 = 0);
	virtual ~NrnRandom123();
	virtual uint32_t asLong() { return nrnran123_ipick(s_); }
	virtual double asDouble() { return nrnran123_dblpick(s_); }
	virtual void reset() { nrnran123_setseq(s_, 0, 0); }
	nrnran123_State* s_;
};
NrnRandom123::NrnRandom123(uint32_t id1, uint32_t id2, uint32_t id3) {
	s_ = nrnran123_newstream3(id1, id2, id3);
}
NrnRandom123::~NrnRandom123() {
	nrnran123_deletestream(s_);
}


// The decision that has to be made is whether each generator instance
// should have its own seed or only one seed for all. We choose separate
// seed for each but if arg not present or 0 then seed chosen by system.

// the addition of ilow > 0 means that value is used for the lowindex
// instead of the mcell_ran4_init global 32 bit lowindex.

class MCellRan4 : public RNG {
public:
	MCellRan4(uint32_t ihigh = 0, uint32_t ilow = 0);
	virtual ~MCellRan4();
	virtual uint32_t asLong() {
		return (uint32_t)(ilow_ == 0 ? mcell_iran4(&ihigh_) :
			nrnRan4int(&ihigh_, ilow_));
	}
	virtual void reset() { ihigh_ = orig_; }
	virtual double asDouble() {
		return (ilow_ == 0 ? mcell_ran4a(&ihigh_) :
			 nrnRan4dbl(&ihigh_, ilow_)); }
	uint32_t ihigh_;
	uint32_t orig_;
	uint32_t ilow_;
private:
	static uint32_t cnt_;
};

MCellRan4::MCellRan4(uint32_t ihigh, uint32_t ilow) {
	++cnt_;
	ilow_ = ilow;
	ihigh_ = ihigh;
	if (ihigh_ == 0) {
		ihigh_ = cnt_;
		ihigh_ = (uint32_t)asLong();
	}
	orig_ = ihigh_;
}
MCellRan4::~MCellRan4() {}

uint32_t MCellRan4::cnt_ = 0;

class Isaac64 : public RNG {
public:
	Isaac64(uint32_t seed = 0);
	virtual ~Isaac64();
	virtual uint32_t asLong() {
		return (uint32_t)nrnisaac_uint32_pick(rng_);
	}
	virtual void reset() { nrnisaac_init(rng_, seed_); }
	virtual double asDouble() { return nrnisaac_dbl_pick(rng_); }
	uint32_t  seed() { return seed_; }
	void seed(uint32_t s) { seed_ = s; reset(); }
private:
	uint32_t seed_;
	void* rng_;
	static uint32_t cnt_;
};

Isaac64::Isaac64(uint32_t seed) {
	if (cnt_ == 0) { cnt_ = 0xffffffff; }
	--cnt_;
	seed_ = seed;
	if (seed_ == 0) {
		seed_ = cnt_;
	}
	rng_ = nrnisaac_new();
	reset();
}
Isaac64::~Isaac64() {
	nrnisaac_delete(rng_);
}

uint32_t Isaac64::cnt_ = 0;

RandomPlay::RandomPlay(Rand* r, double* px) {
//printf("RandomPlay\n");
	r_ = r;
	px_ = px;
	random_play_list_->append(this);
	ref();
	nrn_notify_when_double_freed(px_, this);
	nrn_notify_when_void_freed((void*)r->obj_, this);
}
RandomPlay::~RandomPlay() {
//printf("~RandomPlay\n");
}
void RandomPlay::play() {
//printf("RandomPlay::play\n");
	*px_ = (*(r_->rand))();
}
void RandomPlay::list_remove() {
	long i, cnt = random_play_list_->count();
	for (i = 0; i < cnt; ++i) {
		if (random_play_list_->item(i) == (RandomPlay*)this) {
//printf("RandomPlay %p removed from list cnt=%d i=%d %p\n", this, cnt, i);
			random_play_list_->remove(i);
			unref_deferred();
			break;
		}
	}
}
void RandomPlay::update(Observable*) {
//printf("RandomPlay::update\n");
	nrn_notify_pointer_disconnect(this);
	list_remove();
}

Rand::Rand(unsigned long seed, int size, Object* obj) {
//printf("Rand\n");
   gen = new ACG(seed,size);
   rand = new Normal(0.,1.,gen); 
   type_ = 0;
   obj_ = obj;
}

Rand::~Rand() {
//printf("~Rand\n");
  delete gen;
  delete rand;
}

// constructor for a random number generator based on the RNG class
// from the gnu c++ class library 
// defaults to the ACG generator (see below)

  // syntax:
  // a = new Rand([seed],[size])

static void* r_cons(Object* obj) {
	  unsigned long seed = 0;
	  int size = 55;

	  if (ifarg(1)) seed = long(*getarg(1));
	  if (ifarg(2)) size = int(chkarg(2,7,98));

	Rand* r =  new Rand(seed,size, obj);
	return (void*)r;
}

// destructor -- called when no longer referenced

static void r_destruct(void* r) {
	delete (Rand*)r;
}

// Use a variant of the Linear Congruential Generator (algorithm M)
// described in Knuth, Art of Computer Programming, Vol. III in
// combination with a Fibonacci Additive Congruential Generator.
// This is a "very high quality" random number generator, 
// Default size is 55, giving a size of 1244 bytes to the structure
// minimum size is 7 (total 100 bytes), maximum size is 98 (total 2440 bytes)
  // syntax:
  // r.ACG([seed],[size])

static double r_ACG(void* r)
{
  Rand* x = (Rand*)r;

  unsigned long seed = 0;
  int size = 55;

  if (ifarg(1)) seed = long(*getarg(1));
  if (ifarg(2)) size = int(chkarg(2,7,98));

  x->rand->generator(new ACG(seed,size));
  x->type_ = 0;
  delete x->gen;
  x->gen = x->rand->generator();
  return 1.;
}  

// Use a Multiplicative Linear Congruential Generator.  Not as high
// quality as the ACG, but uses only 8 bytes
  // syntax:
  // r.MLCG([seed1],[seed2])

static double r_MLCG(void* r)
{
  Rand* x = (Rand*)r;

  unsigned long seed1 = 0;
  unsigned long seed2 = 0;

  if (ifarg(1)) seed1 = long(*getarg(1));
  if (ifarg(2)) seed2 = long(*getarg(2));

  x->rand->generator(new MLCG(seed1,seed2));
  delete x->gen;
  x->gen = x->rand->generator();
  x->type_ = 1;
  return 1.;
}

static double r_MCellRan4(void* r) {
  Rand* x = (Rand*)r;

  uint32_t seed1 = 0;
  uint32_t  ilow = 0;

  if (ifarg(1)) seed1 = (uint32_t)(chkarg(1, 0., dmaxuint));
  if (ifarg(2)) ilow = (uint32_t)(chkarg(2, 0., dmaxuint));
  MCellRan4* mcr = new MCellRan4(seed1, ilow);
  x->rand->generator(mcr);
  delete x->gen;
  x->gen = x->rand->generator();
  x->type_ = 2;
  return (double)mcr->orig_;
}

extern "C" long nrn_get_random_sequence(Rand* r) {
	assert(r->type_ == 2);
	MCellRan4* mcr = (MCellRan4*)r->gen;
	return mcr->ihigh_;
}

extern "C" void nrn_set_random_sequence(Rand* r, long seq) {
	assert(r->type_ == 2);
	MCellRan4* mcr = (MCellRan4*)r->gen;
	mcr->ihigh_ = seq;
}

extern "C" int nrn_random_isran123(Rand* r, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
	if (r->type_ == 4) {
		NrnRandom123* nr = (NrnRandom123*)r->gen;
		nrnran123_getids3(nr->s_, id1, id2, id3);
		return 1;
	}
	return 0;
}

static double r_nrnran123(void* r) {
	Rand* x = (Rand*)r;
	uint32_t id1 = 0;
	uint32_t id2 = 0;
	uint32_t id3 = 0;
	if (ifarg(1)) id1 = (uint32_t)(chkarg(1, 0., dmaxuint));
	if (ifarg(2)) id2 = (uint32_t)(chkarg(2, 0., dmaxuint));
	if (ifarg(3)) id3 = (uint32_t)(chkarg(3, 0., dmaxuint));
	NrnRandom123* r123 = new NrnRandom123(id1, id2, id3);
	x->rand->generator(r123);
	delete x->gen;
	x->gen = x->rand->generator();
	x->type_ = 4;
	return 0.;
}

static double r_ran123_globalindex(void* r) {
	if (ifarg(1)) {
		uint32_t gix = (uint32_t)chkarg(1, 0., 4294967295.); /* 2^32 - 1 */
		nrnran123_set_globalindex(gix);
	}
	return (double)nrnran123_get_globalindex();
}

static double r_sequence(void* r) {
	Rand* x = (Rand*)r;
	if (x->type_ != 2 && x->type_ != 4) {
hoc_execerror("Random.seq() can only be used if the random generator was MCellRan4 or Random123", 0);
	}

	if (x->type_ == 4) {
		uint32_t seq; char which;
		if (ifarg(1)) {
			double s = chkarg(1, 0., 17179869183.); /* 2^34 - 1 */
			seq = (uint32_t)(s/4.);
			which = char(s - seq*4.);
			NrnRandom123* nr = (NrnRandom123*)x->gen;
			nrnran123_setseq(nr->s_, seq, which);
		}			
		nrnran123_getseq(((NrnRandom123*)x->gen)->s_, &seq, &which);
		return double(seq)*4. + double(which);
	}

	MCellRan4* mcr = (MCellRan4*)x->gen;
	if (ifarg(1)) {
		mcr->ihigh_ = (long)(*getarg(1));
	}
	return (double)mcr->ihigh_;
}

static double r_Isaac64(void* r) {
  Rand* x = (Rand*)r;

  uint32_t seed1 = 0;

  if (ifarg(1)) seed1 = (uint32_t)(*getarg(1));
  Isaac64* mcr = new Isaac64(seed1);
  x->rand->generator(mcr);
  delete x->gen;
  x->gen = x->rand->generator();
  x->type_ = 3;
  return (double)mcr->seed();
}

// Pick again from the distribution last used
  // syntax:
  // r.repick()
static double r_repick(void* r)
{
  Rand* x = (Rand*)r;
  return (*(x->rand))();
}

extern "C" double nrn_random_pick(Rand* r) {
	if (r) {
		return (*(r->rand))();
	}else{
		return .5;
	}
}

extern "C" void nrn_random_reset(Rand* r) {
	if(r) {
		r->gen->reset();
	}
}

extern "C" Rand* nrn_random_arg(int i) {
	Object* ob = *hoc_objgetarg(i);
	check_obj_type(ob, "Random");
	Rand* r = (Rand*)(ob->u.this_pointer);
	return r;
}

// uniform random variable over the open interval [low...high)
// syntax:
//     r.uniform(low,high)

static double r_uniform(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new Uniform(a1, a2, x->gen);
  return (*(x->rand))();
}

// uniform random variable over the closed interval [low...high]
// syntax:
//     r.discunif(low,high)

static double r_discunif(void* r)
{
  Rand* x = (Rand*)r;
  long a1 = long(*getarg(1));
  long a2 = long(*getarg(2));
  delete x->rand;
  x->rand = new DiscreteUniform(a1, a2, x->gen);
  return (*(x->rand))();
}

  
// normal (gaussian) distribution 
// syntax:
//     r.normal(mean,variance)

static double r_normal(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new Normal(a1, a2, x->gen);
  return (*(x->rand))();
}


// logarithmic normal distribution
// syntax:
//     r.lognormal(mean)

static double r_lognormal(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new LogNormal(a1, a2, x->gen);
  return (*(x->rand))();
}


// poisson distribution 
// syntax:
//   r.poisson(mean)

static double r_poisson(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  delete x->rand;
  x->rand = new Poisson(a1, x->gen);
  return (*(x->rand))();
}


// binomial distribution, which models successfully drawing items from a pool
// n is the number items in the pool and p is the probablity of each item
// being successfully drawn (n>0, 0<=p<=1)
// syntax:
//     r.binomial(n,p)

static double r_binomial(void* r)
{
  Rand* x = (Rand*)r;
  int a1 = int(chkarg(1, 0, 1e99));
  double a2 = chkarg(2, 0, 1);
  delete x->rand;
  x->rand = new Binomial(a1, a2, x->gen);
  return (*(x->rand))();
}


// discrete geometric distribution
// Given 0<=mean<=1, returns the number of uniform random samples
// that were drawn before the sample was larger than mean (always
// greater than 0.
// syntax:
//     r.geometric(mean)

static double r_geometric(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = chkarg(1, 0, 1);
  delete x->rand;
  x->rand = new Geometric(a1, x->gen);
  return (*(x->rand))();
}



// hypergeometric distribution
// syntax:
//     r.hypergeo(mean,variance)

static double r_hypergeo(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new HyperGeometric(a1, a2, x->gen);
  return (*(x->rand))();
}


// negative exponential distribution
// syntax:
//     r.negexp(mean)

static double r_negexp(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  delete x->rand;
  x->rand = new NegativeExpntl(a1, x->gen);
  return (*(x->rand))();
}



// Erlang distribution
// syntax:
//     r.erlang(mean,variance)

static double r_erlang(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new Erlang(a1, a2, x->gen);
  return (*(x->rand))();
}


// Weibull distribution
// syntax:
//     r.weibull(alpha,beta)

static double r_weibull(void* r)
{
  Rand* x = (Rand*)r;
  double a1 = *getarg(1);
  double a2 = *getarg(2);
  delete x->rand;
  x->rand = new Weibull(a1, a2, x->gen);
  return (*(x->rand))();
}

static double r_play(void* r){
	new RandomPlay((Rand*)r, hoc_pgetarg(1));
	return 0.;
}

extern "C" void nrn_random_play() {
	long i, cnt = random_play_list_->count();
	for (i=0; i < cnt; ++i) {
		random_play_list_->item(i)->play();
	}
}


static Member_func r_members[] = {
        "ACG",              r_ACG,
	"MLCG",             r_MLCG,
	"Isaac64",	    r_Isaac64,
	"MCellRan4",	    r_MCellRan4,
	"Random123",	    r_nrnran123,
	"Random123_globalindex", r_ran123_globalindex,
	"seq",		    r_sequence,
	"repick",           r_repick,
	"uniform",          r_uniform,
	"discunif",         r_discunif,
	"normal",           r_normal,
	"lognormal",        r_lognormal,
	"binomial",         r_binomial,
	"poisson",          r_poisson,
	"geometric",        r_geometric,
	"hypergeo",         r_hypergeo,
	"negexp",           r_negexp,
	"erlang",           r_erlang,
	"weibull",          r_weibull,
	"play",		r_play,
 	0, 0
};

void Random_reg() {
        class2oc("Random", r_cons, r_destruct, r_members, NULL, NULL, NULL);
	random_play_list_ = new RandomPlayList();
}
