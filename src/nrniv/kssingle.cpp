#include <../../nrnconf.h>
#include <string.h>
#include <stdlib.h>
#include "nrnoc2iv.h"
#include "kschan.h"
#include "kssingle.h"

// needed for DiscreteEvent aspect
#include "netcvode.h"
#include "cvodeobj.h"

//----------------
// KSSingleTrans, KSSingleState is only apparently redundant with KSTrans
// and KSState. i.e. they are identical only if KSChan has only a single
// gating complex with a power of 1. If there are G gating complexes with
// powers pg and numbers of states sg (where g ranges from 0 to G-1) then
// the number of distinct states for the gate g is the binomial factor
// nsg = (sg + pg - 1, pg). i.e. (sg + pg - 1)! / pg! / (sg - 1)!
// and the total number of distinct states is the product of the nsg.
// consider m^3 * h, and suppose m is open and M is closed then there
// are three ways to go from mmmh to Mmmh and one way to go to mmmH. i.e
// MMMH <-> mMMH (3am, bm)   MMMH <-> MMMh (ah, bh)   MMMh <-> mMMh (3am, bm)
// mMMH <-> mmMH (2am, 2bm)  mMMH <-> mMMh (ah, bh)   mMMh <-> mmMh (2am, 2bm)
// mmMH <-> mmmH (am, 3bm)   mmMH <-> mmMh (ah, bh)   mmMh <-> mmmh (am, 3bm)
// there are 8 states and 18 transitions.
//
// Following a single particle as it moves from state to state is
// the simplest case. It suffices to keep track of the state index
// and, following ...
// use two random numbers. One random number is transformed into
// a negative exponential distribution
// and defines the time interval to the next transition. The rate is the
// sum of all the possible transition rates. The other is uniformly
// distibuted between 0 and 1 and is used to determine which transition
// branch was taken according to the
//
// For the case of N particles distributed among the states we use
// an integer array in which an element is the integer population of the
// corresponding state. Too bad we lose the efficiency of the implicit handling
// of 0 population states but I don't see how it can be helped.
// Again we use two random numbers. The transition rate of some event, i.e.
// some particle moves to another state, is the sum over all states of the
// state population times the sum of all the possible transition rates for
// that state. And, again, the second random number defines the branch taken.
// Note that this is still an exact stochastic simulation in that only
// one thing happens at a time.
//
// It remains to discuss how things are affected by changing rates.
// We make the approximation that rates are constant within a dt. In
// consequence we only need to compute the rate information once per dt
// (at least for the N particle case) no matter how many transitions have
// to be followed til the last one is beyond the t0+dt point. The question
// is what to do with that last interval which remains unused, ie. time
// has not reached the "transition time"
// Also we'd like to be efficient when the average interval is much greater
// than dt.
// The simplest implementation is to just discard the random numbers and
// assert that the time is t0 <- t0+dt and compute from that point
// at the next solve step. The alternative is to hold the last random
// pair and recalculate the interval (assuming the rate has changed)
// to obtain (from the last actual transition time) a new transition time.
// and see if it is now less than the new t0 + dt. Note that there is
// some extra error here because of the assumption of constant rate over the
// last entire random interval and not just the last dt.
// Note that we do not have to worry about this (or at least worry less)
// in voltage clamp context as well as in contexts in which intervals are
// seldom > dt. However no analysis has been done and I can't demonstrate
// that the statistical error of following a single channel
// during an action potential is not substantial. We compromise and
// test the voltage. If it has changed then we reset the transition time
// to t0 and repick random numbers. If it has not changed significantly,
// then we keep the last random interval and use that when t0+dt passes the
// up to then unexecuted transition.
//----------------
// The above broad design envisioned a parallel structure involving
// a single kinetic scheme isomorphic to the user level spec which
// may have multiple gates raised to powers. The initial implementation
// for the KSSingle constructor, however, for simplicity assumed that the
// user level spec was only one ks gate complex with power 1. In any case
// the question of state read out and conductance calculation arises
// and in either case it seems to make most sense that the standard
// state array in the prop->param array be used. With the current
// limited implementation, KSSingleNodeData.statelist_ is conceptually
// identical to the prop->param states. In the envisioned full implementation
// the least violent possibility is to keep the user level spec as much
// as possible but to expand the prop->param states to correspond
// to single channel requirements. Another possibility which does a
// lot more violence to the user spec but makes the current implementation
// almost complete is to replace the user spec with a single channel
// adequate ks scheme. Merely stating it is sufficient to reject it.
// A third possiblity is to not use the prop->param states at all since
// their user level spec does not correspond to the ks single channel
// states. But this makes GUI interface not re-usable (e.g. plotting
// a state) and the question of determining states values at the user
// level remains.  The last possibility is to not allow single channel
// mode unless there is only one ks gate complex raised to a single power
// and coelesce the redundant structures.
// So it seems that the best extension so far is to
// keep the user level spec in KSChan but when in single channel mode,
// define the prop->param state array to correspond to single channel
// states. Since we are going that far we might as well go the whole
// way and also introduce a new Nsingle PARAMETER and make the
// dparam[2]._pvoid == KSSingleNodeData existent only in single channel
// mode. Note that since KSChan.nstate_ <= KSSingle.nstate_, initialization
// can fill the KSChan interpreted prop->param states with the fractions,
// and then, per normal, the prop->param states interpreted at KSSingle
// states get filled with the right integers. We'll also need a naming
// convention for KSSingle states since that is how they get accessed at
// the hoc level. We need names for the hh open and closed state
// if the hh state name is "a". We need names generated for powers.
// Products are easy, just concatenate the names. For powers of hh states we
// use the hh state name followed by the number that are open. eg an m^3
// gate gets the names m0, m1, m2, m3. Thus m^3*h has the names
// m0h0 m1h0 m2h0 m3h0 m0h1 m1h1 m2h1 m3h1 (m3h1 is the only open state)
// Kind of makes mh[4][2] attractive.
// The combinatorics for ks gates raised to a power
// is more complex and it is not clear that that is so useful. I think
// it makes sense to ignore that and use array indices only for hh gates
// raised to a power. Thus a 3 state gate multiplied by an h hh gate would
// have the names c0h[2], c1h[2], oh[2]
// The nice aspect of this is that the current implementation can be
// easily transformed since the transformation of prop->param states can
// be deferred to later. i.e there seems to be a nice incremental path
// to a full implementation without removing at one step what was added
// at the previous step. The first step is to replace the not always used
// int* KSSingleNodeData.statelist_ with an always used
// double* KSSingleNodeData.statepop_ which points into the prop->param.
//----------------

