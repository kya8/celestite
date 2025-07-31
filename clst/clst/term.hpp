#ifndef CLST_TERM_HPP
#define CLST_TERM_HPP

#include <cstdio>

namespace clst::term {

struct TermSize {
    int row;
    int column;
};

TermSize get_term_size();

bool is_tty(std::FILE* file) noexcept;

// Detect if terminal supports color, based on a few heuristics.
// Does not differentiate between levels of color support,
// e.g. ANSI, 256 color, 24bit true color.
bool is_colorterm() noexcept;
bool is_colorterm(std::FILE* file) noexcept;

} // namespace clst::term

#endif // CLST_TERM_HPP
