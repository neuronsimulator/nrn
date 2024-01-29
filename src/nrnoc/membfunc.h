#pragma once
extern void hoc_register_prop_size(int type, int psize, int dpsize);

#include "neuron/container/data_handle.hpp"
#include "nrnoc_ml.h"
#include "oc_ansi.h"  // neuron::model_sorted_token
#include "options.h"  // EXTRACELLULAR

#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

typedef struct NrnThread NrnThread;
struct Extnode;
struct Section;
struct Symbol;

typedef Datum* (*Pfrpdat)();
typedef void (*Pvmi)(struct NrnThread*, Memb_list*, int);
typedef void (*Pvmp)(Prop*);
typedef int (*nrn_ode_count_t)(int);
using nrn_bamech_t = void (*)(Node*,
                              Datum*,
                              Datum*,
                              NrnThread*,
                              Memb_list*,
                              std::size_t,
                              neuron::model_sorted_token const&);
using nrn_cur_t = void (*)(neuron::model_sorted_token const&, NrnThread*, Memb_list*, int);
using nrn_init_t = nrn_cur_t;
using nrn_jacob_t = nrn_cur_t;
using nrn_ode_map_t = void (*)(Prop*,
                               int /* ieq */,
                               neuron::container::data_handle<double>* /* pv (std::span) */,
                               neuron::container::data_handle<double>* /* pvdot (std::span) */,
                               double* /* atol */,
                               int /* type */);
using nrn_ode_matsol_t = nrn_cur_t;
using nrn_ode_spec_t = nrn_cur_t;
using nrn_ode_synonym_t = void (*)(neuron::model_sorted_token const&, NrnThread&, Memb_list&, int);
using nrn_state_t = nrn_cur_t;
using nrn_thread_table_check_t = void (*)(Memb_list*,
                                          std::size_t,
                                          Datum*,
                                          Datum*,
                                          NrnThread*,
                                          int,
                                          neuron::model_sorted_token const&);

#define NULL_CUR        (Pfri) 0
#define NULL_ALLOC      (Pfri) 0
#define NULL_STATE      (Pfri) 0
#define NULL_INITIALIZE (Pfri) 0

struct Memb_func {
    Pvmp alloc;
    nrn_cur_t current;
    nrn_jacob_t jacob;
    nrn_state_t state;
    bool has_initialize() const {
        return m_initialize;
    }
    void invoke_initialize(neuron::model_sorted_token const& sorted_token,
                           NrnThread* nt,
                           Memb_list* ml,
                           int type) const;
    void set_initialize(nrn_init_t init) {
        m_initialize = init;
    }
    Pvmp destructor; /* only for point processes */
    Symbol* sym;
    nrn_ode_count_t ode_count;
    nrn_ode_map_t ode_map;
    nrn_ode_spec_t ode_spec;
    nrn_ode_matsol_t ode_matsol;
    nrn_ode_synonym_t ode_synonym;
    Pvmi singchan_; /* managed by kschan for variable step methods */
    int vectorized;
    int thread_size_;                 /* how many Datum needed in Memb_list if vectorized */
    void (*thread_mem_init_)(Datum*); /* after Memb_list._thread is allocated */
    void (*thread_cleanup_)(Datum*);  /* before Memb_list._thread is freed */
    nrn_thread_table_check_t thread_table_check_;
    int is_point;
    void* hoc_mech;
    void (*setdata_)(struct Prop*);
    int* dparam_semantics;  // for nrncore writing.
  private:
    nrn_init_t m_initialize{};
};

/* Direct call Python wrappers to density mechanism functions */
struct NPyDirectMechFunc {
    const char* name;
    double (*func)(Prop*);
};
/* Above struct in translated mod files are elements of a {nullptr, nullptr}
   terminated list. The translator could easily create an unordered map instead,
   and that would be nicer.
   However there is some question (as with std::string) that an unordered_map
   is ABI compatible across codes compiled by different toolchains.
   So we will build our per mechanism map in NEURON world from the null
   terminated list in NMODL world.
*/
using NPyDirectMechFuncs = std::unordered_map<std::string, NPyDirectMechFunc*>;
extern void hoc_register_npy_direct(int type, NPyDirectMechFunc*);
extern std::unordered_map<int, NPyDirectMechFuncs> nrn_mech2funcs_map;

