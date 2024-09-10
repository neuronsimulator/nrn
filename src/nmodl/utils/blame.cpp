#include "blame.hpp"

#include "string_utils.hpp"
#include "utils/fmt.h"
#include <filesystem>
#include <stdexcept>
#include <string>

#if NMODL_ENABLE_BACKWARD
#include <backward.hpp>
#endif

namespace nmodl {
namespace utils {

class NoBlame: public Blame {
  public:
    using Blame::Blame;

  protected:
    std::string format() const override {
        return "";
    }
};

#if NMODL_ENABLE_BACKWARD
size_t first_relevant_trace(backward::TraceResolver& tr, const backward::StackTrace& st) {
    std::vector<std::string> stop_at{"printer/code_printer.cpp", "printer/code_printer.hpp"};
    int start_from = int(st.size()) - 2;
    for (int i = start_from; i >= 0; --i) {
        backward::ResolvedTrace trace = tr.resolve(st[i]);
        const std::string& filename = trace.source.filename;

        for (const auto& f: stop_at) {
            if (stringutils::ends_with(filename, f)) {
                return i + 1;
            }
        }
    }

    throw std::runtime_error("Failed to determine relevant trace.");
}

class BackwardTracePrinter {
  public:
    std::string format(const backward::ResolvedTrace& trace, const std::string& trace_label) const {
        const std::string& filename = trace.source.filename;
        size_t linenumber = trace.source.line;

        auto pad = std::string(trace_label.size(), ' ');

        std::stringstream sout;

        sout << fmt::format("{}    Source: \"{}\", line {}, in {}\n",
                            trace_label,
                            filename,
                            linenumber,
                            trace.source.function);

        if (std::filesystem::exists(trace.source.filename) && trace.source.line != 0) {
            auto snippet = snippets.get_snippet(filename, linenumber, 3);
            for (auto line: snippet) {
                sout << fmt::format("{}    {}{:>5d}:  {}\n",
                                    pad,
                                    linenumber == line.first ? ">" : " ",
                                    line.first,
                                    line.second);
            }
        }

        return sout.str();
    }

    mutable backward::SnippetFactory snippets;
};

class ShortBlame: public Blame {
  public:
    using Blame::Blame;

  protected:
    std::string format() const override {
        backward::StackTrace st;
        st.load_here(32);

        backward::TraceResolver tr;

        size_t trace_id = first_relevant_trace(tr, st);
        backward::ResolvedTrace trace = tr.resolve(st[trace_id]);

        return trace_printer.format(trace, "");
    }

    BackwardTracePrinter trace_printer;
};


class DetailedBlame: public Blame {
  public:
    using Blame::Blame;

  protected:
    std::string format() const override {
        backward::StackTrace st;
        st.load_here(32);

        backward::TraceResolver tr;

        size_t first_trace_id = first_relevant_trace(tr, st);

        std::string full_trace = "\n\n --- Backtrace ----------------\n";
        for (int i = st.size() - 1; i >= first_trace_id; --i) {
            backward::ResolvedTrace trace = tr.resolve(st[i]);
            full_trace += trace_printer.format(trace, std::to_string(i) + ":");
        }
        return full_trace;
    }

    BackwardTracePrinter trace_printer;
};


#else
using ShortBlame = NoBlame;
using DetailedBlame = NoBlame;
#endif

std::unique_ptr<Blame> make_blame(size_t blame_line, BlameLevel blame_level) {
    if (blame_level == BlameLevel::Short) {
        return std::make_unique<ShortBlame>(blame_line);
    } else if (blame_level == BlameLevel::Detailed) {
        return std::make_unique<DetailedBlame>(blame_line);
    } else {
        throw std::runtime_error("Unknown blame_level.");
    }
}

}  // namespace utils
}  // namespace nmodl
