/*
** sptree.hpp:  The following type declarations provide the binary tree
**  representation of event-sets or priority queues needed by splay trees
**
**  assumes that data and datb will be provided by the application
**  to hold all application specific information
**
**  assumes that key will be provided by the application, comparable
**  with the compare function applied to the addresses of two keys.
**
** Originaly written by Douglas W. Jones in Fortran helped by Srinivas R. Sataluri.
** Translated by David Brower to C circa 1988.
** Some fixes by Mark Moraes <moraes@csri.toronto.edu>.
**
** For some litterature about this Splay tree: https://en.wikipedia.org/wiki/Splay_tree
** The original implementation is based on:
**     Self Adjusting Binary Trees
**         by D. D. Sleator and R. E. Tarjan,
**             Proc. ACM SIGACT Symposium on Theory
**             of Computing (Boston, Apr 1983) 235-245.
**
** For more insights, `enqueue` is doing the splay top-down.
** `splay` itself is doing it bottom-up.
*/

#pragma once

#include <functional>

template <typename T>
class SPTree {
  public:
    SPTree() = default;

    // Is this SPTree empty?
    bool empty() const;

    // Return the value of enqcmps.
    int get_enqcmps() const;

    // Insert item in the tree.
    //
    // Put `n` after all other nodes with the same key; when this is
    // done, `n` will be the root of the splay tree, all nodes
    // with keys less than or equal to that of `n` will be in the
    // left subtree, all with greater keys will be in the right subtree;
    // the tree is split into these subtrees from the top down, with rotations
    // performed along the way to shorten the left branch of the right subtree
    // and the right branch of the left subtree
    void enqueue(T* n);

    // Return and remove the first node.
    //
    // Remove and return the first node; this deletes
    // (and returns) the leftmost node, replacing it with its right
    // subtree (if there is one); on the way to the leftmost node, rotations
    // are performed to shorten the left branch of the tree.
    T* dequeue();

    // Return the element in the tree with the lowest key.
    //
    // Returns a pointer to the item with the lowest key.
    // The left branch will be shortened as if the tree had been splayed around the first item.
    // This is done by dequeue and enqueuing the first item.
    T* first();

    // Remove node `n` from the tree.
    //
    // `n` is removed; the resulting splay tree has been splayed
    // around its new root, which is the successor of n
    void remove(T* n);

    // Find a node with the given key.
    //
    // Splays the found node to the root.
    T* find(double key);

    // Apply the function `f` to each node in ascending order.
    //
    // `f` is the function that will be applied to nodes. The first argument will be
    // a pointer to the current node, the integer value is unused and will always be `0`.
    // If n is given, start at that node, otherwise start from the head.
    void apply_all(void (*f)(const T*, int), T* n) const;

  private:
    // Dequeue the first node in the subtree np
    T* dequeue(T** np);

    // Reorganize the tree.
    //
    // The tree is reorganized so that `n` is the root of the
    // splay tree; operation is undefined if `n` is not
    // in the tree; the tree is split from `n` up to the old root, with all
    // nodes to the left of `n` ending up in the left subtree, and all nodes
    // to the right of n ending up in the right subtree; the left branch of
    // the right subtree and the right branch of the left subtree are
    // shortened in the process
    //
    // This code assumes that `n` is not `nullptr` and is in the tree; it can sometimes
    // detect that `n` is not in the tree and throw an exception.
    void splay(T* n);

    // Return the first element in the tree witout splaying.
    //
    // Returns a reference to the head event.
    // Avoids splaying but just searches for and returns a pointer to
    // the bottom of the left branch.
    T* fast_first() const;

    // Fast return next higher item in the tree, or nullptr
    //
    // Return the successor of `n`, represented as a splay tree.
    // This is a fast (on average) version that does not splay.
    T* fast_next(T* n) const;

    // The node at the top of the tree
    T* root{};

    // Number of comparison in enqueue
    int enqcmps{};
};

template <typename T>
bool SPTree<T>::empty() const {
    return root == nullptr;
}

template <typename T>
int SPTree<T>::get_enqcmps() const {
    return enqcmps;
}

