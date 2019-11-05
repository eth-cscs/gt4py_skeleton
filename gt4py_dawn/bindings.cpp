

#include "computation.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <chrono>
#include <vector>

namespace py = ::pybind11;

namespace {

BufferInfo make_buffer_info(py::object &b) {
    auto buffer_info = static_cast<py::buffer &>(b).request();

    py_size_t ndim = static_cast<py_size_t>(buffer_info.ndim);
    std::vector<py_size_t> &shape = buffer_info.shape;
    std::vector<py_size_t> &strides = buffer_info.strides;
    void *ptr = static_cast<void *>(buffer_info.ptr);

    if (ndim != 3) {
        throw std::runtime_error("Wrong number of dimensions [" +
                                 std::to_string(ndim) +
                                 " != " + std::to_string(3) + "]");
    }

    return BufferInfo{ndim, shape, strides, ptr};
}

void run_computation(const std::array<gt::uint_t, 3> &domain, py::object f_out,
                     const std::array<gt::uint_t, 3> &f_out_origin,
                     py::object f_in,
                     const std::array<gt::uint_t, 3> &f_in_origin,
                     py::object &exec_info) {
    if (!exec_info.is(py::none())) {
        auto exec_info_dict = exec_info.cast<py::dict>();
        exec_info_dict["start_run_cpp_time"] =
            static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now()
                        .time_since_epoch())
                    .count()) /
            1e9;
    }
    auto bi_f_out = make_buffer_info(f_out);
    auto bi_f_in = make_buffer_info(f_in);

    new_computation::run(domain, bi_f_out, f_out_origin, bi_f_in, f_in_origin);

    if (!exec_info.is(py::none())) {
        auto exec_info_dict = exec_info.cast<py::dict>();
        exec_info_dict["end_run_cpp_time"] = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count() /
            1e9);
    }
}

}  // namespace

PYBIND11_MODULE(gt4py_dawn_computation, m) {
    m.def("run_computation", &run_computation, "Runs the given computation",
          py::arg("domain"), py::arg("f_out"), py::arg("f_out_origin"),
          py::arg("f_in"), py::arg("f_in_origin"), py::arg("exec_info"));
}
