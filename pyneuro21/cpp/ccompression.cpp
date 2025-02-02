#include <filesystem>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "../../neuro21_cpp/src/data_handlers.cpp"


namespace py = pybind11;
namespace fs = std::filesystem;

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

void write_file_header(
    const std::string& file_path,
    const std::string& file_type,
    const std::string& version,
    const std::string& patient_id,
    const std::string& session_id,
    const std::string& channel_id,
    const std::string& compression,
    const double_t& sampling_rate
    ) {
        fs::path fpath = fs::path(file_path);
    neuro21::FileHeader header(
        file_type,
        version,
        patient_id,
        session_id,
        channel_id,
        compression,
        sampling_rate
    );
    neuro21::GenericFile fileHeaderWrite(fpath, header);
    }

py::tuple read_file_header(const std::string& file_path) {
    fs::path fpath = fs::path(file_path);
    neuro21::GenericFile fileRead(fpath);
    neuro21::FileHeader header = fileRead.getHeader();

    return py::make_tuple(
        header.file_type,
        header.get_file_header_version(),
        header.patient_id,
        header.session_id,
        header.channel_id,
        header.compression,
        header.sampling_rate
    );
}

PYBIND11_MODULE(ccompression, m) {
    m.doc() = "Gorilla compression module implemented in C++ with pybind11";
    m.def("gorilla_ccompress", &compress_wrapper, py::arg("data_points"), "Compress a NumPy array of doubles using Gorilla compression");
    m.def("gorilla_cdecompress", &decompress_wrapper, py::arg("compressed_data"), "Decompress data compressed with Gorilla compression back to a NumPy array");
    m.def("write_file_header_cpp", &write_file_header,
      py::arg("file_path"), py::arg("file_type"), py::arg("version"),
      py::arg("patient_id"), py::arg("session_id"), py::arg("channel_id"),
      py::arg("compression"), py::arg("sampling_rate"),
      "Write a file header to a file");
    m.def("read_file_header_cpp", &read_file_header, py::arg("file_path"), "Read a file header from a file");
}
