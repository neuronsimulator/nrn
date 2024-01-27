#ifndef NRN_SECTION_MAPPING
#define NRN_SECTION_MAPPING

#include <string>
#include <vector>

/** @brief Section to segment mapping
 *
 *  For a section list (of particulat type), store mapping
 *  of section to segment. Note that the section is repeated
 *  when there are multiple segments in a section.
 */
struct SecMapping {
    /** number of sections in section list */
    int nsec;

    /** name of section list */
    std::string name;

    /** list of segments */
    std::vector<int> segments;

    /** list sections associated with each segment */
    std::vector<int> sections;

    /** list of lfp factors associated with each segment */
    std::vector<double> seglfp_factors;

    /** Number of electrodes per segment */
    int num_electrodes;

    SecMapping(int n, std::string s)
        : nsec(n)
        , name(s) {}

    size_t size() {
        return segments.size();
    }
};

/** @brief Compartment mapping information for a cell
 *
 * A cell can have multiple section list types like
 * soma, axon, apic, dend etc. User will add these
 * section lists using HOC interface.
 */
struct CellMapping {
    /** gid of a cell */
    int gid;

    /** list of section lists (like soma, axon, apic) */
    std::vector<SecMapping*> secmapping;

    CellMapping(int g, SecMapping* s) {
        gid = g;
        secmapping.push_back(s);
    }

    /** @brief total number of sections in a cell */
    int num_sections() {
        int nsec = 0;
        for (size_t i = 0; i < secmapping.size(); i++) {
            nsec += secmapping[i]->nsec;
        }
        return nsec;
    }

    /** @brief total number of segments in a cell */
    int num_segments() {
        int nseg = 0;
        for (size_t i = 0; i < secmapping.size(); i++) {
            nseg += secmapping[i]->segments.size();
        }
        return nseg;
    }

    /** @brief number of section lists */
    size_t size() {
        return secmapping.size();
    }

    ~CellMapping() {
        for (size_t i = 0; i < secmapping.size(); i++) {
            delete secmapping[i];
        }
    }
};

/** @brief Compartment mapping information for NrnThread
 *
 * NrnThread could have more than one cell in cellgroup
 * and we store this in vector.
 */
struct NrnMappingInfo {
    /** list of cells mapping */
    std::vector<CellMapping*> mapping;

    /** @brief number of cells */
    size_t size() {
        return mapping.size();
    }

    /** @brief after writing NrnThread to file we remove
     *	all previous mapping information, free memory.
     */
    void clear() {
        for (size_t i = 0; i < mapping.size(); i++) {
            delete mapping[i];
        }
        mapping.clear();
    }

    /** @brief memory cleanup */
    ~NrnMappingInfo() {
        for (size_t i = 0; i < mapping.size(); i++) {
            delete mapping[i];
        }
    }

    /** @brief get cell mapping information for given gid
     *	if exist otherwise return NULL.
     */
    CellMapping* get_cell_mapping(int gid) {
        for (int i = 0; i < mapping.size(); i++) {
            if (mapping[i]->gid == gid)
                return mapping[i];
        }
        return NULL;
    }

    /** @brief add section mapping information for given gid
     *	if cell is not peviously added, create new cell mapping.
     */
    void add_sec_mapping(int gid, SecMapping* s) {
        CellMapping* cm = get_cell_mapping(gid);

        if (cm == NULL) {
            CellMapping* c = new CellMapping(gid, s);
            mapping.push_back(c);
        } else {
            cm->secmapping.push_back(s);
        }
    }
};

void nrn_write_mapping_info(const char*, int, NrnMappingInfo&);

#endif  // NRN_SECTION_MAPPING