extern NetCvode* net_cvode_instance;

double KSSingle::vres_;
unsigned int KSSingle::idum_;

KSSingle::KSSingle(KSChan* c) {
    // implemenation assumes one ks gate complex with power 1
    int i;
    sndindex_ = 2;
    nstate_ = c->nstate_;
    states_ = new KSSingleState[nstate_];
    ntrans_ = 2 * c->ntrans_;
    transitions_ = new KSSingleTrans[ntrans_];
    rval_ = new double[std::max(ntrans_, nstate_)];
    uses_ligands_ = false;
    for (i = 0; i < c->ntrans_; ++i) {
        {
            KSSingleTrans& tf = transitions_[2 * i];
            tf.kst_ = c->trans_ + i;
            if (tf.kst_->type_ >= 2) {
                uses_ligands_ = true;
            }
            tf.f_ = true;
            tf.fac_ = 1.;
            tf.src_ = tf.kst_->src_;
            tf.target_ = tf.kst_->target_;
        }
        {
            KSSingleTrans& tf = transitions_[2 * i + 1];
            tf.kst_ = c->trans_ + i;
            tf.f_ = false;
            tf.fac_ = 1.;
            tf.src_ = tf.kst_->target_;
            tf.target_ = tf.kst_->src_;
        }
    }
    // The transition list for each source state is required for
    // N=1 single channels.
    // count first
    for (i = 0; i < ntrans_; ++i) {
        states_[transitions_[i].src_].ntrans_++;
    }
    // allocate and reset count
    for (i = 0; i < nstate_; ++i) {
        KSSingleState& ss = states_[i];
        ss.transitions_ = new int[ss.ntrans_];
        ss.ntrans_ = 0;
    }
    // link
    for (i = 0; i < ntrans_; ++i) {
        KSSingleState& ss = states_[transitions_[i].src_];
        ss.transitions_[ss.ntrans_] = i;
        ++ss.ntrans_;
    }
}

KSSingle::~KSSingle() {
    delete[] states_;
    delete[] transitions_;
    delete[] rval_;
}

