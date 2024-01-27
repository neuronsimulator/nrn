#include <../../nrnconf.h>
#include <stdio.h>
#include "oclist.h"
#include "nrnoc2iv.h"
#include "classreg.h"

#if HAVE_IV
#include "ppshape.h"
#endif  // HAVE_IV
#include "gui-redirect.h"

// ppshape registration

static double pp_append(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("PPShape.append", v);
#if HAVE_IV
    IFGUI
    Object* ob = *hoc_objgetarg(1);
    ((PPShape*) v)->pp_append(ob);
    ENDGUI
#endif
    return 1.;
}

static Member_func pp_members[] = {
    //	{"view", pp_view},
    {"append", pp_append},
    {0, 0}};

static void* pp_cons(Object* ho) {
    TRY_GUI_REDIRECT_OBJ("PPShape", NULL);
#if HAVE_IV
    IFGUI
    Object* ob = *hoc_objgetarg(1);
    check_obj_type(ob, "List");
    PPShape* p = new PPShape((OcList*) ob->u.this_pointer);
    p->ref();
    p->view(200);
    p->hoc_obj_ptr(ho);
    return (void*) p;
    ENDGUI
#endif
    return 0;
}

static void pp_destruct(void* v) {
    TRY_GUI_REDIRECT_NO_RETURN("~PPShape", v);
#if HAVE_IV
    IFGUI
    Resource::unref((PPShape*) v);
    ENDGUI
#endif
}

void PPShape_reg() {
    //	printf("PPShape_reg\n");
    class2oc("PPShape", pp_cons, pp_destruct, pp_members, NULL, NULL, NULL);
}

#if HAVE_IV  // to end of file

/* static */ class PPShapeImpl {
  public:
    OcList* ocl_;
};


PPShape::PPShape(OcList* ocl)
    : ShapeScene(NULL) {
    si_ = new PPShapeImpl;
    si_->ocl_ = ocl;
    Resource::ref(si_->ocl_);
    long i, cnt = ocl->count();
    for (i = 0; i < cnt; ++i) {
        install(si_->ocl_->object(i));
    }
}

PPShape::~PPShape() {
    Resource::unref(si_->ocl_);
    delete si_;
}

void PPShape::pp_append(Object* ob) {
    if (!is_point_process(ob)) {
        hoc_execerror(hoc_object_name(ob), "not a point process");
    }
    if (si_->ocl_->index(ob) != -1) {
        return;
    }
    si_->ocl_->append(ob);
    install(ob);
}

void PPShape::install(Object* ob) {
    append_fixed(new PointProcessGlyph(ob));
}

void PPShape::pp_remove(PointProcessGlyph* ppg) {
    long i = si_->ocl_->index(ppg->object());
    if (i == -1) {
        return;
    }
    si_->ocl_->remove(i);
    remove(glyph_index(ppg));
}

void PPShape::pp_move(PointProcessGlyph*) {}

void PPShape::examine(PointProcessGlyph*) {}

PointProcessGlyph::PointProcessGlyph(Object* ob)
    : GLabel("x", colors->color(2), true, 12, .5, .5) {
    ob_ = ob;
    ++ob->refcount;
}

PointProcessGlyph::~PointProcessGlyph() {
    hoc_dec_refcount(&ob_);
}

#endif
