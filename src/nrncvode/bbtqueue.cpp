// balanced binary tree queue implemented by Michael Hines

TQItem::TQItem() {
    left_ = nullptr;
    right_ = nullptr;
    parent_ = nullptr;
}

TQItem::~TQItem() {
    if (left_) {
        delete left_;
    }
    if (right_) {
        delete right_;
    }
}

static void deleteitem(TQItem* i) {
    if (i->left_) {
        deleteitem(i->left_);
    }
    if (i->right_) {
        deleteitem(i->right_);
    }
    i->left_ = nullptr;
    i->right_ = nullptr;
    tpool_->free(i);
}

bool TQItem::check() {
#if DOCHECK
    if (left_ && left_->t_ > t_) {
        printf("left %g not <= %g\n", left_->t_, t_);
        return false;
    }
    if (right_ && right_->t_ < t_) {
        printf("right %g not >= %g\n", right_->t_, t_);
        return false;
    }
    if (parent_) {
        if (parent_->left_ == this) {
            if (t_ > parent_->t_) {
                printf("%g not <= parent %g\n", t_, parent_->t_);
                return false;
            }
        } else if (parent_->right_ == this) {
            if (t_ < parent_->t_) {
                printf("%g not >= parent %g\n", t_, parent_->t_);
                return false;
            }
        } else {
            printf("this %g is not a child of its parent %g\n", t_, parent_->t_);
            return false;
        }
    }
    if (w_ != wleft() + wright() + 1) {
        printf("%g: weight %d inconsistent with left=%d and right=%d\n", t_, w_, wleft(), wright());
        return false;
    }
#endif
    return true;
}

static void prnt(const TQItem* b, int level) {
    int i;
    for (i = 0; i < level; ++i) {
        Printf("    ");
    }
    Printf("%g %c %d\n", b->t_, b->data_ ? 'x' : 'o', b->w_);
}

static void chk(TQItem* b, int level) {
    if (!b->check()) {
        hoc_execerror("chk failed", errmess_);
    }
}

void TQItem::t_iterate(void (*f)(const TQItem*, int), int level) {
    if (left_) {
        left_->t_iterate(f, level + 1);
    }
    f(this, level);
    if (right_) {
        right_->t_iterate(f, level + 1);
    }
}

#if BBTQ == 0  // balanced binary tree implemented by Michael Hines

TQueue::TQueue() {
    if (!tpool_) {
        tpool_ = new TQItemPool(1000);
    }
    root_ = nullptr;
    least_ = nullptr;

#if COLLECT_TQueue_STATISTICS
    nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
    nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
    if (root_) {
        deleteitem(root_);
    }
}

void TQueue::print() {
    if (root_) {
        root_->t_iterate(prnt, 0);
    }
}

void TQueue::forall_callback(void (*f)(const TQItem*, int)) {
    if (root_) {
        root_->t_iterate(f, 0);
    }
}

void TQueue::check(const char* mes) {
#if DOCHECK
    errmess_ = mes;
    if (root_) {
        root_->t_iterate(chk, 0);
    }
    errmess_ = nullptr;
#endif
}

double TQueue::least_t() {
    TQItem* b = least();
    if (b) {
        return b->t_;
    } else {
        return 1e15;
    }
}

TQItem* TQueue::least() {
    STAT(nleast)
#if DOCHECK
    TQItem* b;
    b = root_;
    if (b)
        for (; b->left_; b = b->left_) {
            STAT(nleastsrch)
        }
#if DOCHECK
    assert(least_ == b);
#else
    least_ = b;
#endif
#endif
    return least_;
}