#define IMEMFAST     -2
#define VINDEX       -1
#define CABLESECTION 1
#define MORPHOLOGY   2
#define CAP          3
#if EXTRACELLULAR
#define EXTRACELL 5
extern int nrn_nlayer_extracellular;
namespace neuron::extracellular {
// these are the fields in the standard NEURON mechanism data structures
static constexpr auto xraxial_index = 0;          // array of size nlayer
static constexpr auto xg_index = 1;               // array of size nlayer
static constexpr auto xc_index = 2;               // array of size nlayer
static constexpr auto e_extracellular_index = 3;  // scalar
static constexpr auto i_membrane_index = 4;       // scalar, might not exist
static constexpr auto sav_g_index = 5;            // scalar, might not exist
static constexpr auto sav_rhs_index = 6;          // scalar, might not exist

// these are the indices into the Extnode param array
inline std::size_t xraxial_index_ext(std::size_t ilayer) {
    return ilayer;
}
inline std::size_t xg_index_ext(std::size_t ilayer) {
    return nrn_nlayer_extracellular + ilayer;
}
inline std::size_t xc_index_ext(std::size_t ilayer) {
    return 2 * nrn_nlayer_extracellular + ilayer;
}
inline std::size_t e_extracellular_index_ext() {
    return 3 * nrn_nlayer_extracellular;
}
#if I_MEMBRANE
inline std::size_t sav_rhs_index_ext() {
    return 3 * nrn_nlayer_extracellular + 3;
}
#endif
inline std::size_t vext_pseudoindex() {
#if I_MEMBRANE
    return 3 * nrn_nlayer_extracellular + 4;
#else
    return 3 * nrn_nlayer_extracellular + 1;
#endif
}
}  // namespace neuron::extracellular
#endif

#define nrnocCONST 1  // PARAMETER
#define DEP        2  // ASSIGNED
#define STATE      3 /* STATE: See init.cpp and cabvars.h for order of nrnocCONST, DEP, and STATE */
#define NRNPOINTER                                                            \
    4 /* added on to list of mechanism variables.These are                    \
pointers which connect variables  from other mechanisms via the _ppval array. \
*/

#define _AMBIGUOUS 5  // for Ions
#define RAND       6  // RANDOM

#define BEFORE_INITIAL    0
#define AFTER_INITIAL     1
#define BEFORE_BREAKPOINT 2
#define AFTER_SOLVE       3
#define BEFORE_STEP       4
#define BEFORE_AFTER_SIZE 5 /* 1 more than the previous */
typedef struct BAMech {
    nrn_bamech_t f;
    int type;
    struct BAMech* next;
} BAMech;
extern BAMech** bamech_;

extern std::vector<Memb_func> memb_func;
extern int n_memb_func;
extern int* nrn_prop_param_size_;
extern int* nrn_prop_dparam_size_;

extern std::vector<Memb_list> memb_list;
/* for finitialize, order is same up through extracellular, then ions,
then mechanisms that write concentrations, then all others. */
extern short* memb_order_;

