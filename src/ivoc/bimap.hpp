//
//  bimap.hpp
//  bimap
//
//  Created by Ewart Timothee on 16/06/16.
//

#ifndef bimap_h
#define bimap_h

#include <map>

namespace nrn {
namespace tool {

template <class T, class O>
struct bimap {
    typedef std::multimap<T, O> pd2ob_map;
    typedef std::multimap<O, T> ob2pd_map;

    void insert(T const&, O const&);
    bool find(T const& p, size_t n, T&, O&);
    bool find(T const& p, T&, O&);
    void obremove(O const&);
    void remove(T const&, O const&);

    pd2ob_map pd2ob;
    ob2pd_map ob2pd;
};

// free function for remove, generic also no need to duplicate
template <class T>
void remove_from(typename T::key_type const& a, typename T::mapped_type const& b, T& m) {
    std::pair<typename T::iterator, typename T::const_iterator> itp = m.equal_range(a);
    for (typename T::iterator it = itp.first; it != itp.second;) {
        typename T::iterator it2(it);
        ++it;
        if (it2->second == b) {
            m.erase(it2);
        }
    }
}

template <class T, class O>
void bimap<T, O>::insert(T const& pd, O const& ob) {
    pd2ob.insert(std::pair<T, O>(pd, ob));
    ob2pd.insert(std::pair<O, T>(ob, pd));
}

template <class T, class O>
void bimap<T, O>::obremove(O const& ob) {
    std::pair<typename ob2pd_map::iterator, typename ob2pd_map::iterator> itp = ob2pd.equal_range(
        ob);
    for (typename ob2pd_map::iterator it = itp.first; it != itp.second; ++it) {
        T const& pd = it->second;
        remove_from(pd, ob, pd2ob);
    }
    ob2pd.erase(itp.first, itp.second);
}

template <class T, class O>
void bimap<T, O>::remove(T const& pd, O const& ob) {
    remove_from(pd, ob, pd2ob);
    remove_from(ob, pd, ob2pd);
}

template <class T, class O>
bool bimap<T, O>::find(T const& p, size_t n, T& pret, O& obret) {
    bool result = false;
    typename pd2ob_map::iterator it = pd2ob.upper_bound(p + n);
    if (it != pd2ob.begin()) {
        --it;
        if ((it->first >= p) && (it->first < p + n)) {
            result = true;
            pret = it->first;
            obret = it->second;
        }
    }
    return result;
}

template <class T, class O>
bool bimap<T, O>::find(T const& p, T& pret, O& obret) {
    bool result = false;
    typename pd2ob_map::iterator it = pd2ob.find(p);
    if (it != pd2ob.end()) {
        result = true;
        pret = it->first;
        obret = it->second;
    }
    return result;
}

}  // namespace tool
}  // end namespace nrn
#endif /* bimap_h */
