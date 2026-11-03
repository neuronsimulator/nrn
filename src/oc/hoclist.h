#pragma once

#if HOC_L_LIST
#define stralloc    hoc_l_stralloc
#define newitem     hoc_l_newitem
#define newlist     hoc_l_newlist
#define freelist    hoc_l_freelist
#define insertstr   hoc_l_insertstr
#define insertitem  hoc_l_insertitem
#define insertlist  hoc_l_insertlist
#define insertsym   hoc_l_insertsym
#define insertsec   hoc_l_insertsec
#define insertobj   hoc_l_insertobj
#define insertvoid  hoc_l_insertvoid
#define linsertsym  hoc_l_linsertsym
#define linsertstr  hoc_l_linsertstr
#define lappendsym  hoc_l_lappendsym
#define lappendstr  hoc_l_lappendstr
#define lappenditem hoc_l_lappenditem
#define lappendlst  hoc_l_lappendlst
#define lappendsec  hoc_l_lappendsec
#define lappendobj  hoc_l_lappendobj
#define lappendvoid hoc_l_lappendvoid
#define delitems    hoc_l_delitems
#define movelist    hoc_l_movelist
#define replacstr   hoc_l_replacstr
#define Item        hoc_Item
#define List        hoc_List
#endif

struct Object;
struct Section;
struct Symbol;
struct hoc_Item {
    union {
        hoc_Item* itm;
        hoc_Item* lst;
        char* str;
        Symbol* sym;
        Section* sec;
        Object* obj;
        void* vd;
    } element; /* pointer to the actual item */
    hoc_Item* next;
    hoc_Item* prev;
    short itemtype;
};
using hoc_List = hoc_Item;

constexpr auto range_sec(hoc_List* iterable) {
    struct iterator {
        hoc_Item* iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            iter = iter->next;
        }
        Section* operator*() const {
            return iter->element.sec;
        }
    };
    struct iterable_wrapper {
        hoc_List* iterable;
        auto begin() {
            return iterator{iterable->next};
        }
        auto end() {
            return iterator{iterable};
        }
    };
    return iterable_wrapper{iterable};
}

#define ITEM0 nullptr
#define LIST0 nullptr

#define ITERATE(itm, lst) for (itm = (lst)->next; itm != (lst); itm = itm->next)
/*
 * this is convenient way to get the element pointer if you know what type
 * the item is
 */
#define SYM(q)     ((q)->element.sym)
#define STR(q)     ((q)->element.str)
#define ITM(q)     ((q)->element.itm)
#define LST(q)     ((q)->element.lst)
#define hocSEC(q)  ((q)->element.sec)
#define OBJ(q)     ((q)->element.obj)
#define VOIDITM(q) ((q)->element.vd)

/* types not defined in parser */
#define ITEM        2
#define LIST        3
#define VOIDPOINTER 4
/*
 * An item type, STRING is also used as an item type
 */

extern char* hoc_l_stralloc(const char*, char* release);
extern hoc_List* hoc_l_newlist();
extern hoc_Item* hoc_l_insertstr(hoc_Item*, const char*);
extern hoc_Item* hoc_l_insertsym(hoc_Item*, struct Symbol*);
extern hoc_Item* hoc_l_insertitem(hoc_Item*, hoc_Item*);
extern hoc_Item* hoc_l_insertlist(hoc_Item*, hoc_List*);
extern hoc_Item* hoc_l_insertsec(hoc_Item*, struct Section*);
extern hoc_Item* hoc_l_insertvoid(hoc_Item*, void*);
extern hoc_Item* hoc_l_linsertstr(hoc_List*, const char*);
extern hoc_Item* hoc_l_linsertsym(hoc_List*, struct Symbol*);
extern hoc_Item* hoc_l_lappendstr(hoc_List*, const char*);
extern hoc_Item* hoc_l_lappendsym(hoc_List*, struct Symbol*);
extern hoc_Item* hoc_l_lappenditem(hoc_List*, hoc_Item*);
extern hoc_Item* hoc_l_lappendlst(hoc_List*, hoc_List*);
extern hoc_Item* hoc_l_lappendsec(hoc_List*, struct Section*);
extern hoc_Item* hoc_l_lappendvoid(hoc_List*, void*);
extern hoc_Item* hoc_l_lappendobj(hoc_List*, struct Object*);
extern void hoc_l_freelist(hoc_List**);
extern hoc_Item* hoc_l_next(hoc_Item*);
extern hoc_Item* hoc_l_prev(hoc_Item*);
extern void hoc_l_delete(hoc_Item*);
extern void hoc_l_move(hoc_Item*, hoc_Item*, hoc_Item*);
extern void hoc_l_movelist(hoc_Item*, hoc_Item*, hoc_List*);
extern void hoc_l_replacstr(hoc_Item*, const char*);

#define Insertstr   insertstr
#define Insertsym   insertsym
#define Insertsec   insertsec
#define Linsertsym  linsertsym
#define Linsertstr  linsertstr
#define Lappendsym  lappendsym
#define Lappendstr  lappendstr
#define Lappenditem lappenditem
#define Lappendlst  lappendlst
#define Lappendsec  lappendsec
