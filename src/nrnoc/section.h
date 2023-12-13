/* /local/src/master/nrn/src/nrnoc/section.h,v 1.4 1996/05/21 17:09:24 hines Exp */
#pragma once

/* In order to support oc objects containing sections, instead of vector
    of ordered sections, we now have a list (in the nmodl sense)
    of unordered sections. The lesser efficiency is ok because the
    number crunching is vectorized. ie only the user interface deals
    with sections and that needs to be convenient
*/
/* Data structure for solving branching 1-D tree diffusion type equations.
    Vector of ordered sections each of which points to a vector of nodes.
    Each section must have at least one node. There may be 0 sections.
    The order of last node to first node is used in triangularization.
    First node to last is used in back substitution.
    The first node of a section is connected to some node of a section
      with lesser index.
*/
/* An equation is associated with each node. d and rhs are the diagonal and
   right hand side respectively.  a is the effect of this node on the parent
   node's equation.  b is the effect of the parent node on this node's
   equation.
   d is assumed to be non-zero.
   d and rhs is calculated from the property list.
*/
#include "hoclist.h"
#include "membfunc.h"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/model_data.hpp"
#include "nrnredef.h"
#include "options.h"
#include "section_fwd.hpp"

/*#define DEBUGSOLVE 1*/
#define xpop      hoc_xpop
#define pc        hoc_pc
#define spop      hoc_spop
#define execerror hoc_execerror
#include "hocdec.h"

#include <optional>

#if DIAMLIST
struct Pt3d {
    float x, y, z, d;  // 3d point, microns
    double arc;
};
#endif
struct Section {
    int refcount{};        // may be in more than one list
    short nnode{};         // Number of nodes for ith section
    Section* parentsec{};  // parent section of node 0
    Section* child{};      // root of the list of children connected to this parent kept in order of
                           // increasing x
    Section* sibling{};    // used as list of sections that have same parent

    Node* parentnode{};     // parent node; only valid when tree_changed = 0
    Node** pnode{};         // Pointer to pointer vector of node structures
    int order{};            // index of this in secorder vector
    short recalc_area_{};   // NODEAREA, NODERINV, diam, L need recalculation
    short volatile_mark{};  // for searching
    void* volatile_ptr{};   // e.g. ShapeSection*
#if DIAMLIST
    short npt3d{};               // number of 3-d points
    short pt3d_bsize{};          // amount of allocated space for 3-d points
    Pt3d* pt3d{};                // list of 3d points with diameter
    Pt3d* logical_connection{};  // nullptr for legacy, otherwise specifies logical connection
                                 // position (for translation)
#endif
    Prop* prop{};  // eg. length, etc.
};

typedef float NodeCoef;
typedef double NodeVal;

typedef struct Info3Coef {
    NodeVal current; /* for use in next time step */
    NodeVal djdv0;
    NodeCoef coef0;    /* 5dx/12 */
    NodeCoef coefn;    /* 1dx/12 */
    NodeCoef coefjdot; /* dx^2*ra/12 */
    NodeCoef coefdg;   /* dx/12 */
    NodeCoef coefj;    /* 1/(ra*dx) */
    struct Node* nd2;  /* the node dx away in the opposite direction*/
    /* note above implies that nodes next to branches cannot have point processes
    and still be third order correct. Also nodes next to branches cannot themselves
    be branch points */
} Info3Coef;

typedef struct Info3Val { /* storage to help build matrix efficiently */
    NodeVal GC;           /* doesn't include point processes */
    NodeVal EC;
    NodeCoef Cdt;
} Info3Val;


/* if any double is added after area then think about changing
the notify_free_val parameter in node_free in solve.cpp
*/
#define NODEAREA(n) ((n)->area())
#define NODERINV(n) ((n)->_rinv)

struct Extnode;
struct Node {
    // Eventually the old Node class should become an alias for
    // neuron::container::handle::Node, but as an intermediate measure we can
    // add one of those as a member and forward some access/modifications to it.
    neuron::container::Node::owning_handle _node_handle{neuron::model().node_data()};