KSSingleState::KSSingleState() {
    ntrans_ = 0;
    transitions_ = NULL;
}
KSSingleState::~KSSingleState() {
    if (transitions_) {
        delete[] transitions_;
    }
}

KSSingleTrans::KSSingleTrans() {}
KSSingleTrans::~KSSingleTrans() {}
double KSSingleTrans::rate(Point_process* pnt) {
    if (kst_->type_ < 2) {
        return rate(NODEV(pnt->node));
    } else {
        return rate(pnt->prop->dparam);
    }
}

KSSingleNodeData::KSSingleNodeData() {
    nsingle_ = 1;
}

KSSingleNodeData::~KSSingleNodeData() {}

void KSSingleNodeData::deliver(double tt, NetCvode* nc, NrnThread* nt) {
    ++KSSingle::singleevent_deliver_;
    Cvode* cv = (Cvode*) ((*ppnt_)->nvi_);
    if (cv) {
        nc->retreat(tt, cv);
        cv->set_init_flag();
    }
    assert(nt->_t == tt);
    vlast_ = NODEV((*ppnt_)->node);
    if (nsingle_ == 1) {
        kss_->do1trans(this);
    } else {
        kss_->doNtrans(this);
    }
    qi_ = nc->event(t1_, this, nt);
}

void KSSingleNodeData::pr(const char* s, double tt, NetCvode* nc) {
    Printf("%s %s %.15g\n", s, hoc_object_name((*ppnt_)->ob), tt);
}

void KSSingle::state(Node* nd, Datum* pd, NrnThread* nt) {
    // integrate from t-dt to t
    int i;
    double v = NODEV(nd);
    auto* snd = pd[sndindex_].get<KSSingleNodeData*>();
    // if truly single channel, as opposed to N single channels
    // then follow the one populated state. Otherwise do the
    // general case
    if (snd->nsingle_ == 1) {
        one(v, snd, nt);
    } else {
        multi(v, snd, nt);
    }
}

void KSSingle::cv_update(Node* nd, Datum* pd, NrnThread* nt) {
    // if v changed then need to move the outstanding
    // single channel event time to a recalculated time
    int i;
    double v = NODEV(nd);
    auto* snd = pd[sndindex_].get<KSSingleNodeData*>();
    if (uses_ligands_ || !vsame(v, snd->vlast_)) {
        assert(nt->_t < snd->t1_);
        snd->vlast_ = v;
        snd->t0_ = nt->_t;
        if (snd->nsingle_ == 1) {
            next1trans(snd);
        } else {
            nextNtrans(snd);
        }
        net_cvode_instance->move_event(snd->qi_, snd->t1_, nt);
        ++KSSingle::singleevent_move_;
    }
}

int KSSingle::rvalrand(int n) {
    int i;
    --n;
    double x = unifrand(rval_[n]);
    for (i = 0; i < n; ++i) {
        if (rval_[i] >= x) {
            return i;
        }
    }
    return n;
}

void KSSingle::one(double v, KSSingleNodeData* snd, NrnThread* nt) {
    if (uses_ligands_ || !vsame(v, snd->vlast_)) {
        snd->vlast_ = v;
        snd->t0_ = nt->_t - nt->_dt;
        next1trans(snd);
    }
    while (snd->t1_ <= nt->_t) {
        snd->vlast_ = v;
        do1trans(snd);
    }
}

void KSSingle::do1trans(KSSingleNodeData* snd) {
    snd->t0_ = snd->t1_;
    // printf("KSSingle::do1trans t1=%g old=%d ", snd->t1_, snd->filledstate_);
    snd->statepop(snd->filledstate_) = 0.;
    snd->filledstate_ = transitions_[snd->next_trans_].target_;
    snd->statepop(snd->filledstate_) = 1.;
    // printf("new=%d \n", snd->filledstate_);
    next1trans(snd);
}

