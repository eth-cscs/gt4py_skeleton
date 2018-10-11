#define STRUCTURED_GRIDS

#include <gridtools/boundary-conditions/boundary.hpp>
#include <gridtools/boundary-conditions/copy.hpp>
#include <gridtools/common/defs.hpp>
#include <gridtools/stencil-composition/stencil-composition.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <array>
#include <cassert>
#include <stdexcept>

namespace gt = ::gridtools;
namespace py = ::pybind11;

namespace gtcomputation {

namespace {

static constexpr gt::uint_t halo_size = 1;
using float_t = double;

using backend_t = gt::backend<gt::platform::x86, gt::grid_type::structured,
                              gt::strategy::block>;

gt::halo_descriptor make_halo_descriptor(gt::uint_t outer_size,
                                         gt::uint_t halo_size) {
    return {halo_size, halo_size, halo_size, outer_size - halo_size - 1,
            outer_size};
}

using storage_info_t =
    gt::storage_traits<backend_t::backend_id_t>::custom_layout_storage_info_t<
        0, typename gt::get_layout<3, true>::type,
        gt::halo<halo_size, halo_size, 0>>;
using data_store_t =
    gt::storage_traits<backend_t::backend_id_t>::data_store_t<float_t,
                                                              storage_info_t>;
using p_f_out = gt::arg<0, data_store_t>;
using p_f_in = gt::arg<1, data_store_t>;

data_store_t make_data_store(py::buffer& b,
                             const std::array<gt::uint_t, 3>& outer_size,
                             const std::array<gt::uint_t, 3>& origin) {
    auto buffer_info = b.request();

    if (buffer_info.format != py::format_descriptor<float_t>::format()) {
        throw std::runtime_error(
            "Wrong real type: " + buffer_info.format +
            " != " + py::format_descriptor<float_t>::format());
    }

    if (buffer_info.ndim != 3) {
        throw std::runtime_error("Wrong number of dimensions [" +
                                 std::to_string(buffer_info.ndim) +
                                 " != " + std::to_string(3) + "]");
    }

    for (int i = 0; i < 3; ++i) {
        if (i < 2 && outer_size[i] <= 2 * halo_size)
            throw std::runtime_error(
                "Invalid domain size. Compute domain must be non-empty "
                "(domain size > 2 * halo-size)");

        if (origin[i] + outer_size[i] > buffer_info.shape[i])
            throw std::runtime_error(
                "Given shape and origin exceed buffer dimension");
    }

    // ptr, dims and strides are "outer domain" (i.e., compute domain + halo
    // region). The halo region is only defined through `make_grid` (and
    // currently, in the storage info)
    float_t* ptr = static_cast<float_t*>(buffer_info.ptr);
    gt::array<gt::uint_t, 3> dims{};
    gt::array<gt::uint_t, 3> strides{};
    for (int i = 0; i < 3; ++i) {
        strides[i] = buffer_info.strides[i] / sizeof(float_t);
        ptr += strides[i] * origin[i];
        dims[i] = outer_size[i];
    }
    return data_store_t{storage_info_t{dims, strides}, ptr,
                        gt::ownership::ExternalCPU};
}

}  // namespace

class GTCopyBoundary {
   public:
    // halo descriptors: total_length is not required again, so we can pass the
    // subdomain
    GTCopyBoundary(std::array<gt::uint_t, 3> size, gt::uint_t halo)
        : boundary_({make_halo_descriptor(size[0], halo_size),
                     make_halo_descriptor(size[1], halo_size),
                     make_halo_descriptor(size[2], 0)},
                    gt::copy_boundary{}),
          size_(size) {}

    void run(py::buffer b_f_out, const std::array<gt::uint_t, 3>& f_out_origin,
             py::buffer b_f_in, const std::array<gt::uint_t, 3>& f_in_origin) {
        auto ds_f_out = make_data_store(b_f_out, size_, f_out_origin);
        auto ds_f_in = make_data_store(b_f_in, size_, f_in_origin);

        boundary_.apply(ds_f_out, ds_f_in);
    }

   private:
    const std::array<gt::uint_t, 3> size_;
    gt::boundary<gt::copy_boundary, gt::platform::x86> boundary_;
};

}  // namespace gtcomputation

static constexpr std::array<gt::uint_t, 3> zero_origin{0, 0, 0};

PYBIND11_MODULE(gtboundary, m) {
    py::class_<gtcomputation::GTCopyBoundary>(m, "GTCopyBoundary")
        .def(py::init<std::array<gt::uint_t, 3>, gt::uint_t>(),
             py::arg("shape"), py::arg("halo"))
        .def("run", &gtcomputation::GTCopyBoundary::run, py::arg("f_out"),
             py::arg("f_out_origin") = zero_origin, py::arg("f_in"),
             py::arg("f_in_origin") = zero_origin);
}