void TQueue::new_least() {
    assert(least_);
    assert(!least_->left_);
    TQItem* b = least_->right_;
    if (b) {
        for (; b->left_; b = b->left_) {
            ;
        }
        least_ = b;
    } else {
        b = least_->parent_;
        if (b) {
            assert(b->left_ == least_);
            least_ = b;
        } else {
            least_ = nullptr;
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
#if 0
	// this is a bug
	// check if it really needs moving
	STAT(ncompare)
	double tmin = -1e9, tmax = 1e9;
	if (i->left_) {
		tmin = i->left_->t_;
	}else if (i->parent_ && i->parent_->right_ == i) {
		tmin = i->parent_->t_;
	}
	if (i->right_) {
		tmax = i->right_->t_;
	}else if (i->parent_ && i->parent_->left_ == i) {
		tmax = i->parent_->t_;
	}
	if ( tmin <= tnew && tnew < tmax) { // confusing to check equality
		STAT(nfastmove)
		i->t_ = tnew;
i->check();
PSTOP(1)
		return;
	}
#endif
    // the long way
    remove1(i);
    insert1(tnew, i);
    PSTOP(1)
}

TQItem* TQueue::find(double t) {
    TQItem* b;
    STAT(nfind)
    for (b = root_; b;) {
        STAT(nfindsrch)
        STAT(ncompare)
        if (t == b->t_) {
            // printf("found %g\n", t);
            return b;
        } else if (t < b->t_) {
            b = b->left_;
        } else {
            b = b->right_;
        }
    }
    Printf("couldn't find %g\n", t);
    return b;
}

void TQueue::remove(TQItem* i) {
    PSTART(1)
    if (i) {
        remove1(i);
        deleteitem(i);
    }
    PSTOP(1)
}

void TQueue::remove1(TQItem* i) {
    STAT(nrem)
    // merely remove if leaf
    // if no left, right becomes left of parent
    // replace i with rightmost on left (say x)
    // we could just replace i.data with x.data
    // and remove x. But then, data holding pointer to
    // x would be invalid. So take the trouble to reset
    // pointer belonging to x
    check("begin remove1");
    if (least_ && least_ == i) {
        new_least();
    }
    TQItem* p = i->parent_;
    TQItem** child;
    bool doweight = true;
    if (p) {
        // printf("removing with a parent %g\n", i->t_);
        if (p->left_ == i) {
            child = &p->left_;
        } else {
            child = &p->right_;
        }
    }

    if (i->left_) {
        STAT(ncmplxrem);
        // printf("removing with a left %g\n", i->t_);
        // replace i with rightmost on left
        TQItem* x;
        for (x = i->left_; x->right_; x = x->right_) {
            ;
        }
        if (x == i->left_) {
            // printf("x == i->left\n");
            if (p) {
                *child = x;
            } else {
                root_ = x;
            }
            x->parent_ = i->parent_;
            x->right_ = i->right_;
            if (x->right_) {
                x->right_->parent_ = x;
                //				x->w_ += x->right_->w_;
                x->w_ = i->w_ - 1;
            }
        } else {
            remove1(x);
            doweight = false;
            if (p) {
                *child = x;
            } else {
                root_ = x;
            }
            x->parent_ = i->parent_;
            x->right_ = i->right_;
            x->left_ = i->left_;
            x->left_->parent_ = x;
            if (x->right_) {
                x->right_->parent_ = x;
            }
            x->w_ = i->w_;
        }
    } else if (i->right_) {
        // printf("removing with a right but no left %g\n", i->t_);
        // check(); printf("checked\n");
        // no left
        if (p) {
            *child = i->right_;
            (*child)->parent_ = p;
        } else {
            root_ = i->right_;
            root_->parent_ = nullptr;
        }
    } else {
        // a leaf
        // printf("removing leaf %g\n", i->t_);
        if (p) {
            *child = nullptr;
        } else {
            root_ = nullptr;
        }
    }
    if (doweight) {
        while (p) {
            --p->w_;
            p = p->parent_;
        }
    }
    i->right_ = nullptr;
    i->left_ = nullptr;
    i->parent_ = nullptr;
    check("end remove1");
}

void TQueue::reverse(TQItem* b) {  // switch item and parent
    TQItem* p = b->parent_;
    if (p) {
        STAT(nbal)
        if (p->parent_) {
            if (!p->parent_->check())
                Printf("p->parent failed start\n");
        }
        if (!p->check())
            Printf("p failed at start\n");
        if (!b->check())
            Printf("b failed at start\n");
        if (p->parent_) {
            b->parent_ = p->parent_;
            if (p->parent_->left_ == p) {
                p->parent_->left_ = b;
            } else {
                p->parent_->right_ = b;
            }
        } else {
            assert(root_ == p);
            b->parent_ = nullptr;
            root_ = b;
        }
        b->w_ = p->w_;
        p->parent_ = b;
        if (p->left_ == b) {
            p->left_ = b->right_;
            b->right_ = p;
            if (p->left_) {
                p->left_->parent_ = p;
            }
        } else {
            p->right_ = b->left_;
            b->left_ = p;
            if (p->right_) {
                p->right_->parent_ = p;
            }
        }
        p->w_ = p->wleft() + p->wright() + 1;
        if (b->parent_) {
            if (!b->parent_->check())
                Printf("b->parent failed end\n");
        }
        if (!p->check())
            Printf("p failed at end\n");
        if (!b->check())
            Printf("b failed at end\n");
    }
}

TQItem* TQueue::insert(double t, void* data) {
    PSTART(1)
    //	TQItem* i = new TQItem();
    TQItem* i = tpool_->alloc();
    i->data_ = data;
    insert1(t, i);
    PSTOP(1)
    return i;
}

void TQueue::insert1(double t, TQItem* i) {
    check("begin insert1");
    STAT(ncompare)
    if (!least_ || t < least_->t_) {
        least_ = i;
    }
    TQItem* p = root_;
    STAT(ninsert)
    if (p) {
        for (;;) {
            STAT(ncompare)
            if (t < p->t_) {
                if (p->left_) {
                    p = p->left_;
                    //					if (p->unbalanced()) {
                    //						reverse(p);
                    //					}
                } else {
                    p->left_ = i;
                    break;
                }
            } else {
                if (p->right_) {
                    p = p->right_;
                    //					if (p->unbalanced()) {
                    //						reverse(p);
                    //					}
                } else {
                    p->right_ = i;
                    break;
                }
            }
        }
    } else {
        root_ = i;
    }
    i->parent_ = p;
    i->t_ = t;
    i->w_ = 1;
    for (p = i->parent_; p; p = p->parent_) {
        ++p->w_;
    }
    for (p = i->parent_; p;) {
        if (p->unbalanced()) {
            reverse(p);
        } else {
            p = p->parent_;
        }
    }
    check("end insert1");
}

bool TQItem::unbalanced() {
    int balance, dbalance;
    if (parent_) {
        balance = parent_->wright() - parent_->wleft();
        if (parent_->left_ == (TQItem*) this) {
            if (balance < -(wright() + 1)) {
                // printf("balance %d to %d\n", balance, balance + 2*(wright()+1));
                return true;
            }
        } else {
            if (balance > wleft() + 1) {
                // printf("balance %d to %d\n", balance, balance - 2*(wleft()+1));
                return true;
            }
        }
    }
    return false;
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
    Printf("insertions=%lu  moves=%lu fastmoves=%lu removals=%lu calls to least=%lu\n",
           ninsert,
           nmove,
           nfastmove,
           nrem,
           nleast);
    Printf("calls to find=%lu balances=%lu complex removals=%lu\n", nfind, nbal, ncmplxrem);
    Printf("comparisons=%lu leastsearch=%lu findsearch=%lu\n", ncompare, nleastsrch, nfindsrch);
#else
    Printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

#endif
