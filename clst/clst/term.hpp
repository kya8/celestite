#ifndef CLST_TERM_HPP
#define CLST_TERM_HPP

#include <utility>

namespace clst::term {

struct TermSize {
    int row;
    int column;
};

TermSize get_term_size();

} // namespace clst::term

#endif // CLST_TERM_HPP
