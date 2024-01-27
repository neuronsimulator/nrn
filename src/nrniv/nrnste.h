#pragma once
#include "neuron/container/data_handle.hpp"

#include <vector>
// StateTransitionEvent is a finite state machine in which a transtion occurs
// when the transition condition is true. For speed the transition condition
// is of the form *var1 > *var2 and allows second order interpolation.
// Transition conditions are checked only if the source state is the
// current state.

class HocCommand;
struct StateTransitionEvent;
class STECondition;

struct STETransition {
    STETransition(Point_process* pnt);
    void event();  // from STECondition::deliver
    double value() {
        assert(var1_ && var2_);
        return *var1_ - *var2_;
    }
    void activate();    // add ste_ to watch list
    void deactivate();  // remove ste_ from watch list

    neuron::container::data_handle<double> var1_{}, var2_{};
    std::unique_ptr<HocCommand> hc_{};
    StateTransitionEvent* ste_{};
    std::unique_ptr<STECondition> stec_;
    int dest_{};
    bool var1_is_time_{};
};

struct STEState {
    STETransition& add_transition(Point_process* pnt);
    std::vector<STETransition> transitions_;
};

struct StateTransitionEvent {
    StateTransitionEvent(int nstate, Point_process*);
    ~StateTransitionEvent();
    void transition(int src,
                    int dest,
                    neuron::container::data_handle<double> var1,
                    neuron::container::data_handle<double> var2,
                    std::unique_ptr<HocCommand>);
    void state(int i);  // set current state  -- update watch list.
    int state() {
        return istate_;
    }
    int nstate() {
        return states_.size();
    }
    void activate();
    void deactivate();

    int istate_{};
    std::vector<STEState> states_{};
    Point_process* pnt_{};
    int activated_{-1};
};
