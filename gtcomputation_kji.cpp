#include <gridtools/common/defs.hpp>
#include <gridtools/stencil_composition/stencil_composition.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <array>
#include <cassert>
#include <stdexcept>

namespace gt = ::gridtools;
namespace py = ::pybind11;

namespace gtcomputation_kji {

namespace {

struct diag_diff_1_functor {
    using out = gt::inout_accessor<0>;
    using in = gt::in_accessor<1, gt::extent<-1, 0, -1, 0>>;
    using param_list = gt::make_param_list<out, in>;

    template <typename Evaluation>
    GT_FUNCTION static void apply(Evaluation& eval) {
        eval(out()) = eval(in(-1, -1, 0));
    }
};

static constexpr gt::uint_t halo_size = 1;
using float_t = double;

using backend_t = gt::backend::x86;

gt::halo_descriptor make_halo_descriptor(gt::uint_t outer_size,
                                         gt::uint_t halo_size) {
    return {halo_size, halo_size, halo_size, outer_size - halo_size - 1,
            outer_size};
}

// TODO: We need to find a way how to define the vertical, which intervals
// we want, who defines the sizes of them, and whether we want to have
// additional layers on top / bottom (LevelOffsetLimit of axis)
auto make_grid(const std::array<gt::uint_t, 3>& size) {
    return gt::make_grid(make_halo_descriptor(size[0], halo_size),
                         make_halo_descriptor(size[1], halo_size), size[2]);
}

using storage_info_t =
    gt::storage_traits<backend_t>::custom_layout_storage_info_t<
        0, typename gt::get_layout<3, false>::type,
        gt::halo<halo_size, halo_size, 0>>;
using data_store_t =
    gt::storage_traits<backend_t>::data_store_t<float_t, storage_info_t>;
using p_f_out = gt::arg<0, data_store_t>;
using p_f_in = gt::arg<1, data_store_t>;

template <typename Grid>
auto make_computation_helper(const Grid& grid) {
    return gt::make_computation<backend_t>(
        grid, gt::make_multistage(
                  gt::execute::forward(),
                  gt::make_stage<diag_diff_1_functor>(p_f_out(), p_f_in())));
}

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
                        gt::ownership::external_cpu};
}

}  // namespace

// Note: The GTComputation object is expensive to be created and should not be
// recreated over and over again. Usually we will only call a computation with
// one size. If this is not the case, we should cache the most recent
// GTComputation objects on python side (@lru_cache probably). I don't see the
// point in supporting this here, because we impose an additional requirement
// here which is not needed in general.
class GTComputation {
   public:
    GTComputation(std::array<gt::uint_t, 3> size, gt::uint_t halo)
        : size_(size), computation_(make_computation_helper(make_grid(size))) {
        // TODO the halo_size will not be compile-time anymore at a certain
        // point. Currently we just want the user to pass it to verify if he is
        // really doing what he intends to do. In fact, I think we don't care
        // that much about this, because we can calculate the halo size when we
        // create the computation. There is nothing we can gain when passing a
        // different halo size every time. This number will be internal anyway.
        //
        // Currently, we require that halo_sizes the same on both sides, i.e.,
        // halo_left == halo_right and halo_front == halo_back. This restriction
        // will disappear when the halo region is only controlled by the halo
        // descriptors. But again, this is only an internal requirement (with
        // very small limitations from user perspective).
        assert(halo == halo_size);
    }

    void run(py::buffer b_f_out, const std::array<gt::uint_t, 3>& f_out_origin,
             py::buffer b_f_in, const std::array<gt::uint_t, 3>& f_in_origin) {
        // Initialize data stores from input buffers
        auto ds_f_out = make_data_store(b_f_out, size_, f_out_origin);
        auto ds_f_in = make_data_store(b_f_in, size_, f_in_origin);
        // Run computation and wait for the synchronization of the output stores
        computation_.run(p_f_out() = ds_f_out, p_f_in() = ds_f_in);
    }

   private:
    gt::computation<p_f_out, p_f_in> computation_;
    const std::array<gt::uint_t, 3> size_;
};

}  // namespace gtcomputation_kji

static constexpr std::array<gt::uint_t, 3> zero_origin{0, 0, 0};
PYBIND11_MODULE(gtcomputation_kji, m) {
    py::class_<gtcomputation_kji::GTComputation>(m, "GTComputation")
        .def(py::init<std::array<gt::uint_t, 3>, gt::uint_t>(),
             py::arg("shape"), py::arg("halo"))
        .def("run", &gtcomputation_kji::GTComputation::run, py::arg("f_out"),
             py::arg("f_out_origin") = zero_origin, py::arg("f_in"),
             py::arg("f_in_origin") = zero_origin);
}
