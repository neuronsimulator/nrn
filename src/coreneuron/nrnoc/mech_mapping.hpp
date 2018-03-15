#ifndef MECH_MAPPING_H
#define MECH_MAPPING_H

/*
 * todo : currently mod2c has exactly 4 different variable categories
 * that are registered to coreneuron.
 */
#define NB_MECH_VAR_CATEGORIES 4

/*
 * SerializedNames
 *
 * names are passed serialized using the following format:
 * SerializedNames : {"0",[[<CategorieNames>,]*0,]* [[<CategorieNames>,]* 0]}
 * All categories must be filled, if they are emtpy, just an other 0 follow.
 *
 * ex: {"0", "name1", "name2", 0, "name3, "name4", 0,0,0}
 *     This means the first categorie with names {name1,name2},
 *     the second categorie with {name3, name4}, 2 last categories are empty
 */
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

struct Memb_list;

typedef const char** SerializedNames;

// return pointer to value of a variable's mechanism, or nullptr if not found
EXTERN_C double* get_var_location_from_var_name(int mech_id,
                                                const char* variable_name,
                                                Memb_list* ml,
                                                int local_index);

// initialize mapping of variable names of mechanism, to their places in memory
EXTERN_C void register_all_variables_offsets(int mech_id, SerializedNames variable_names);

#undef EXTERN_C
#endif
