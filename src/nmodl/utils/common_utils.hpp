#ifndef _COMMON_H_
#define _COMMON_H_

template <typename Iter, typename Cont>
bool is_last(Iter iter, const Cont* cont) {
    return ((iter != cont->end()) && (next(iter) == cont->end()));
}

#endif
