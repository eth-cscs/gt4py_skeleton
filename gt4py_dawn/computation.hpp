

#include <gridtools/common/defs.hpp>
#include <gridtools/common/gt_math.hpp>
#include <gridtools/stencil_composition/stencil_composition.hpp>

#include <boost/cstdfloat.hpp>

#include <array>
#include <cstdint>
#include <vector>

using boost::float32_t;
using boost::float64_t;

using py_size_t = std::intptr_t;

struct BufferInfo {
  py_size_t ndim;
  std::vector<py_size_t> shape;
  std::vector<py_size_t> strides;
  void *ptr;
};

namespace gt = ::gridtools;

namespace new_computation {

// Backend
using backend_t = gt::backend::mc;

// These halo sizes are used to determine the sizes of the temporaries
static constexpr gt::uint_t halo_size_i = 3;
static constexpr gt::uint_t halo_size_j = 3;
static constexpr gt::uint_t halo_size_k = 0;

void run(const std::array<gt::uint_t, 3> &domain, const BufferInfo &bi_f_out,
         const std::array<gt::uint_t, 3> &f_out_origin,
         const BufferInfo &bi_f_in,
         const std::array<gt::uint_t, 3> &f_in_origin);

} // namespace new_computation
