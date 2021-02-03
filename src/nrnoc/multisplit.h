#ifndef multisplit_h
#define multisplit_h

#include <nrnmpiuse.h>

#if 0 /* comment */

in the classical order, knowing a node means you know the classical parent with
v_parent[node->v_node_index]. Also the effect of the parent on the node equation
is given by NODEB(node) and the effect of the node on the parent equation is
NODEA(node).

One major circumstance of the multisplit order is that a parent child may
be reversed. For a single sid0, this happens on the path between the classical
root and sid0. If a parent-child has been reversed and given a node we know
the classical parent (presently a child) then at the node
ClassicalB = ClassicalParentPresentA
ClassicalA = ClassicalParentPresentB

The other aspect of multisplit, both sid0 and sid1 exist, is more complex
because of the two parent nature of the center node on the path between sid0
and sid1. Our convention is that v_parent[centernode->v_node_index] is toward
sid0. And the other parent is
v_parent[c2sid1_parent_index[centernode->v_node_index - backbone_center_begin]]
So we need to know what Classical A and B are for the center node and its
two parents. A center node may be reversed or not with respect to its classical
parent. One of the present parents may or may not be its classical parent.

#endif /* end comment */


#if 1 || PARANEURON
extern double* nrn_classicalNodeA(Node* n);
extern double* nrn_classicalNodeB(Node* n);
#define ClassicalNODEA(n) (*nrn_classicalNodeA(n))
#define ClassicalNODEB(n) (*nrn_classicalNodeB(n))
#else
#define ClassicalNODEA(n) NODEA(n)
#define ClassicalNODEB(n) NODEB(n)
#endif


#endif /* multisplit_h */