void KSSingle::next1trans(KSSingleNodeData* snd) {
    int i;
    KSSingleState* ss = states_ + snd->filledstate_;
    double x = 0;
    for (i = 0; i < ss->ntrans_; ++i) {
        KSSingleTrans* st = transitions_ + ss->transitions_[i];
        x += st->rate(*snd->ppnt_);
        rval_[i] = x;
    }
    if (x > 1e-9) {
        snd->t1_ = exprand() / x + snd->t0_;
        snd->next_trans_ = ss->transitions_[rvalrand(ss->ntrans_)];
    } else {
        snd->t1_ = 1e9 + snd->t0_;
        snd->next_trans_ = ss->transitions_[0];
    }
    // printf("KSSingle::next1trans v=%g t1_=%g %d (%d, %d)\n", snd->vlast_, snd->t1_,
    // snd->next_trans_, transitions_[snd->next_trans_].src_,
    // transitions_[snd->next_trans_].target_);
}
void KSSingle::multi(double v, KSSingleNodeData* snd, NrnThread* nt) {
    if (uses_ligands_ || !vsame(v, snd->vlast_)) {
        snd->vlast_ = v;
        snd->t0_ = nt->_t - nt->_dt;
        nextNtrans(snd);
    }
    while (snd->t1_ <= nt->_t) {
        snd->vlast_ = v;
        doNtrans(snd);
    }
}

void KSSingle::doNtrans(KSSingleNodeData* snd) {
    snd->t0_ = snd->t1_;
    KSSingleTrans* st = transitions_ + snd->next_trans_;
    assert(snd->statepop(st->src_) >= 1.);
    --snd->statepop(st->src_);
    ++snd->statepop(st->target_);
    // printf("KSSingle::doNtrans t1=%g %d with %g -> %d with %g\n", snd->t1_,
    // st->src_, snd->statepop(st->src_), st->target_, snd->statepop(st->target_));
    nextNtrans(snd);
}

void KSSingle::nextNtrans(KSSingleNodeData* snd) {
    int i;
    double x = 0;
    for (i = 0; i < ntrans_; ++i) {
        KSSingleTrans* st = transitions_ + i;
        x += snd->statepop(st->src_) * st->rate(*snd->ppnt_);
        rval_[i] = x;
    }
    if (x > 1e-9) {
        snd->t1_ = exprand() / x + snd->t0_;
        snd->next_trans_ = rvalrand(ntrans_);
    } else {
        snd->t1_ = 1e9 + snd->t0_;
        snd->next_trans_ = 0;
    }
}

void KSSingle::alloc(Prop* p, int sindex) {  // and discard old if not NULL
    auto* snd = p->dparam[2].get<KSSingleNodeData*>();
    if (snd) {
        delete snd;
    }
    snd = new KSSingleNodeData();
    snd->kss_ = this;
    snd->ppnt_ = &(p->dparam[1].literal_value<Point_process*>());
    p->dparam[2] = snd;
    snd->prop_ = p;
    snd->statepop_offset_ = sindex;
}

void KSSingle::init(double v,
                    KSSingleNodeData* snd,
                    NrnThread* nt,
                    Memb_list* ml,
                    std::size_t instance,
                    std::size_t offset) {
    // assuming 1-1 correspondence between KSChan and KSSingle states
    // place steady state population intervals end to end
    int i;
    double x = 0;
    snd->qi_ = NULL;
    snd->t0_ = nt->_t;
    snd->vlast_ = v;
    for (i = 0; i < nstate_; ++i) {
        x += ml->data(instance, offset + i);
        rval_[i] = x;
    }
    // initialization of complex kinetic schemes often not accurate to 9 decimal places
    //	assert(Math::equal(rval_[nstate_ - 1], 1., 1e-9));
    for (i = 0; i < nstate_; ++i) {
        snd->statepop(i) = 0;
    }
    if (snd->nsingle_ == 1) {
        snd->filledstate_ = rvalrand(nstate_);
        ++snd->statepop(snd->filledstate_);
        next1trans(snd);
    } else {
        for (i = 0; i < snd->nsingle_; ++i) {
            ++snd->statepop(rvalrand(nstate_));
        }
        nextNtrans(snd);
        // for (i=0; i < nstate_; ++i) {
        //	printf("  state %d pop %g\n", i, snd->statepop(i));
        //}
    }
    if (cvode_active_) {
        snd->qi_ = net_cvode_instance->event(snd->t1_, snd, nrn_threads);
    }
}

void KSChan::nsingle(Point_process* pp, int n) {
    if (auto* snd = pp->prop->dparam[2].get<KSSingleNodeData*>(); snd) {
        snd->nsingle_ = n;
    }
}
int KSChan::nsingle(Point_process* pp) {
    if (auto* snd = pp->prop->dparam[2].get<KSSingleNodeData*>(); snd) {
        return snd->nsingle_;
    } else {
        return 1000000000;
    }
}
