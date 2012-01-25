// red black tree
/* Derived from
http://ciips.ee.uwa.edu.au/~morris/Year2/PLDS210/niemann/s_man.htm
Sorting and Searching Algorithms:
A Cookbook
by
Thomas Niemann
*/
/* The original c code is enclosed in #if 0 and the original functions
are given the class name RBTQueue (which is defined as TQueue)
whereas my own analogous class methods to the old TQueue have the same name
*/

/* red-black tree */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#if 0
typedef int T;                  /* type of item to be stored */
#else
typedef double T;
#endif

#define compLT(a,b) (a < b)
#define compEQ(a,b) (a == b)

#if 0
/* Red-Black tree description */
typedef enum { BLACK, RED } nodeColor;
#endif

#if 0
typedef struct Node_ {
    struct Node_ *left;         /* left child */
    struct Node_ *right;        /* right child */
    struct Node_ *parent;       /* parent */
    nodeColor color;            /* node color (BLACK, RED) */
    T data;                     /* data stored in node */
} Node;
#else
#define left left_
#define right right_
#define parent parent_
#define Node TQItem
#define data t_
#define color red_
#define BLACK false
#define RED true
#define root root_
#endif

#if 0
#define NIL &sentinel           /* all leafs are sentinels */
Node sentinel = { NIL, NIL, 0, BLACK, 0};
Node *root = NIL;               /* root of Red-Black tree */

#else

#undef NIL
#define NIL sentinel
static TQItem* sentinel;
#endif

TQItem::TQItem() {
	left_ = NIL;
	right_ = NIL;
	parent_ = nil;
}

TQItem::~TQItem() {
	assert(this != NIL);
}

static void deleteitem(TQItem* i) {
	assert(i != NIL);
	if (i->left_ && i->left != NIL) {
		deleteitem(i->left_);
	}
	if (i->right_ && i->right != NIL) {
		deleteitem(i->right_);
	}
	i->left_ = nil;
	i->right_ = nil;
	tpool_->free(i);
}

bool TQItem::check() {
#if DOCHECK
#endif
	return true;
}

static void prnt(const TQItem* b, int level) {
	int i;
	for (i=0; i < level; ++i) {
		printf("    ");
	}
//	printf("%g %c %d\n", b->t_, b->data_?'x':'o', b->red_);
	printf("%g %c %d Q=%p D=%p\n", b->t_, b->data_?'x':'o', b->red_, b, b->data_);
}

static void chk(TQItem* b, int level) {
	if (!b->check()) {
		hoc_execerror("chk failed", errmess_);
	}
}

void TQItem::t_iterate(void (*f)(const TQItem*, int), int level) {
	if (left_ && left_ != NIL) {
		left_->t_iterate(f, level+1);
	}
	f(this, level);
	if (right_ && right_ != NIL) {
		right_->t_iterate(f, level+1);
	}
}
		
TQueue::TQueue() {
	if (!sentinel) {
		sentinel = new TQItem();
		sentinel->red_ = BLACK;
		sentinel->t_ = 0.;
		sentinel->data_ = nil;
		sentinel->left_ = NIL;
		sentinel->right_ = NIL;
	}
	if (!tpool_) {
		tpool_ = new TQItemPool(1000);
	}
	root_ = NIL;
	least_ = NIL;

#if COLLECT_TQueue_STATISTICS
	nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
	nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
	if (root_ != NIL) {
		deleteitem(root_);
		
	}
}
	
void TQueue::print() {
	if (root_!= NIL) {
		root_->t_iterate(prnt, 0);
	}
}

void TQueue::forall_callback(void(*f)(const TQItem*, int)) {
	if (root_ != NIL ) {
		root_->t_iterate(f, 0);
	}
}

void TQueue::check(const char* mes) {
#if DOCHECK
	errmess_ = mes;
	if (root_ != NIL) {
		root_->t_iterate(chk, 0);
	}
	errmess_ = nil;
#endif
}

double TQueue::least_t() {
	TQItem* b = least();
	if (b) {
		return b->t_;
	}else{
		return 1e15;
	}
}

TQItem* TQueue::least() {
	STAT(nleast)
#if !FAST_LEAST || DOCHECK
	TQItem* b;
	b = root_;
	if (b != NIL) for (; b->left_ != NIL; b = b->left_) {
		STAT(nleastsrch)
	}
#if DOCHECK
	assert(least_ == b);
#else
	least_ = b;
#endif
#endif
	return least_ != NIL ? least_ : nil ;

}

void TQueue::new_least() {
	assert(least_ != NIL);
	assert(least_->left_ == NIL);
	TQItem* b = least_->right_;
	if (b != NIL) {
		for (;b->left_ != NIL; b = b->left_) {;}
		least_ = b;
	}else{
		b = least_->parent_;
		if (b && b != NIL) {
			assert(b->left_ == least_);
			least_ = b;
		}else{
			least_ = NIL;
		}
	}
}

void TQueue::move_least(double tnew) {
	TQItem* b = least();
	if (b) {
		move(b, tnew);
	}
}

void TQueue::move(TQItem* i, double tnew) {
PSTART(1)
	STAT(nmove)
	// the long way
	deleteNode(i);
	insertNode(tnew, i);
PSTOP(1)
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
	printf("insertions=%lu  moves=%lu fastmoves=%lu removals=%lu calls to least=%lu\n", ninsert, nmove, nfastmove, nrem, nleast);
	printf("calls to find=%lu balances=%lu complex removals=%lu\n", nfind, nbal, ncmplxrem);
	printf("comparisons=%lu leastsearch=%lu findsearch=%lu\n", ncompare, nleastsrch, nfindsrch);
#else
	printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

void RBTQueue::rotateLeft(Node *x) {

   /**************************
    *  rotate node x to left *
    **************************/

    Node *y = x->right;

    /* establish x->right link */
    x->right = y->left;
    if (y->left != NIL) y->left->parent = x;

    /* establish y->parent link */
    if (y != NIL) y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    } else {
        root = y;
    }

    /* link x and y */
    y->left = x;
    if (x != NIL) x->parent = y;
}

