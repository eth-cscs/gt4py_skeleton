#include "gridtools/clang_dsl.hpp"

using namespace gridtools::clang;

globals { double global_param = 1.0; };

stencil copy_stencil {
    storage out;
    storage in;

    Do {
        vertical_region(k_start, k_end) { out = global_param * in; }
    }
};

