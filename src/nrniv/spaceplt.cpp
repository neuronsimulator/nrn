#include <../../nrnconf.h>
#include <stdio.h>
#include "classreg.h"


#include <vector>
#include <string.h>
#if HAVE_IV
#include "graph.h"
#include "scenepic.h"
#include "utility.h"
#include "ivoc.h"
#endif
#include "ivocvect.h"
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "objcmd.h"

extern int nrn_multisplit_active_;
extern int hoc_execerror_messages;
extern int node_index(Section*, double);
extern int nrn_shape_changed_;
extern int hoc_return_type_code;
Object* (*nrnpy_rvp_rxd_to_callable)(Object*) = 0;

class SecPos {
  public:
    float x;
    float len;
    Section* sec;
};

using SecPosList = std::vector<SecPos>;

class RangeExpr {
  public:
    RangeExpr(const char* expr, Object* pyobj, SecPosList*);
    virtual ~RangeExpr();
    void fill();
    void compute();
    bool exists(int);
    double* pval(int);

  private:
    long n_;
    SecPosList* spl_;
    double* val_;
    bool* exist_;
    HocCommand* cmd_;
};

#if !HAVE_IV
struct NoIVGraphVector {
    NoIVGraphVector(const char* /* name */) {}
    virtual ~NoIVGraphVector() {}
    void begin();
    void add(float, neuron::container::data_handle<double>);
    int count();
    std::vector<float> x_{};
    std::vector<neuron::container::data_handle<double>> py_{};
};
int NoIVGraphVector::count() {
    auto const s = x_.size();
    assert(s == py_.size());
    return s;
}
void NoIVGraphVector::begin() {
    x_.clear();
    py_.clear();
    x_.reserve(20);
    py_.reserve(20);
}
void NoIVGraphVector::add(float x, neuron::container::data_handle<double> y) {
    x_.push_back(x);
    py_.push_back(std::move(y));
}
#endif

#if HAVE_IV
class RangeVarPlot: public GraphVector {
#else
class RangeVarPlot: public NoIVGraphVector {
#endif
  public:
    RangeVarPlot(const char*, Object* pyobj);
    virtual ~RangeVarPlot();
#if HAVE_IV
    virtual void save(std::ostream&);
    virtual void request(Requisition& req) const;
    virtual bool choose_sym(Graph*);
    virtual void update(Observable*);
#endif
    void x_begin(float, Section*);
    void x_end(float, Section*);
    void origin(float);
    double d2root();
    float left();
    float right();
    void list(Object*);
    void compute();
    int get_color(void);
    void set_color(int);

  private:
    int color_;
    void set_x();
    void fill_pointers();

