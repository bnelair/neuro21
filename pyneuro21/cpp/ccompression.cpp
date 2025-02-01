#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
//#include "../../neuro21_cpp/src/gorilla_compression.cpp"
//#include "../../neuro21_cpp/src/utils.cpp"
#include "../../neuro21_cpp/src/data_handlers.cpp"


namespace py = pybind11;

py::bytes compress_wrapper(py::array_t<double_t> data_points) {
    auto buf = data_points.request();
    if (buf.ndim != 1) {
        throw std::runtime_error("Input should be a 1-D array");
    }
    std::vector<double_t> input_data(static_cast<double_t*>(buf.ptr), static_cast<double_t*>(buf.ptr) + buf.size);
    neuro21::GorillaCompressor compressor;
    auto compressed_data = compressor.compress(input_data);
    return py::bytes(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
}

py::array_t<double_t> decompress_wrapper(const std::string& compressed_data) {
    std::vector<uint8_t> bytes(compressed_data.begin(), compressed_data.end());
    neuro21::GorillaDecompressor decompressor;
    auto decompressed_data = decompressor.decompress(bytes);
    py::array_t<double_t> result(decompressed_data.size());
    auto buf = result.request();
    std::memcpy(buf.ptr, decompressed_data.data(), decompressed_data.size() * sizeof(double_t));
    return result;
}

PYBIND11_MODULE(ccompression, m) {
    m.doc() = "Gorilla compression module implemented in C++ with pybind11";
    m.def("gorilla_ccompress", &compress_wrapper, py::arg("data_points"), "Compress a NumPy array of doubles using Gorilla compression");
    m.def("gorilla_cdecompress", &decompress_wrapper, py::arg("compressed_data"), "Decompress data compressed with Gorilla compression back to a NumPy array");
}
