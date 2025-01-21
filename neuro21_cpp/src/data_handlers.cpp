//
// Created by Mivalt, Filip, M.S. on 1/15/25.
//
#include <vector>
#include <iostream>
#include <cstdint>
#include <fstream>

#include <iostream>
#include <fstream>
#include <string>

#include "utils.cpp"
#include "gorilla_compression.cpp"


namespace neuro21 {

    class MetadataBlock{
    private:
        int64_t start_uutc;
        int64_t end_uutc;
        uint64_t block_start;
        uint64_t block_length;
        int32_t block_crc;

    public:
        MetadataBlock(int64_t start_uutc, int64_t end_uutc, uint64_t block_start, uint64_t block_length, uint64_t block_crc) {
            this->start_uutc = start_uutc;
            this->end_uutc = end_uutc;
            this->block_start = block_start;
            this->block_length = block_length;
            this->block_crc = block_crc;
        }

        MetadataBlock(const std::vector<uint8_t>& data) {
            uint64_t offset = 0;
            start_uutc = neuro21::SerializeInt64::deserialize(data, offset);
            end_uutc = neuro21::SerializeInt64::deserialize(data, offset);
            block_start = neuro21::SerializeUInt64::deserialize(data, offset);
            block_length = neuro21::SerializeUInt64::deserialize(data, offset);
            block_crc = neuro21::SerializeInt32::deserialize(data, offset);
        }

        [[nodiscard]] std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> data;
            neuro21::SerializeInt64::serialize(start_uutc, data);
            neuro21::SerializeInt64::serialize(end_uutc, data);
            neuro21::SerializeInt64::serialize(block_start, data);
            neuro21::SerializeInt64::serialize(block_length, data);
            neuro21::SerializeInt64::serialize(block_crc, data);
            return data;
        }

        [[nodiscard]] int64_t get_start_uutc() const {
            return start_uutc;
        }

        [[nodiscard]] int64_t get_end_uutc() const {
            return end_uutc;
        }

        [[nodiscard]] uint64_t get_block_start() const {
            return block_start;
        }

        [[nodiscard]] uint64_t get_block_length() const {
            return block_length;
        }

        [[nodiscard]] uint64_t get_block_crc() const {
            return block_crc;
        }

