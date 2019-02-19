#define GT_STRUCTURED_GRIDS
#include <pybind11/pybind11.h>
#include <gridtools/stencil-composition/stencil-composition.hpp>

namespace py = pybind11;
using backend_t =
    gridtools::backend<gridtools::target::x86, gridtools::grid_type::structured,
                       gridtools::strategy::block>;
constexpr int ndims = 3;
using float_type = double;
// we can use halo = 0. This is redundant here because we use an external
// storage.
using storage_info_t =
    gridtools::storage_traits<backend_t::backend_id_t>::storage_info_t<
        0, ndims, gridtools::halo<0, 0, 0>>;
using data_store_t = gridtools::storage_traits<
    backend_t::backend_id_t>::data_store_t<float_type, storage_info_t>;
namespace copy_stencil {

struct copy_functor {
    using in = gridtools::in_accessor<0>;
    using out = gridtools::inout_accessor<1>;
    using param_list = gridtools::make_param_list<in, out>;

    template <typename Evaluation>
    GT_FUNCTION static void apply(Evaluation &eval) {
        eval(out()) = eval(in());
    }
};
data_store_t make_data_store(py::buffer b) {
    auto buffer_info = b.request();
    if (buffer_info.format != py::format_descriptor<float_type>::format())
        throw std::runtime_error(
            "wrong real type: " + buffer_info.format +
            " != " + py::format_descriptor<float_type>::format());
    if (buffer_info.ndim != 3) throw std::runtime_error("wrong dimension");
    gridtools::array<unsigned, ndims> dims{};
    gridtools::array<unsigned, ndims> strides{};
    for (int i = 0; i < ndims; ++i) {
        dims[i] = buffer_info.shape[i];
        strides[i] = buffer_info.strides[i] / sizeof(float_type);
    }
    storage_info_t storage_info{dims, strides};
    return data_store_t{storage_info,
                        static_cast<float_type *>(buffer_info.ptr),
                        gridtools::ownership::external_cpu};
}
void do_copy(py::buffer b_in, py::buffer b_out) {
    auto in = make_data_store(b_in);
    auto out = make_data_store(b_out);
    // somehow we need to infer the size of the halo region, then we do not pass
    // in.total_length<0>, but
    //
    // ```
    // halo_descriptor{hs, hs, hs, in.total_length<0>() - hs - 1,
    //                 in.total_length<0>()}
    // ```
    auto v = make_host_view(in);
    auto grid = gridtools::make_grid(in.total_length<0>(), in.total_length<1>(),
                                     in.total_length<2>());
    typedef gridtools::arg<0, data_store_t> p_in;
    typedef gridtools::arg<1, data_store_t> p_out;
    // This should not be in the do_copy. We should create the object separately
    // such that we can reuse it. If we want to apply a computation to a
    // subdomain of a grid, we need to make a new computation, but this also
    // makes sense, because temporaries and any fields bound in the
    // make_computation will have different size
    auto copy = gridtools::make_computation<backend_t>(
        grid, gridtools::make_multistage(
                  gridtools::enumtype::execute<gridtools::enumtype::forward>(),
                  gridtools::make_stage<copy_functor>(p_in(), p_out())));
    copy.run(p_in() = in, p_out() = out);
    copy.sync_bound_data_stores();
}
}  // namespace copy_stencil
PYBIND11_MODULE(copy_simple, m) { m.def("copy", &copy_stencil::do_copy); }