void RBTQueue::rotateRight(Node *x) {

   /****************************
    *  rotate node x to right  *
    ****************************/

    Node *y = x->left;

    /* establish x->left link */
    x->left = y->right;
    if (y->right != NIL) y->right->parent = x;

    /* establish y->parent link */
    if (y != NIL) y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    } else {
        root = y;
    }

    /* link x and y */
    y->right = x;
    if (x != NIL) x->parent = y;
}

void RBTQueue::insertFixup(Node *x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after inserting node x           *
    *************************************/

    /* check Red-Black properties */
    while (x != root && x->parent->color == RED) {
        /* we have a violation */
        if (x->parent == x->parent->parent->left) {
            Node *y = x->parent->parent->right;
            if (y->color == RED) {

                /* uncle is RED */
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            } else {

                /* uncle is BLACK */
                if (x == x->parent->right) {
                    /* make x a left child */
                    x = x->parent;
                    rotateLeft(x);
                }

                /* recolor and rotate */
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateRight(x->parent->parent);
            }
        } else {

            /* mirror image of above code */
            Node *y = x->parent->parent->left;
            if (y->color == RED) {

                /* uncle is RED */
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            } else {

                /* uncle is BLACK */
                if (x == x->parent->left) {
                    x = x->parent;
                    rotateRight(x);
                }
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateLeft(x->parent->parent);
            }
        }
    }
    root->color = BLACK;
}

Node* RBTQueue::insert(double t, void* d) {
	TQItem* i = tpool_->alloc();
	i->data_ = d;
	insertNode(t, i);
	return i;
}

void RBTQueue::insertNode(T data, Node* x) {
//printf("%p::insertNode %g\n", this, data);
    Node *current, *parent;

#if FAST_LEAST
	STAT(ncompare)
	if (data < least_t()) {
		least_ = x;
	}
#endif

   /***********************************************
    *  allocate node for data and insert in tree  *
    ***********************************************/

    /* find where node belongs */
	STAT(ninsert)
    current = root;
    parent = 0;
    while (current != NIL) {
#if 0
        if (compEQ(data, current->data)) return (current);
#endif
        parent = current;
        current = compLT(data, current->data) ?
            current->left : current->right;
    }

    /* setup new node */
#if 0
    if ((x = malloc (sizeof(*x))) == 0) {
        printf ("insufficient memory (insertNode)\n");
        exit(1);
    }
#endif
    x->data = data;
    x->parent = parent;
    x->left = NIL;
    x->right = NIL;
    x->color = RED;

    /* insert node in tree */
    if(parent) {
        if(compLT(data, parent->data))
            parent->left = x;
        else
            parent->right = x;
    } else {
        root = x;
    }

    insertFixup(x);
}

void RBTQueue::deleteFixup(Node *x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after deleting node x            *
    *************************************/

    while (x != root && x->color == BLACK) {
        if (x == x->parent->left) {
            Node *w = x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotateLeft (x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotateRight (w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotateLeft (x->parent);
                x = root;
            }
        } else {
            Node *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotateRight (x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotateLeft (w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotateRight (x->parent);
                x = root;
            }
        }
    }
    x->color = BLACK;
}

void RBTQueue::remove(Node* z) {
	if (z && z != NIL) {
		deleteNode(z);
		tpool_->free(z);
	}
}

void RBTQueue::deleteNode(Node *z) {
//printf("%p::deleteNode %g\n", this, z->t_);
    Node *x, *y;

	STAT(nrem)
   /*****************************
    *  delete node z from tree  *
    *****************************/

    if (z == NIL) return;

#if FAST_LEAST
	if (least_ == z) {
		new_least();
	}
#endif

    if (z->left == NIL || z->right == NIL) {
        /* y has a NIL node as a child */
        y = z;
    } else {
        /* find tree successor with a NIL node as a child */
        y = z->right;
        while (y->left != NIL) y = y->left;
    }

    /* x is y's only child */
    if (y->left != NIL)
        x = y->left;
    else
        x = y->right;

    /* remove y from the parent chain */
    x->parent = y->parent;
    if (y->parent)
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    else
        root = x;

#if 0
	// inadequate. does not preserve association between data and Node*
    if (y != z) z->data = y->data;
#endif

    if (y->color == BLACK)
        deleteFixup (x);

#if 0
    free (y);
#else
	//we need to preserve the association between data and Node*
	if (y != z) {
		y->red_ = z->red_;
		y->parent_ = z->parent_;
		y->left_ = z->left_;
		y->right_ = z->right_;
		if (z->left_ != NIL) { z->left_->parent_ = y; }
		if (z->right_ != NIL) { z->right_->parent_ = y; }
		if (z->parent && z->parent_ != NIL) {
			if (z->parent_->left_ == z) {
				z->parent_->left_ = y;
			}else{
				z->parent_->right_ = y;
			}
		}
	}
#endif
}

Node* RBTQueue::find(T data) {

	STAT(nfind);
   /*******************************
    *  find node containing data  *
    *******************************/

    Node *current = root;
    while(current != NIL)
	STAT(nfindsrch)
	STAT(ncompare)
        if(compEQ(data, current->data))
            return (current);
        else
            current = compLT (data, current->data) ?
                current->left : current->right;
    return(0);
}