        [[nodiscard]] std::string to_string() const {
            return "Start UUTC: " + std::to_string(start_uutc) + "\n"
                   + "End UUTC: " + std::to_string(end_uutc) + "\n"
                   + "Block start: " + std::to_string(block_start) + "\n"
                   + "Block length: " + std::to_string(block_length) + "\n"
                   + "Block CRC: " + std::to_string(block_crc) + "\n";
        }

    };

    class DataBlock {
        DataBlock(std::vector<double_t> data, std::string compression) {
            this->compression = compression;
            compress_data(data);
        }

        DataBlock(std::vector<uint8_t> compressed_data, std::string compression) {
            this->compressed_data = compressed_data;
            this->compression = compression;
        }

    public:
        std::vector<double_t> get_data() {
            return decompress_data();
        }

        [[nodiscard]] std::vector<uint8_t> get_compressed_data() const {
            return compressed_data;
        }

        [[nodiscard]] std::string get_compression() const {
            return compression;
        }

        [[nodiscard]] std::string to_string() const {
            return "Compression: " + compression + "\n"
                   + "Data size: " + std::to_string(compressed_data.size()) + " bytes\n";
        }

    protected:
        std::vector<uint8_t> compressed_data;
        std::string compression = "";

        void compress_data(std::vector<double_t>& data) {
            if (compression == "gorilla") {
                compressed_data = GorillaCompressor().compress(data);
            } else if (compression == "") {
                compressed_data = std::vector<uint8_t>(data.size() * sizeof(double_t));
                std::memcpy(compressed_data.data(), data.data(), data.size() * sizeof(double_t));
            }
            else {
                throw std::runtime_error("Unsupported compression type: " + compression);
            }
        }

        std::vector<double_t> decompress_data() {
            if (compression == "gorilla") {
                return GorillaDecompressor().decompress(compressed_data);
            } else if (compression == "") {
                std::vector<double_t> data(compressed_data.size() / sizeof(double_t));
                std::memcpy(data.data(), compressed_data.data(), compressed_data.size());
                return data;
            } else {
                throw std::runtime_error("Unsupported compression type: " + compression);
            }

        }

    };

    class FileHeader {
    protected:
        std::int64_t number_of_bytes_header = 2048;
        std::int64_t number_of_bytes_str = 100;
        bool initiated = false;

    public:
        std::string file_type = "";
        std::string version = "";
        std::string compression = "";
        double_t sampling_rate = -1;

        FileHeader() = default;

        explicit FileHeader(const std::string &file_type, const std::string &version, const std::string &compression,
                            double sampling_rate) {
            this->file_type = file_type;
            this->version = version;
            this->compression = compression;
            this->sampling_rate = sampling_rate;
        }

        explicit FileHeader(const std::vector<uint8_t> &data) {
            uint64_t offset = 0;

            file_type = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            version = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            compression = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            sampling_rate = neuro21::SerializeDouble::deserialize(data, offset);
            int32_t crc_value = neuro21::SerializeInt32::deserialize(data, offset);

            if (crc_value != this->crc()) {
                throw std::runtime_error("CRC mismatch");
            }
        }

        [[nodiscard]] std::vector<uint8_t> serialize_header_wo_crc() const {
            std::vector<uint8_t> data;
            data.reserve(number_of_bytes_header);

            neuro21::SerializeString::serialize(file_type, data, number_of_bytes_str);
            neuro21::SerializeString::serialize(version, data, number_of_bytes_str);
            neuro21::SerializeString::serialize(compression, data, number_of_bytes_str);
            neuro21::SerializeDouble::serialize(sampling_rate, data);
            return data;
        }

        [[nodiscard]] std::vector<uint8_t> serialize_header() const {
            std::vector<uint8_t> data = serialize_header_wo_crc();
            neuro21::SerializeInt32::serialize(neuro21::CRC::calculate(data), data);
            data.insert(data.end(), number_of_bytes_header - data.size(), 0);
            return data;
        }

        [[nodiscard]] std::uint64_t header_size_wo_crc() const {
            int64_t size_bytes = 0;

            size_bytes += number_of_bytes_str * 3; // file_type, version, compression
            size_bytes += sizeof(double_t); // sampling_rate
            return size_bytes;
        }

        [[nodiscard]] int32_t crc() const {
            return neuro21::CRC::calculate(serialize_header_wo_crc());
        }

        [[nodiscard]] uint64_t get_number_of_bytes_header() const {
            return number_of_bytes_header;
        }

        [[nodiscard]] uint64_t get_number_of_bytes_str() const {
            return number_of_bytes_str;
        }

        [[nodiscard]] std::string to_string() const {
            return "File type: " + file_type + "\n"
                   + "Version: " + version + "\n"
                   + "Compression: " + compression + "\n"
                   + "Sampling rate: " + std::to_string(sampling_rate) + "\n";
        }

    };

    class MetadataFile {
    private:
        std::string path;
        std::string mode;

        void _readHeader() {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            std::vector<uint8_t> data(header.get_number_of_bytes_header());
            file.read(reinterpret_cast<char*>(data.data()), header.get_number_of_bytes_header());
            header = FileHeader(data);
        }


    public:
        FileHeader header;
        std::fstream file;

        explicit MetadataFile(const std::string& path_file, const std::string& mode) {
            path = path_file;
        }

        ~MetadataFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void openFile(const std::string& path_file, const std::string& mode) {
            if (mode == "r") {
                file.open(path_file, std::ios::in | std::ios::binary);
            } else if (mode == "w") {
                file.open(path_file, std::ios::out | std::ios::binary);
            } else if (mode == "a") {
                file.open(path_file, std::ios::out | std::ios::app | std::ios::binary);
            } else {
                throw std::invalid_argument("Invalid file mode. Use 'r', 'w', or 'a' for read, write or append.");
            }

            if (!file.is_open()) {
                throw std::ios_base::failure("Failed to open file: " + path_file);
            }
        }

        void closeFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void writeHeader() {
            if (!file.is_open()) {
                openFile(path, "w");
            }
            std::vector<uint8_t> data = header.serialize_header();
            file.write(reinterpret_cast<char*>(data.data()), data.size());
            closeFile();
        }

        void readHeader() {
            if (!file.is_open()) {
                openFile(path, "r");
            }
            _readHeader();
            closeFile();
        }



    };

} // neuro21