    [[nodiscard]] auto id() {
        return _node_handle.id();
    }
    [[nodiscard]] auto& a() {
        return _node_handle.a();
    }
    [[nodiscard]] auto const& a() const {
        return _node_handle.a();
    }
    [[nodiscard]] auto& area() {
        return _node_handle.area_hack();
    }
    [[nodiscard]] auto const& area() const {
        return _node_handle.area_hack();
    }
    [[nodiscard]] auto area_handle() {
        return _node_handle.area_handle();
    }
    [[nodiscard]] auto& b() {
        return _node_handle.b();
    }
    [[nodiscard]] auto const& b() const {
        return _node_handle.b();
    }
    [[nodiscard]] auto& d() {
        return _node_handle.d();
    }
    [[nodiscard]] auto const& d() const {
        return _node_handle.d();
    }
    [[nodiscard]] auto& v() {
        return _node_handle.v_hack();
    }
    [[nodiscard]] auto const& v() const {
        return _node_handle.v_hack();
    }
    [[nodiscard]] auto& v_hack() {
        return _node_handle.v_hack();
    }
    [[nodiscard]] auto const& v_hack() const {
        return _node_handle.v_hack();
    }
    [[nodiscard]] auto v_handle() {
        return _node_handle.v_handle();
    }
    [[nodiscard]] auto& rhs() {
        return _node_handle.rhs();
    }
    [[nodiscard]] auto const& rhs() const {
        return _node_handle.rhs();
    }
    [[nodiscard]] auto rhs_handle() {
        return _node_handle.rhs_handle();
    }
    [[nodiscard]] auto& sav_d() {
        return _node_handle.sav_d();
    }
    [[nodiscard]] auto const& sav_d() const {
        return _node_handle.sav_d();
    }
    [[nodiscard]] auto& sav_rhs() {
        return _node_handle.sav_rhs();
    }
    [[nodiscard]] auto const& sav_rhs() const {
        return _node_handle.sav_rhs();
    }
    [[nodiscard]] auto sav_rhs_handle() {
        return _node_handle.sav_rhs_handle();
    }
    [[nodiscard]] auto non_owning_handle() {
        return _node_handle.non_owning_handle();
    }
    double _rinv{}; /* conductance uS from node to parent */
    double* _a_matelm;
    double* _b_matelm;
    double* _d_matelm;
    int eqn_index_;                 /* sparse13 matrix row/col index */
                                    /* if no extnodes then = v_node_index +1*/
                                    /* each extnode adds nlayer more equations after this */
    Prop* prop{};                   /* Points to beginning of property list */
    Section* child;                 /* section connected to this node */
                                    /* 0 means no other section connected */
    Section* sec;                   /* section this node is in */
                                    /* #if NRNMPI */
    struct Node* _classical_parent; /* needed for multisplit */
    struct NrnThread* _nt;
/* #endif */
#if EXTRACELLULAR
    Extnode* extnode{};
#endif

#if EXTRAEQN
    Eqnblock* eqnblock{}; /* hook to other equations which
           need to be solved at the same time as the membrane
           potential. eg. fast changeing ionic concentrations */
#endif

#if DEBUGSOLVE
    double savd;
    double savrhs;
#endif                   /*DEBUGSOLVE*/
    int v_node_index;    /* only used to calculate parent_node_indices*/
    int sec_node_index_; /* to calculate segment index from *Node */
    Node() = default;
    Node(Node const&) = delete;
    Node(Node&&) = default;
    Node& operator=(Node const&) = delete;
    Node& operator=(Node&&) = default;
    ~Node();
    friend std::ostream& operator<<(std::ostream& os, Node const& n) {
        return os << n._node_handle;
    }
};

#if !INCLUDEHOCH
#include "hocdec.h" /* Prop needs Datum and Datum needs Symbol */
#endif

#define PROP_PY_INDEX 10
struct Prop {
    // Working assumption is that we can safely equate "Prop" with "instance
    // of a mechanism" apart from a few special cases like CABLESECTION
    Prop(short type)
        : _type{type} {
        if (type != CABLESECTION) {
            m_mech_handle = neuron::container::Mechanism::owning_handle{
                neuron::model().mechanism_data(type)};
        }
    }
    Prop* next;      /* linked list of properties */
    short _type;     /* type of membrane, e.g. passive, HH, etc. */
    int dparam_size; /* for notifying hoc_free_val_array */
    // double* param;     /* vector of doubles for this property */
    Datum* dparam;   /* usually vector of pointers to doubles
                of other properties but maybe other things as well
                for example one cable section property is a
                symbol */
    long _alloc_seq; /* for cache efficiency */
    Object* ob;      /* nullptr if normal property, otherwise the object containing the data*/

