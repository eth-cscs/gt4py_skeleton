#define STRUCTURED_GRIDS

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
struct data_t {
    double x;
    double y;
};

struct revert {
    using out = gt::accessor<0, gt::enumtype::inout>;
    using in = gt::accessor<1, gt::enumtype::in, gt::extent<-1, 0, -1, 0>>;
    using gp = gt::global_accessor<2>;
    using arg_list = boost::mpl::vector<out, in, gp>;

    template <typename Evaluation>
    GT_FUNCTION static void Do(Evaluation& eval) {
        eval(out()).y = eval(in()).x + eval(gp()).y;
        eval(out()).x = eval(in()).y + eval(gp()).x;
    }
};

using backend_t = gt::backend<gt::target::x86, gt::grid_type::structured,
                              gt::strategy::block>;

template <typename T>
using global_parameter_t =
    decltype(backend_t::make_global_parameter(std::declval<T>()));

gt::halo_descriptor make_halo_descriptor(gt::uint_t outer_size,
                                         gt::uint_t halo_size) {
    return {halo_size, halo_size, halo_size, outer_size - halo_size - 1,
            outer_size};
}

auto make_grid(const std::array<gt::uint_t, 3>& size)
    GT_AUTO_RETURN(gt::make_grid(make_halo_descriptor(size[0], halo_size),
                                 make_halo_descriptor(size[1], halo_size),
                                 size[2]));

using storage_info_t =
    gt::storage_traits<backend_t::backend_id_t>::custom_layout_storage_info_t<
        0, typename gt::get_layout<3, true>::type,
        gt::halo<halo_size, halo_size, halo_size>>;
using data_store_t =
    gt::storage_traits<backend_t::backend_id_t>::data_store_t<data_t,
                                                              storage_info_t>;
using p_f_out = gt::arg<0, data_store_t>;
using p_f_in = gt::arg<1, data_store_t>;
using p_f_gp = gt::arg<2, global_parameter_t<data_t>>;

template <typename Grid>
auto make_computation(const Grid& grid,
                      global_parameter_t<data_t> global_parameter)
    GT_AUTO_RETURN(gt::make_computation<backend_t>(
        grid, p_f_gp() = global_parameter,
        gt::make_multistage(gt::enumtype::execute<gt::enumtype::forward>(),
                            gt::make_stage<revert>(p_f_out(), p_f_in(),
                                                   p_f_gp()))));

data_store_t make_data_store(py::buffer& b,
                             const std::array<gt::uint_t, 3>& outer_size,
                             const std::array<gt::uint_t, 3>& origin) {
    auto buffer_info = b.request();

    if (buffer_info.format != "T{d:x:d:y:}")
        throw std::runtime_error("Wrong real type: " + buffer_info.format +
                                 " != " + "T{d:x:d:y:}");

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
    data_t* ptr = static_cast<data_t*>(buffer_info.ptr);
    gt::array<gt::uint_t, 3> dims{};
    gt::array<gt::uint_t, 3> strides{};
    for (int i = 0; i < 3; ++i) {
        strides[i] = buffer_info.strides[i] / sizeof(data_t);
        ptr += strides[i] * origin[i];
        dims[i] = outer_size[i];
    }
    return data_store_t{storage_info_t{dims, strides}, ptr,
                        gt::ownership::ExternalCPU};
}  // namespace

}  // namespace

// Note: The GTComputation object is expensive to be created and should not be
// recreated over and over again. Usually we will only call a computation with
// one size. If this is not the case, we should cache the most recent
// GTComputation objects on python side (@lru_cache probably). I don't see the
// point in supporting this here, because we impose an additional requirement
// here which is not needed in general.
class GTComputationStruct {
   public:
    GTComputationStruct(std::array<gt::uint_t, 3> size, gt::uint_t halo)
        : size_(size),
          global_parameter_(backend_t::make_global_parameter(data_t{})),
          computation_(make_computation(make_grid(size), global_parameter_)) {
        assert(halo == halo_size);
    }

    void run(py::buffer b_f_out, const std::array<gt::uint_t, 3>& f_out_origin,
             py::buffer b_f_in, const std::array<gt::uint_t, 3>& f_in_origin,
             double x, double y) {
        // Initialize data stores from input buffers
        auto ds_f_out = make_data_store(b_f_out, size_, f_out_origin);
        auto ds_f_in = make_data_store(b_f_in, size_, f_in_origin);
        backend_t::update_global_parameter(global_parameter_, data_t{x, y});
        // Run computation and wait for the synchronization of the output stores
        computation_.run(p_f_out() = ds_f_out, p_f_in() = ds_f_in);
        computation_.sync_bound_data_stores();
    }

   private:
    const std::array<gt::uint_t, 3> size_;
    const global_parameter_t<data_t> global_parameter_;
    gt::computation<void, p_f_out, p_f_in> computation_;
};

}  // namespace gtcomputation

static constexpr std::array<gt::uint_t, 3> zero_origin{0, 0, 0};
PYBIND11_MODULE(gtcomputation_struct, m) {
    py::class_<gtcomputation::GTComputationStruct>(m, "GTComputationStruct")
        .def(py::init<std::array<gt::uint_t, 3>, gt::uint_t>(),
             py::arg("shape"), py::arg("halo"))
        .def("run", &gtcomputation::GTComputationStruct::run, py::arg("f_out"),
             py::arg("f_out_origin") = zero_origin, py::arg("f_in"),
             py::arg("f_in_origin") = zero_origin, py::arg("x"), py::arg("y"));
}
