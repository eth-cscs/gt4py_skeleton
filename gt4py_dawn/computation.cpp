#include "../generated/copy_stencil.hpp"

#include "computation.hpp"

#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace new_computation {

namespace {

// Grids and halos
constexpr int halo_size = GRIDTOOLS_CLANG_HALO_EXTEND;

gridtools::clang::domain make_domain(const std::array<gt::uint_t, 3> &size) {
    gridtools::clang::domain d{size};
    d.set_halos(0, 0, 0, 0, 0, 0);
    return d;
}
using storage_info_t = meta_data_ijk_t;
using data_store_t = storage_ijk_t;
static_assert(
    std::is_same<data_store_t, gridtools::clang::storage_ijk_t>::value,
    "Storage types do not match");

data_store_t
make_data_store(const BufferInfo &bi,
                const std::array<gt::uint_t, 3> &compute_domain_shape,
                const std::array<gt::uint_t, 3> &origin) {
  if (bi.ndim != 3) {
    throw std::runtime_error("Wrong number of dimensions [" +
                             std::to_string(bi.ndim) +
                             " != " + std::to_string(3) + "]");
  }

  for (int i = 0; i < 2 /*3*/; ++i) {
    if (origin[i] < halo_size_i)
      throw std::runtime_error("Origin too small");
    if (origin[i] + halo_size_i + compute_domain_shape[i] < bi.shape[i])
      throw std::runtime_error(
          "Given shape and origin exceed buffer dimension");
  }

  // ptr, dims and strides are "outer domain" (i.e., compute domain + halo
  // region). The halo region is only defined through `make_grid` (and
  // currently, in the storage info)
  gt::array<gt::uint_t, 3> dims{};
  gt::array<gt::uint_t, 3> strides{};
  double *ptr = static_cast<double *>(bi.ptr);
  for (int i = 0; i < 3; ++i) {
    strides[i] = bi.strides[i] / sizeof(double);
    ptr += strides[i] * origin[i];
    dims[i] = compute_domain_shape[i] + 2 * origin[i];
  }
  return data_store_t{storage_info_t{dims, strides}, ptr,
                             gt::ownership::external_cpu};
}

} // namespace

// Run actual computation
void run(const std::array<gt::uint_t, 3> &domain, const BufferInfo &bi_f_out,
         const std::array<gt::uint_t, 3> &f_out_origin,
         const BufferInfo &bi_f_in,
         const std::array<gt::uint_t, 3> &f_in_origin) {
  // Initialize data stores from input buffers
  auto ds_f_out = make_data_store(bi_f_out, domain, f_out_origin);
  auto ds_f_in = make_data_store(bi_f_in, domain, f_in_origin);

  // Update global parameters

  // Run computation and wait for the synchronization of the output stores
  dawn_generated::cxxnaive::copystencil{make_domain(domain)}
    .run(ds_f_in, ds_f_out);

  // computation_.sync_bound_data_stores();
}

} // namespace new_computation