    /** @brief Get the identifier of this instance.
     */
    [[nodiscard]] auto id() const {
        assert(m_mech_handle);
        return m_mech_handle->id_hack();
    }

    /**
     * @brief Check if the given handle refers to data owned by this Prop.
     */
    [[nodiscard]] bool owns(neuron::container::data_handle<double> const& handle) const {
        assert(m_mech_handle);
        auto const num_fpfields = m_mech_handle->num_fpfields();
        auto* const raw_ptr = static_cast<double const*>(handle);
        for (auto i = 0; i < num_fpfields; ++i) {
            for (auto j = 0; j < m_mech_handle->fpfield_dimension(i); ++j) {
                if (raw_ptr == &m_mech_handle->fpfield(i, j)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Return a reference to the i-th floating point data field associated with this Prop.
     *
     * Note that there is a subtlety with the numbering scheme in case of array variables.
     * If we have 3 array variables (a, b, c) with dimensions x, y, z:
     *   a[x] b[y] c[z]
     * then, for example, the second element of b (assume y >= 2) is obtained with param(1, 1).
     * In AoS NEURON these values were all stored contiguously, and the values were obtained using
     * a single index; taking the same example, the second element of b used to be found at index
     * x + 1 in the param array. In all of the above, scalar variables are treated the same and
     * simply have dimension 1. In SoA NEURON then a[1] is stored immediately after a[0] in memory,
     * but for a given mechanism instance b[0] is **not** stored immediately after a[x-1].
     *
     * It is possible, but a little inefficient, to calculate the new pair of indices from an old
     * index. For that, see the param_legacy and param_handle_legacy functions.
     */
    [[nodiscard]] double& param(int field_index, int array_index = 0) {
        assert(m_mech_handle);
        return m_mech_handle->fpfield(field_index, array_index);
    }

    /**
     * @brief Return a reference to the i-th double value associated with this Prop.
     *
     * See the discussion above about numbering schemes.
     */
    [[nodiscard]] double const& param(int field_index, int array_index = 0) const {
        assert(m_mech_handle);
        return m_mech_handle->fpfield(field_index, array_index);
    }

    /**
     * @brief Return a handle to the i-th double value associated with this Prop.
     *
     * See the discussion above about numbering schemes.
     */
    [[nodiscard]] auto param_handle(int field, int array_index = 0) {
        assert(m_mech_handle);
        return m_mech_handle->fpfield_handle(field, array_index);
    }

    [[nodiscard]] auto param_handle(neuron::container::field_index ind) {
        return param_handle(ind.field, ind.array_index);
    }

  private:
    /**
     * @brief Translate a legacy (flat) index into a (variable, array offset) pair.
     * @todo Reimplement this using the new helpers.
     */
    [[nodiscard]] std::pair<int, int> translate_legacy_index(int legacy_index) const {
        assert(m_mech_handle);
        int total{};
        auto const num_fields = m_mech_handle->num_fpfields();
        for (auto field = 0; field < num_fields; ++field) {
            auto const array_dim = m_mech_handle->fpfield_dimension(field);
            if (legacy_index < total + array_dim) {
                auto const array_index = legacy_index - total;
                return {field, array_index};
            }
            total += array_dim;
        }
        throw std::runtime_error("could not translate legacy index " +
                                 std::to_string(legacy_index));
    }

  public:
    [[nodiscard]] double& param_legacy(int legacy_index) {
        auto const [array_dim, array_index] = translate_legacy_index(legacy_index);
        return param(array_dim, array_index);
    }

    [[nodiscard]] double const& param_legacy(int legacy_index) const {
        auto const [array_dim, array_index] = translate_legacy_index(legacy_index);
        return param(array_dim, array_index);
    }

    [[nodiscard]] auto param_handle_legacy(int legacy_index) {
        auto const [array_dim, array_index] = translate_legacy_index(legacy_index);
        return param_handle(array_dim, array_index);
    }

    /**
     * @brief Return how many double values are assocated with this Prop.
     *
     * In case of array variables, this is the sum over array dimensions.
     * i.e. if a mechanism has a[2] b[2] then param_size()=4 and param_num_vars()=2.
     */
    [[nodiscard]] int param_size() const {
        assert(m_mech_handle);
        return m_mech_handle->fpfields_size();
    }

    /**
     * @brief Return how many (possibly-array) variables are associated with this Prop.
     *
     * In case of array variables, this ignores array dimensions.
     * i.e. if a mechanism has a[2] b[2] then param_size()=4 and param_num_vars()=2.
     */
    [[nodiscard]] int param_num_vars() const {
        assert(m_mech_handle);
        return m_mech_handle->num_fpfields();
    }

    /**
     * @brief Return the array dimension of the given value.
     */
    [[nodiscard]] int param_array_dimension(int field) const {
        assert(m_mech_handle);
        return m_mech_handle->fpfield_dimension(field);
    }

    [[nodiscard]] std::size_t current_row() const {
        assert(m_mech_handle);
        return m_mech_handle->current_row();
    }

    friend std::ostream& operator<<(std::ostream& os, Prop const& p) {
        if (p.m_mech_handle) {
            return os << *p.m_mech_handle;
        } else {
            return os << "Prop{nullopt}";
        }
    }

  private:
    // This is a handle that owns a row of the ~global mechanism data for
    // `_type`. Usage of `param` and `param_size` should be replaced with
    // indirection through this.
    std::optional<neuron::container::Mechanism::owning_handle> m_mech_handle;
};

extern void nrn_prop_datum_free(int type, Datum* ppd);
extern void nrn_delete_mechanism_prop_datum(int type);
extern int nrn_mechanism_prop_datum_count(int type);

#if EXTRAEQN
/*Blocks of equations can hang off each node of the current conservation
equations. These are equations which must be solved simultaneously
because they depend on the voltage and affect the voltage. An example
are fast changing ionic concentrations (or merely if we want to be
able to calculate steady states using a stable method).
*/
typedef struct Eqnblock {
    struct Eqnblock* eqnblock_next; /* may be several such blocks */
    Pfri eqnblock_triang;           /* triangularization function */
    Pfri eqnblock_bksub;            /* back substitution function */
    double* eqnblock_data;
#if 0
	the solving functions know how to find the following info from
	the eqnblock_data.
	double *eqnblock_row;	/* current conservation depends on states */
	double *eqnblock_col;	/* state equations depend on voltage */
	double *eqnblock_matrix; /* state equations depend on states */
	double *eqnblock_rhs:
	the functions merely take a pointer to the node and this Eqnblock
	in order to update the values of the diagonal, v, and the rhs
	The interface with EXTRACELLULAR makes things a bit more subtle.
	It seems clear that we will have to change the meaning of v to
	be membrane potential so that the internal potential is v + vext.
	This will avoid requiring two rows and two columns since
	the state equations will depend only on v and not vext.
	In fact, if vext did not have longitudinal relationships with
	other vext the extracellular mechanism could be implemented in
	this style.
#endif
} Eqnblock;
#endif /*EXTRAEQN*/

extern int nrn_global_ncell;   /* note that for multiple threads all the rootnodes are no longer
                                  contiguous */
extern hoc_List* section_list; /* Where the Sections live */

extern Section* sec_alloc();             /* Allocates a single section */
extern void node_alloc(Section*, short); /* Allocates node vectors in a section*/
extern double section_length(Section*), nrn_diameter(Node*);
extern Node* nrn_parent_node(Node*);
extern Section* nrn_section_alloc();
extern void nrn_section_free(Section*);
extern int nrn_is_valid_section_ptr(void*);


#include <multicore.h>

#define tstopbit   (1 << 15)
#define tstopset   stoprun |= tstopbit
#define tstopunset stoprun &= (~tstopbit)
/* cvode.event(tevent) sets this. Reset at beginning */
/* of any hoc call for integration and before returning to hoc */


#include "nrn_ansi.h"
