#ifndef _NMODL_VISITOR_HELPER_HPP_
#define _NMODL_VISITOR_HELPER_HPP_

#include "visitors/nmodl_visitor.hpp"


/** Helper function to visit vector elements
 *
 * @tparam T
 * @param separator separator to print for individual vector element
 * @param program  true if provided elements belong to program node
 * @param statement true if elements in vector of statement type
 */

template <typename T>
void NmodlPrintVisitor::visit_element(const std::vector<T>& elements,
                                      std::string separator,
                                      bool program,
                                      bool statement) {
    for (auto iter = elements.begin(); iter != elements.end(); iter++) {
        /// statements need indentation at the start
        if (statement) {
            printer->add_indent();
        }

        (*iter)->accept(this);

        /// print separator (e.g. comma, space)
        if (!separator.empty() && !is_last(iter, elements)) {
            printer->add_element(separator);
        }

        /// newline at the end of statement
        if (statement) {
            printer->add_newline();
        }

        /// if there are multiple inline comments then we want them to be
        /// contiguous and only last comment should have extra line.
        bool extra_newline = false;
        if (!is_last(iter, elements)) {
            extra_newline = true;
            if ((*iter)->is_comment() && (*(iter + 1))->is_comment()) {
                extra_newline = false;
            }
        }

        /// program blocks need two newlines except last one
        if (program) {
            printer->add_newline();
            if (extra_newline) {
                printer->add_newline();
            }
        }
    }
}

#endif