  private:
    RangeExpr* rexp_;
    Section *begin_section_, *end_section_;
    float x_begin_, x_end_, origin_;
    SecPosList* sec_list_;
    std::string expr_;
    int shape_changed_;
    int struc_changed_;
    double d2root_;  // distance to root of closest point to root
};

static double s_begin(void* v) {
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    ((RangeVarPlot*) v)->x_begin(x, sec);
    return 1.;
}

static double s_end(void* v) {
    double x;
    Section* sec;
    nrn_seg_or_x_arg(1, &sec, &x);
    ((RangeVarPlot*) v)->x_end(x, sec);
    return 1.;
}

static double s_origin(void* v) {
    ((RangeVarPlot*) v)->origin(*getarg(1));
    return 1.;
}

static double s_d2root(void* v) {
    return ((RangeVarPlot*) v)->d2root();
    return 0.0;
}

static double s_left(void* v) {
    return ((RangeVarPlot*) v)->left();
    return 0.0;
}

static double s_right(void* v) {
    return ((RangeVarPlot*) v)->right();
    return 0.0;
}

static double s_list(void* v) {
    Object* ob = *hoc_objgetarg(1);
    check_obj_type(ob, "SectionList");
    ((RangeVarPlot*) v)->list(ob);
    return 0.;
}

static double s_color(void* v) {
    RangeVarPlot* me = (RangeVarPlot*) v;
    hoc_return_type_code = 1;  // integer
    int old_color = me->get_color();
    if (ifarg(1)) {
        me->set_color((int) chkarg(1, 0, 100));
    }
    return old_color;
}


static long to_vector_helper(RangeVarPlot* rvp, Vect* y) {
#if HAVE_IV
    long i, cnt = rvp->py_data()->count();
#else
    long i, cnt = rvp->count();
#endif
    rvp->compute();
    y->resize(cnt);
    for (i = 0; i < cnt; ++i) {
#if HAVE_IV
        y->elem(i) = *rvp->py_data()->p(i);
#else
        if (rvp->py_[i]) {
            y->elem(i) = *rvp->py_[i];
        } else {
            y->elem(i) = 0.0;
        }
#endif
    }
    return cnt;
}

static Object** rvp_vector(void* v) {
    if (ifarg(1)) {
        hoc_execerror("Too many arguments",
                      "RangeVarPlot.vector takes no arguments; were you thinking of .to_vector?");
    }
    Vect* y = new Vect(0);
    RangeVarPlot* rvp = (RangeVarPlot*) v;
    to_vector_helper(rvp, y);
    return y->temp_objvar();
}

static double to_vector(void* v) {
    if (ifarg(3)) {
        hoc_execerror("Too many arguments", "RangeVarPlot.to_vector takes 1 or 2 arguments.");
    }
    long i;
    RangeVarPlot* rvp = (RangeVarPlot*) v;
    Vect* y = vector_arg(1);
    long cnt = to_vector_helper(rvp, y);
    if (ifarg(2)) {
        Vect* x = vector_arg(2);
        x->resize(cnt);
        for (i = 0; i < cnt; ++i) {
#if HAVE_IV
            x->elem(i) = rvp->x_data()->get_val(i);
#else
            x->elem(i) = rvp->x_[i];
#endif
        }
    }
    return double(cnt);
}

static double from_vector(void* v) {
    RangeVarPlot* rvp = (RangeVarPlot*) v;
    Vect* y = vector_arg(1);
#if HAVE_IV
    long i, cnt = rvp->py_data()->count();
    for (i = 0; i < cnt; ++i) {
        *rvp->py_data()->p(i) = y->elem(i);
    }
#else
    long i, cnt = rvp->count();
    for (i = 0; i < cnt; ++i) {
        if (rvp->py_[i]) {
            *rvp->py_[i] = y->elem(i);
        }
    }
#endif
    return double(cnt);
}

static Member_func s_members[] = {{"begin", s_begin},
                                  {"end", s_end},
                                  {"origin", s_origin},
                                  {"d2root", s_d2root},
                                  {"left", s_left},
                                  {"right", s_right},
                                  {"list", s_list},
                                  {"color", s_color},
                                  {"to_vector", to_vector},
                                  {"from_vector", from_vector},
                                  {0, 0}};

static Member_ret_obj_func rvp_retobj_members[] = {{"vector", rvp_vector}, {0, 0}};

static void* s_cons(Object*) {
    char* var = NULL;
    Object* pyobj = NULL;
    Section* sec;
    double x;
    if (hoc_is_str_arg(1)) {
        var = gargstr(1);
    } else {
        pyobj = *hoc_objgetarg(1);
    }
    RangeVarPlot* s = new RangeVarPlot(var, pyobj);
#if HAVE_IV
    s->ref();
#endif
    if (ifarg(2)) {
        nrn_seg_or_x_arg(2, &sec, &x);
        s->x_begin(x, sec);
    }
    if (ifarg(3)) {
        nrn_seg_or_x_arg(3, &sec, &x);
        s->x_end(x, sec);
    }
    return (void*) s;
}

static void s_destruct(void* v) {
#if HAVE_IV
    Resource::unref((RangeVarPlot*) v);
#endif
}

void RangeVarPlot_reg() {
    // printf("RangeVarPlot_reg\n");
    class2oc("RangeVarPlot", s_cons, s_destruct, s_members, NULL, rvp_retobj_members, NULL);
}

#if HAVE_IV
RangeVarPlot::RangeVarPlot(const char* var, Object* pyobj)
    : GraphVector(var ? var : "pyobj") {
#else
RangeVarPlot::RangeVarPlot(const char* var, Object* pyobj)
    : NoIVGraphVector(var ? var : "pyobj") {
#endif
    color_ = 1;
    begin_section_ = 0;
    end_section_ = 0;
    sec_list_ = new SecPosList;
    struc_changed_ = structure_change_cnt;
    shape_changed_ = nrn_shape_changed_;
#if HAVE_IV
    Oc oc;
    oc.notify_attach(this);
#endif
    if ((var && strstr(var, "$1")) || pyobj) {
        rexp_ = new RangeExpr(var, pyobj, sec_list_);
    } else {
        rexp_ = NULL;
    }
    expr_ = var ? var : "pyobj";
    origin_ = 0.;
    d2root_ = 0.;
}

RangeVarPlot::~RangeVarPlot() {
    if (begin_section_) {
        section_unref(begin_section_);
        begin_section_ = NULL;
    }
    if (end_section_) {
        section_unref(end_section_);
        end_section_ = NULL;
    }
    delete sec_list_;
    if (rexp_) {
        delete rexp_;
    }
#if HAVE_IV
    Oc oc;
    oc.notify_detach(this);
#endif
}

int RangeVarPlot::get_color(void) {
    return color_;
}


void RangeVarPlot::set_color(int new_color) {
    color_ = new_color;
#if HAVE_IV
    IFGUI
    color(colors->color(color_));
    ENDGUI
#endif
}


#if HAVE_IV
void RangeVarPlot::update(Observable* o) {
    if (o) {  // must be Oc::notify_change_ because free is NULL
        // but do not update if multisplit active
        if (shape_changed_ != nrn_shape_changed_ && !nrn_multisplit_active_) {
            // printf("RangeVarPlot::update shape_changed %d %d\n", shape_changed_,
            // nrn_shape_changed_);
            shape_changed_ = nrn_shape_changed_;
            set_x();
            fill_pointers();
        }
    } else {
        // printf("RangeVarPlot::update -> GraphVector::update\n");
        GraphVector::update(o);
    }
}
#endif

void RangeVarPlot::origin(float x) {
    origin_ = x;
    fill_pointers();
}

double RangeVarPlot::d2root() {
    return d2root_;
}

void RangeVarPlot::x_begin(float x, Section* sec) {
    if (begin_section_) {
        section_unref(begin_section_);
    }
    begin_section_ = sec;
    section_ref(begin_section_);
    x_begin_ = x;
    set_x();
    fill_pointers();
}

void RangeVarPlot::x_end(float x, Section* sec) {
    if (end_section_) {
        section_unref(end_section_);
    }
    end_section_ = sec;
    section_ref(end_section_);
    x_end_ = x;
    set_x();
    fill_pointers();
}

float RangeVarPlot::left() {
    if (!sec_list_->empty()) {
        return sec_list_->front().len + origin_;
    } else {
        return origin_;
    }
}

float RangeVarPlot::right() {
    if (!sec_list_->empty()) {
        return sec_list_->back().len + origin_;
    } else {
        return origin_;
    }
}

void RangeVarPlot::compute() {
    if (rexp_) {
        rexp_->compute();
    }
}

#if HAVE_IV
void RangeVarPlot::request(Requisition& req) const {
    if (rexp_) {
        rexp_->compute();
    }
    GraphVector::request(req);
}
#endif

#if HAVE_IV
void RangeVarPlot::save(std::ostream& o) {
    char buf[256];
    o << "objectvar rvp_" << std::endl;
    Sprintf(buf, "rvp_ = new RangeVarPlot(\"%s\")", expr_.c_str());
    o << buf << std::endl;
    Sprintf(buf, "%s rvp_.begin(%g)", hoc_section_pathname(begin_section_), x_begin_);
    o << buf << std::endl;
    Sprintf(buf, "%s rvp_.end(%g)", hoc_section_pathname(end_section_), x_end_);
    o << buf << std::endl;
    Sprintf(buf, "rvp_.origin(%g)", origin_);
    o << buf << std::endl;
    Coord x, y;
    label_loc(x, y);
    Sprintf(buf,
            "save_window_.addobject(rvp_, %d, %d, %g, %g)",
            colors->color(color()),
            brushes->brush(brush()),
            x,
            y);
    o << buf << std::endl;
}
#endif

#if HAVE_IV
bool RangeVarPlot::choose_sym(Graph* g) {
    //	printf("RangeVarPlot::choose_sym\n");
    char s[256];
    s[0] = '\0';
    while (str_chooser("Range Variable or expr involving $1",
                       s,
                       XYView::current_pick_view()->canvas()->window())) {
        RangeVarPlot* rvp = new RangeVarPlot(s, NULL);
        rvp->ref();

        rvp->begin_section_ = begin_section_;
        rvp->x_begin_ = x_begin_;
        rvp->end_section_ = end_section_;
        rvp->x_end_ = x_end_;
        rvp->set_x();
        rvp->origin(origin_);
        // check to see if there is anything to plot
        if (!rvp->trivial()) {
            g->add_graphVector(rvp);
            rvp->label(g->label(s));
            rvp->unref();
            break;
        } else {
            printf("%s doesn't exist along the path %s(%g)", s, secname(begin_section_), x_begin_);
            printf(" to %s(%g)\n", secname(end_section_), x_end_);
        }
        rvp->unref();
    }

    return true;
}
#endif

#if 0
void SpacePlot::expr(const char* expr) {
	is_var_ = false;
	if (gv_) {
		gv_->begin();
	}
	Graph::add_var(expr);
	set_x();
}
#endif

void RangeVarPlot::fill_pointers() {
    long xcnt = sec_list_->size();
    if (xcnt) {
        Symbol* sym;
        char buf[200];
        begin();
        if (rexp_) {
            rexp_->fill();
        } else {
            sscanf(expr_.c_str(), "%[^[]", buf);
            sym = hoc_lookup(buf);
            if (!sym) {
                return;
            }
            Sprintf(buf, "%s(hoc_ac_)", expr_.c_str());
        }
        int noexist = 0;  // don't plot single points that don't exist
        bool does_exist;
        neuron::container::data_handle<double> pval{};
        for (long i = 0; i < xcnt; ++i) {
            Section* sec = (*sec_list_)[i].sec;
            hoc_ac_ = (*sec_list_)[i].x;
            if (rexp_) {
                does_exist = rexp_->exists(int(i));
            } else {
                nrn_pushsec(sec);
                does_exist = nrn_exists(sym, node_exact(sec, hoc_ac_));
                // does_exist = nrn_exists(sym, sec->pnode[node_index(sec, hoc_ac_)]);
            }
            if (does_exist) {
                if (rexp_) {
                    // TODO avoid conversion
                    pval = neuron::container::data_handle<double>{rexp_->pval(int(i))};
                } else {
                    pval = hoc_val_handle(buf);
                }
                if (noexist > 1) {
                    add((*sec_list_)[i - 1].len + origin_, {});
                    add((*sec_list_)[i - 1].len + origin_, pval);
                }
                if (i == 1 && noexist == 1) {
                    add((*sec_list_)[i - 1].len + origin_, pval);
                }
                add((*sec_list_)[i].len + origin_, pval);
                noexist = 0;
            } else {
                if (noexist == 1) {
                    add((*sec_list_)[i - 1].len + origin_, pval);
                    add((*sec_list_)[i - 1].len + origin_, {});
                }
                if (i == xcnt - 1 && noexist == 0) {
                    add((*sec_list_)[i].len + origin_, pval);
                }
                ++noexist;
            }
            nrn_popsec();
        }
    }
}

void RangeVarPlot::list(Object* ob) {
    hoc_List* l = (hoc_List*) ob->u.this_pointer;
    Section* sec = nullptr;
    for (SecPos p: *sec_list_) {
        if (p.sec != sec) {
            sec = p.sec;
            if (sec) {
                hoc_l_lappendsec(l, sec);
                section_ref(sec);
            }
        }
    }
}

#if 0
void SpacePlot::plot() {
	Graph* g = (Graph*)this;
	long xcnt = sec_list_->count();
	double* x = hoc_val_pointer("x");
	if (xcnt) {
		g->begin();
		for (long i=0; i < xcnt; ++i) {
			nrn_pushsec(sec_list_->item(i).sec);
			*x = sec_list_->item(i).x;
			gr_->plot(sec_list_->item(i).len);
			nrn_popsec();
		}
	}
	gr_->flush();
}

void SpacePlot::plot() {
	gr_->flush();
}
#endif

void RangeVarPlot::set_x() {
    if (!begin_section_ || !end_section_ || !begin_section_->prop || !end_section_->prop) {
        sec_list_->clear();
        return;
    }
    SecPos spos;
    double d, dist, d2r, x;
    Section *sec, *sec1, *sec2, *rootsec;
    Node *nd, *nd1, *nd2, *rootnode;
    sec1 = begin_section_;
    sec2 = end_section_;
    v_setup_vectors();
    sec_list_->clear();  // v_setup_vectors() may recurse once.
    nd1 = node_exact(sec1, x_begin_);
    nd2 = node_exact(sec2, x_end_);

    dist = topol_distance(sec1, nd1, sec2, nd2, &rootsec, &rootnode);
    // printf("dist=%g\n", dist);
    if (!rootnode) {
        hoc_execerror("SpacePlot", "No path from begin to end points");
    }
    d2r = topol_distance(sec1, nd1, rootsec, rootnode, &rootsec, &rootnode);
#if 0
	gr_->erase_most();
	gr_->new_size(-d2r, -80, dist-d2r, 40);
	gr_->xaxis(0,1,1);
	gr_->yaxis(0,1,1);
	Coord y1=gr_->y1(), y2 = gr_->y2();
	gr_->new_size(-d2r - dist/10, y1 - (y2-y1)/10, dist*1.1 - d2r, y2 + (y2-y1)/10);
#endif

    nd = nd1;
    sec = sec1;
    d = -d2r + node_dist(sec, nd);
    while (nd != rootnode) {
        x = node_dist(sec, nd);
        spos.sec = sec;
        spos.x = nrn_arc_position(sec, nd);
        spos.len = d - x;
        // printf("%s(%g) at %g  %g\n", secname(spos.sec), spos.x, spos.len, x);
        sec_list_->push_back(spos);
        if (x == 0.) {
            sec = nrn_trueparent(sec);
            d += node_dist(sec, nd);
        }
        nd = nrn_parent_node(nd);
    }

    if (sec) {
        spos.sec = sec;
    } else {
        spos.sec = nd->child;
    }
    spos.x = nrn_arc_position(spos.sec, nd);
    spos.len = 0.;
    // printf("%s(%g) at %g root\n", secname(spos.sec), spos.x, spos.len);
    sec_list_->push_back(spos);

    long indx = sec_list_->size();

    nd = nd2;
    sec = sec2;
    d = dist - d2r - node_dist(sec, nd);
    while (nd != rootnode) {
        x = node_dist(sec, nd);
        spos.sec = sec;
        spos.x = nrn_arc_position(sec, nd);
        spos.len = d + x;
        // printf("%s(%g) at %g\n", secname(spos.sec), spos.x, spos.len);
        sec_list_->insert(sec_list_->begin() + indx, spos);
        if (x == 0.) {
            sec = nrn_trueparent(sec);
            d -= node_dist(sec, nd);
        }
        nd = nrn_parent_node(nd);
    }
    for (sec = rootsec; sec->parentsec; sec = sec->parentsec) {
    }
    nd = sec->parentnode;
    d2root_ = topol_distance(rootsec, rootnode, sec, nd, &sec, &nd);
// debugging
#if 0
printf("debugging\n");
	long cnt, icnt;
	cnt = sec_list_->count();
	for (icnt=0; icnt<cnt; ++icnt) {
		printf("%s(%g) at %g\n", secname(sec_list_->item(icnt).sec),
			sec_list_->item(icnt).x, sec_list_->item(icnt).len);
	}
#endif
}

RangeExpr::RangeExpr(const char* expr, Object* pycall, SecPosList* spl) {
    spl_ = spl;
    n_ = 0;
    val_ = NULL;
    exist_ = NULL;
    if (pycall) {
        if (nrnpy_rvp_rxd_to_callable) {
            cmd_ = new HocCommand(nrnpy_rvp_rxd_to_callable(pycall));
        } else {
            cmd_ = new HocCommand(pycall);
        }
        return;
    }
    char buf[256];
    const char* p1;
    char* p2;
    Sprintf(buf, "hoc_ac_ = ");
    p2 = buf + strlen(buf);
    for (p1 = expr; *p1;) {
        if (p1[0] == '$' && p1[1] == '1') {
            p1 += 2;
            strcpy(p2, "hoc_ac_");
            p2 += 7;
        } else {
            *p2++ = *p1++;
        }
    }
    *p2 = '\0';
    cmd_ = new HocCommand(buf);
}

RangeExpr::~RangeExpr() {
    if (val_) {
        delete[] val_;
        delete[] exist_;
    }
    delete cmd_;
}


void RangeExpr::fill() {
    if (n_ != spl_->size()) {
        if (val_) {
            delete[] val_;
            delete[] exist_;
        }
        n_ = spl_->size();
        if (n_) {
            val_ = new double[n_];
            exist_ = new bool[n_];
        }
    }
    int temp = hoc_execerror_messages;
    for (long i = 0; i < n_; ++i) {
        nrn_pushsec((*spl_)[i].sec);
        hoc_ac_ = (*spl_)[i].x;
        hoc_execerror_messages = 0;
        if (cmd_->pyobject()) {
            hoc_pushx(hoc_ac_);
            int err = 0;                         // no messages
            val_[i] = cmd_->func_call(1, &err);  // return err==0 means success
            exist_[i] = err ? false : true;
            if (err) {
                val_[i] = 0.0;
            }
            nrn_popsec();
            continue;
        }
        if (cmd_->execute(bool(false)) == 0) {
            exist_[i] = true;
            val_[i] = 0.;
        } else {
            exist_[i] = false;
#if 0
			printf("RangeExpr: %s no exist at %s(%g)\n",
				cmd_->name(), secname((*spl_)[i].sec),
				(*spl_)[i].x
			);
#endif
        }
        nrn_popsec();
    }
    hoc_execerror_messages = temp;
}

void RangeExpr::compute() {
    for (long i = 0; i < n_; ++i) {
        if (exist_[i]) {
            nrn_pushsec((*spl_)[i].sec);
            hoc_ac_ = (*spl_)[i].x;
            if (cmd_->pyobject()) {
                hoc_pushx(hoc_ac_);
                int err = 1;  // messages
                val_[i] = cmd_->func_call(1, &err);
            } else {
                cmd_->execute(bool(false));
                val_[i] = hoc_ac_;
            }
            nrn_popsec();
        }
    }
}

bool RangeExpr::exists(int i) {
    if (i < n_) {
        return exist_[i];
    } else {
        return false;
    }
}

double* RangeExpr::pval(int i) {
    if (i < n_) {
        return val_ + i;
    } else {
        return 0;
    }
}