namespace neuron::mechanism {
template <typename T>
struct field {
    using type = T;
    field(std::string name_)
        : name{std::move(name_)} {}
    field(std::string name_, int array_size_)
        : name{std::move(name_)}
        , array_size{array_size_} {}
    field(std::string name_, std::string semantics_)
        : name{std::move(name_)}
        , semantics{std::move(semantics_)} {}
    int array_size{1};
    std::string name{}, semantics{};
};
namespace detail {
void register_data_fields(int mech_type,
                          std::vector<std::pair<const char*, int>> const& param_info,
                          std::vector<std::pair<const char*, const char*>> const& dparam_size);
}  // namespace detail
/**
 * @brief Type- and array-aware version of hoc_register_prop_size.
 *
 * hoc_register_prop_size did not propagate enough information to know which parts of the "data"
 * size were ranges corresponding to a single array variable. This also aims to be ready for
 * supporting multiple variable data types in MOD files.
 */
// This is declared static on purpose. When we are using binary Python wheels and a user runs
// nrnivmodl on their local machine then we link together a libnrniv.so from the binary wheel
// with object files that were produced from .mod files on the user's machine. If the files
// compiled on the user's machine have the same number/type of variables as a built in mechanism
// that is embedded in libnrniv.so then there will be two calls to this function with the same
// template arguments. If this function was not static, those would have external linkage and a
// call from one of the user's .mod files might use a definition of this function from
// libnrniv.so. This can cause problems due to ABI mismatches, and the ::detail version above
// essentially exists to ensure that *it* is the interface between libnrniv.so and code compiled
// on the user's machine. Yes, this is horrible. See #1963 and #2234 for more information.
template <typename... Fields>
static void register_data_fields(int mech_type, Fields const&... fields) {
    // Use of const char* aims to avoid wheel ABI issues with std::string
    std::vector<std::pair<const char*, int>> param_info{};
    std::vector<std::pair<const char*, const char*>> dparam_info{};
    auto const process = [&](auto const& field) {
        using field_t = std::decay_t<decltype(field)>;
        using data_t = typename field_t::type;
        if constexpr (std::is_same_v<data_t, double>) {
            assert(field.semantics.empty());
            param_info.emplace_back(field.name.c_str(), field.array_size);
        } else {
            static_assert(std::is_same_v<data_t, int> /* TODO */ || std::is_pointer_v<data_t>,
                          "only pointers, doubles and ints are supported");
            assert(field.array_size == 1);  // only scalar dparam data is supported
            dparam_info.emplace_back(field.name.c_str(), field.semantics.c_str());
        }
    };
    // fold expression with the comma operator is neater, but hits AppleClang expression depth
    // limits for large sizeof...(Fields); the old initializer_list trick avoids that.
    static_cast<void>(std::initializer_list<int>{(static_cast<void>(process(fields)), 0)...});
    // beware, the next call crosses from translated mechanism code into the main NEURON library
    detail::register_data_fields(mech_type, param_info, dparam_info);
}

/**
 * @brief Get the number of fields (some of which may be arrays) of the given type.
 *
 * If the given mechanism type is negative, -1 will be returned.
 */
template <typename>
[[nodiscard]] int get_field_count(int mech_type);

/**
 * @brief Pointer to a range of pointers to the start of contiguous storage ranges.
 *
 * If the given mechanism type is negative, nullptr will be returned.
 */
template <typename T>
[[nodiscard]] T* const* get_data_ptrs(int mech_type);

/**
 * @brief Get the array dimensions for fields of the given type.
 *
 * This forms part of the API used by translated MOD file code to access the
 * mechanism data managed by NEURON. It serves to help hide the implementation
 * of the mechanism data storage from translated MOD file code and reduce ABI
 * compatibility issues arising from Python wheel support.
 *
 * If the given mechanism type is negative, nullptr will be returned.
 */
template <typename>
[[nodiscard]] int const* get_array_dims(int mech_type);
namespace _get {
[[nodiscard]] std::size_t _current_row(Prop*);
[[nodiscard]] std::vector<double* const*> const& _pdata_ptr_cache_data(
    neuron::model_sorted_token const& cache_token,
    int mech_type);
}  // namespace _get
}  // namespace neuron::mechanism

// See https://github.com/neuronsimulator/nrn/issues/2234 for context of how this might be done
// better in future...
[[nodiscard]] long& _nrn_mechanism_access_alloc_seq(Prop*);
[[nodiscard]] double& _nrn_mechanism_access_a(Node*);
[[nodiscard]] double& _nrn_mechanism_access_b(Node*);
[[nodiscard]] double& _nrn_mechanism_access_d(Node*);
[[nodiscard]] neuron::container::generic_data_handle*& _nrn_mechanism_access_dparam(Prop*);
[[nodiscard]] Extnode*& _nrn_mechanism_access_extnode(Node*);
[[nodiscard]] double& _nrn_mechanism_access_param(Prop*, int field, int array_index = 0);
[[nodiscard]] double& _nrn_mechanism_access_rhs(Node*);
[[nodiscard]] double& _nrn_mechanism_access_voltage(Node*);
[[nodiscard]] neuron::container::data_handle<double> _nrn_mechanism_get_area_handle(Node*);
[[nodiscard]] Section* _nrn_mechanism_get_child(Section*);
[[nodiscard]] int _nrn_mechanism_get_nnode(Section*);
[[nodiscard]] Node* _nrn_mechanism_get_node(Section*, int);
[[nodiscard]] int _nrn_mechanism_get_num_vars(Prop*);
[[nodiscard]] neuron::container::data_handle<double> _nrn_mechanism_get_param_handle(
    Prop* prop,
    neuron::container::field_index field);
[[nodiscard]] inline neuron::container::data_handle<double>
_nrn_mechanism_get_param_handle(Prop* prop, int field, int array_index = 0) {
    return _nrn_mechanism_get_param_handle(prop,
                                           neuron::container::field_index{field, array_index});
}
[[nodiscard]] Section* _nrn_mechanism_get_sibling(Section*);
[[nodiscard]] NrnThread* _nrn_mechanism_get_thread(Node*);
[[nodiscard]] int _nrn_mechanism_get_type(Prop*);
[[nodiscard]] int _nrn_mechanism_get_v_node_index(Node*);
