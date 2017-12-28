#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

/** Check if the iterator is pointing to last element in the container */
template <typename Iter, typename Cont>
bool is_last(Iter iter, const Cont* cont) {
    return ((iter != cont->end()) && (next(iter) == cont->end()));
}

/** Given full file path, returns only name of the file */
template <class T>
T base_name(T const& path, T const& delims = "/\\") {
    return path.substr(path.find_last_of(delims) + 1);
}

/** Given the file name, returns name of the file without extension */
template <class T>
T remove_extension(T const& filename) {
    typename T::size_type const p(filename.find_last_of('.'));
    return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
}

#endif
