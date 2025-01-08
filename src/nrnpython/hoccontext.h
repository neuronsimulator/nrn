#pragma once

extern Object* hoc_thisobject;
extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_symlist;

struct HocContext {
    Object* obj;
    Objectdata* obd;
    Symlist* sl;
};

static HocContext* hc_save_and_set_to_top_(HocContext* hc) {
    hc->obj = hoc_thisobject;
    hc->obd = hoc_objectdata;
    hc->sl = hoc_symlist;
    hoc_thisobject = 0;
    hoc_objectdata = hoc_top_level_data;
    hoc_symlist = hoc_top_level_symlist;
    return hc;
}
static void hc_restore_(HocContext* hc) {
    hoc_thisobject = hc->obj;
    hoc_objectdata = hc->obd;
    hoc_symlist = hc->sl;
}

// RAII guard for the top HOC context
class HocTopContextManager {
  private:
    HocContext hcref;
    HocContext* hc_ = nullptr;

  public:
    HocTopContextManager() {
        // ``hoc_thisobject`` is global
        if (hoc_thisobject) {
            hc_ = hc_save_and_set_to_top_(&hcref);
        }
    }
    ~HocTopContextManager() {
        if (hc_) {
            hc_restore_(hc_);
        }
    }
};