template <typename T>
void SPTree<T>::enqueue(T* n) {
    T* left;  /* the rightmost node in the left tree */
    T* right; /* the leftmost node in the right tree */
    T* next;  /* the root of the unsplit part */
    T* temp;

    double key;

    n->uplink = nullptr;
    next = root;
    root = n;
    if (next == nullptr) /* trivial enq */
    {
        n->leftlink = nullptr;
        n->rightlink = nullptr;
    } else /* difficult enq */
    {
        key = n->key;
        left = n;
        right = n;

        /* n's left and right children will hold the right and left
       splayed trees resulting from splitting on n->key;
       note that the children will be reversed! */

        enqcmps++;
        if (next->key - key > 0) {
            goto two;
        }

    one:   /* assert next->key <= key */
        do /* walk to the right in the left tree */
        {
            temp = next->rightlink;
            if (temp == nullptr) {
                left->rightlink = next;
                next->uplink = left;
                right->leftlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            enqcmps++;
            if (temp->key - key > 0) {
                left->rightlink = next;
                next->uplink = left;
                left = next;
                next = temp;
                goto two; /* change sides */
            }

            next->rightlink = temp->leftlink;
            if (temp->leftlink != nullptr) {
                temp->leftlink->uplink = next;
            }
            left->rightlink = temp;
            temp->uplink = left;
            temp->leftlink = next;
            next->uplink = temp;
            left = temp;
            next = temp->rightlink;
            if (next == nullptr) {
                right->leftlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            enqcmps++;

        } while (next->key - key <= 0); /* change sides */

    two:   /* assert next->key > key */
        do /* walk to the left in the right tree */
        {
            temp = next->leftlink;
            if (temp == nullptr) {
                right->leftlink = next;
                next->uplink = right;
                left->rightlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            enqcmps++;
            if (temp->key - key <= 0) {
                right->leftlink = next;
                next->uplink = right;
                right = next;
                next = temp;
                goto one; /* change sides */
            }
            next->leftlink = temp->rightlink;
            if (temp->rightlink != nullptr) {
                temp->rightlink->uplink = next;
            }
            right->leftlink = temp;
            temp->uplink = right;
            temp->rightlink = next;
            next->uplink = temp;
            right = temp;
            next = temp->leftlink;
            if (next == nullptr) {
                left->rightlink = nullptr;
                goto done; /* job done, entire tree split */
            }

            enqcmps++;

        } while (next->key - key > 0); /* change sides */

        goto one;

    done: /* split is done, branches of `n` need reversal */
        temp = n->leftlink;
        n->leftlink = n->rightlink;
        n->rightlink = temp;
    }
}

template <typename T>
T* SPTree<T>::dequeue() {
    return dequeue(&root);
}

template <typename T>
T* SPTree<T>::dequeue(T** np) /* pointer to a node pointer */
{
    T* deq;        /* one to return */
    T* next;       /* the next thing to deal with */
    T* left;       /* the left child of next */
    T* farleft;    /* the left child of left */
    T* farfarleft; /* the left child of farleft */

    if (np == nullptr || *np == nullptr) {
        deq = nullptr;
    } else {
        next = *np;
        left = next->leftlink;
        if (left == nullptr) {
            deq = next;
            *np = next->rightlink;

            if (*np != nullptr) {
                (*np)->uplink = nullptr;
            }
        } else {
            for (;;) /* left is not null */
            {
                /* next is not it, left is not nullptr, might be it */
                farleft = left->leftlink;
                if (farleft == nullptr) {
                    deq = left;
                    next->leftlink = left->rightlink;
                    if (left->rightlink != nullptr)
                        left->rightlink->uplink = next;
                    break;
                }

                /* next, left are not it, farleft is not nullptr, might be it */
                farfarleft = farleft->leftlink;
                if (farfarleft == nullptr) {
                    deq = farleft;
                    left->leftlink = farleft->rightlink;
                    if (farleft->rightlink != nullptr)
                        farleft->rightlink->uplink = left;
                    break;
                }

                /* next, left, farleft are not it, rotate */
                next->leftlink = farleft;
                farleft->uplink = next;
                left->leftlink = farleft->rightlink;
                if (farleft->rightlink != nullptr)
                    farleft->rightlink->uplink = left;
                farleft->rightlink = left;
                left->uplink = farleft;
                next = farleft;
                left = farfarleft;
            }
        }
    }

    return deq;
}

template <typename T>
void SPTree<T>::splay(T* n) {
    T* up;     /* points to the node being dealt with */
    T* prev;   /* a descendent of up, already dealt with */
    T* upup;   /* the parent of up */
    T* upupup; /* the grandparent of up */
    T* left;   /* the top of left subtree being built */
    T* right;  /* the top of right subtree being built */


    left = n->leftlink;
    right = n->rightlink;
    prev = n;
    up = prev->uplink;

    while (up != nullptr) {
        /* walk up the tree towards the root, splaying all to the left of
           `n` into the left subtree, all to right into the right subtree */
        upup = up->uplink;
        if (up->leftlink == prev) /* up is to the right of n */
        {
            if (upup != nullptr && upup->leftlink == up) /* rotate */
            {
                upupup = upup->uplink;
                upup->leftlink = up->rightlink;
                if (upup->leftlink != nullptr) {
                    upup->leftlink->uplink = upup;
                }
                up->rightlink = upup;
                upup->uplink = up;
                if (upupup == nullptr) {
                    root = up;
                } else if (upupup->leftlink == upup) {
                    upupup->leftlink = up;
                } else {
                    upupup->rightlink = up;
                }
                up->uplink = upupup;
                upup = upupup;
            }
            up->leftlink = right;
            if (right != nullptr) {
                right->uplink = up;
            }
            right = up;

        } else /* up is to the left of `n` */
        {
            if (upup != nullptr && upup->rightlink == up) /* rotate */
            {
                upupup = upup->uplink;
                upup->rightlink = up->leftlink;
                if (upup->rightlink != nullptr) {
                    upup->rightlink->uplink = upup;
                }
                up->leftlink = upup;
                upup->uplink = up;
                if (upupup == nullptr) {
                    root = up;
                } else if (upupup->rightlink == upup) {
                    upupup->rightlink = up;
                } else {
                    upupup->leftlink = up;
                }
                up->uplink = upupup;
                upup = upupup;
            }
            up->rightlink = left;
            if (left != nullptr) {
                left->uplink = up;
            }
            left = up;
        }
        prev = up;
        up = upup;
    }

#ifdef DEBUG
    if (root != prev) {
        /*fprintf(stderr, " *** bug in splay: n not in q *** " ); */
        abort();
    }
#endif

    n->leftlink = left;
    n->rightlink = right;
    if (left != nullptr) {
        left->uplink = n;
    }
    if (right != nullptr) {
        right->uplink = n;
    }
    root = n;
    n->uplink = nullptr;
}

template <typename T>
T* SPTree<T>::first() {
    if (empty()) {
        return nullptr;
    }

    T* x = dequeue();
    x->rightlink = root;
    x->leftlink = nullptr;
    x->uplink = nullptr;
    if (root != nullptr) {
        root->uplink = x;
    }
    root = x;

    return x;
}

template <typename T>
void SPTree<T>::remove(T* n) {
    splay(n);
    T* x = dequeue(&root->rightlink);
    if (x == nullptr) /* empty right subtree */
    {
        root = root->leftlink;
        if (root) {
            root->uplink = nullptr;
        }
    } else /* non-empty right subtree */
    {
        x->uplink = nullptr;
        x->leftlink = root->leftlink;
        x->rightlink = root->rightlink;
        if (x->leftlink != nullptr) {
            x->leftlink->uplink = x;
        }
        if (x->rightlink != nullptr) {
            x->rightlink->uplink = x;
        }
        root = x;
    }
}

template <typename T>
T* SPTree<T>::find(double key) {
    T* n = root;
    while (n && key != n->key) {
        n = key < n->key ? n->leftlink : n->rightlink;
    }

    /* reorganize tree around this node */
    if (n != nullptr) {
        splay(n);
    }

    return n;
}

template <typename T>
void SPTree<T>::apply_all(void (*f)(const T*, int), T* n) const {
    for (T* x = n != nullptr ? n : fast_first(); x != nullptr; x = fast_next(x)) {
        std::invoke(f, x, 0);
    }
}

template <typename T>
T* SPTree<T>::fast_first() const {
    if (empty()) {
        return nullptr;
    }

    T* x = root;
    while (x->leftlink != nullptr) {
        x = x->leftlink;
    }
    return x;
}

template <typename T>
T* SPTree<T>::fast_next(T* n) const {
    if (n == nullptr) {
        return n;
    }

    // If there is a right element, the next element is the smallest item on the most left of this
    // right element
    if (T* x = n->rightlink; x != nullptr) {
        while (x->leftlink != nullptr) {
            x = x->leftlink;
        }
        return x;
    }

    // Otherwise we have to go up and left
    T* next = nullptr;
    T* x = n->uplink;
    while (x != nullptr) {
        if (x->leftlink == n) {
            next = x;
            x = nullptr;
        } else {
            n = x;
            x = n->uplink;
        }
    }

    return next;
}
