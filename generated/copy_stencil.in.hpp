#include "gridtools/clang_dsl.hpp"

using namespace gridtools::clang;

stencil copystencil {
    storage out;
    storage in;

    Do {
        vertical_region(k_start, k_end) { out = in; }
    }
};
