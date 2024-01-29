#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include <string.h>
#include <nrnoc2iv.h>
#include <nrniv_mf.h>
#include <classreg.h>
#include <objcmd.h>
#include <nrnste.h>
#include <netcon.h>

extern int hoc_return_type_code;

static double ste_transition(void* v) {
    auto* const ste = static_cast<StateTransitionEvent*>(v);
    int src = (int) chkarg(1, 0, ste->nstate() - 1);
    int dest = (int) chkarg(2, 0, ste->nstate() - 1);
    auto var1 = hoc_hgetarg<double>(3);
    auto var2 = hoc_hgetarg<double>(4);
    std::unique_ptr<HocCommand> hc{};
    if (ifarg(5)) {
        Object* obj = NULL;
        if (hoc_is_str_arg(5)) {
            char* stmt = NULL;
            stmt = gargstr(5);
            if (ifarg(6)) {
                obj = *hoc_objgetarg(6);
            }
            hc = std::make_unique<HocCommand>(stmt, obj);
        } else {
            obj = *hoc_objgetarg(5);
            hc = std::make_unique<HocCommand>(obj);
        }
    }
    ste->transition(src, dest, std::move(var1), std::move(var2), std::move(hc));
    return 1.;
}

static double ste_state(void* v) {
    StateTransitionEvent* ste = (StateTransitionEvent*) v;
    hoc_return_type_code = 1;  // integer
    int state = ste->state();
    if (ifarg(1)) {
        ste->state((int) chkarg(1, 0, ste->nstate() - 1));
    }
    return (double) state;
}

static Member_func members[] = {{"transition", ste_transition}, {"state", ste_state}, {0, 0}};

static void* ste_cons(Object*) {
    int nstate = (int) chkarg(1, 1, 1e6);
    Point_process* pnt = NULL;
    if (ifarg(2)) {
        Object* obj = *hoc_objgetarg(2);
        pnt = ob2pntproc(obj);
    }
    StateTransitionEvent* ste = new StateTransitionEvent(nstate, pnt);
    return ste;
}

static void ste_destruct(void* v) {
    StateTransitionEvent* ste = (StateTransitionEvent*) v;
    delete ste;
}

void StateTransitionEvent_reg() {
    class2oc("StateTransitionEvent", ste_cons, ste_destruct, members, NULL, NULL, NULL);
}

StateTransitionEvent::StateTransitionEvent(int nstate, Point_process* pnt)
    : pnt_{pnt} {
    states_.resize(nstate);
}

StateTransitionEvent::~StateTransitionEvent() {
    deactivate();
}

void StateTransitionEvent::deactivate() {
    if (activated_ < 0) {
        return;
    }
    STEState& s = states_[activated_];
    for (auto& st: s.transitions_) {
        st.deactivate();
    }
    activated_ = -1;
}

void StateTransitionEvent::activate() {
    if (activated_ >= 0) {
        deactivate();
    }
    STEState& s = states_[istate_];
    for (auto& st: s.transitions_) {
        st.activate();
    }
    activated_ = istate_;
}

void StateTransitionEvent::state(int ist) {
    assert(ist >= 0 && ist < states_.size());
    deactivate();
    istate_ = ist;
    activate();
}

STETransition::STETransition(Point_process* pnt)
    : stec_{std::make_unique<STECondition>(pnt, nullptr)} {}

STETransition& STEState::add_transition(Point_process* pnt) {
    transitions_.emplace_back(pnt);
    // update in case of reallocation of transitions_
    for (auto& st: transitions_) {
        st.stec_->stet_ = &st;
    }
    return transitions_.back();
}

void STETransition::event() {
    ste_->deactivate();
    ste_->istate_ = dest_;
    if (hc_) {
        nrn_hoc_lock();
        hc_->execute();
        nrn_hoc_unlock();
    }
    ste_->activate();
}
