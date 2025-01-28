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



namespace fs = std::filesystem;

namespace neuro21 {

    class MetadataBlock{
    private:
        int64_t start_uutc;
        int64_t end_uutc;
        uint64_t block_start;
        uint64_t block_n_bytes;
        uint32_t block_crc_decompressed;
        uint32_t block_crc_compressed;

    public:
        MetadataBlock(int64_t start_uutc, int64_t end_uutc, uint64_t block_start, uint64_t block_n_bytes, uint32_t block_crc_decompressed, uint32_t block_crc_compressed) {
            this->start_uutc = start_uutc;
            this->end_uutc = end_uutc;
            this->block_start = block_start;
            this->block_n_bytes = block_n_bytes;
            this->block_crc_decompressed = block_crc_decompressed;
            this->block_crc_compressed = block_crc_compressed;
        }

        MetadataBlock(const std::vector<uint8_t>& data) {
            uint64_t offset = 0;
            start_uutc = neuro21::SerializeInt64::deserialize(data, offset);
            end_uutc = neuro21::SerializeInt64::deserialize(data, offset);
            block_start = neuro21::SerializeUInt64::deserialize(data, offset);
            block_n_bytes = neuro21::SerializeUInt64::deserialize(data, offset);
            block_crc_decompressed = neuro21::SerializeUInt32::deserialize(data, offset);
            block_crc_compressed = neuro21::SerializeUInt32::deserialize(data, offset);
        }

        [[nodiscard]] std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> data;
            neuro21::SerializeInt64::serialize(start_uutc, data);
            neuro21::SerializeInt64::serialize(end_uutc, data);
            neuro21::SerializeInt64::serialize(block_start, data);
            neuro21::SerializeInt64::serialize(block_n_bytes, data);
            neuro21::SerializeUInt32::serialize(block_crc_decompressed, data);
            neuro21::SerializeUInt32::serialize(block_crc_compressed, data);
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
            return block_n_bytes;
        }

        [[nodiscard]] uint32_t get_block_crc_compressed() const {
            return block_crc_compressed;
        }

        [[nodiscard]] uint32_t get_block_crc_decompressed() const {
            return block_crc_decompressed;
        }

        [[nodiscard]] uint32_t get_metadata_crc() const {
            std::vector<uint8_t> data = serialize();
            return neuro21::CRC::calculate(data);
        }

        [[nodiscard]] static uint64_t get_number_of_metadata_block_bytes() {
            return 8 + 8 + 8 + 8 + 4 + 4;
        }

    };

    class DataBlock {
    public:

        DataBlock(std::vector<double_t> data, std::string compression) {
            this->compression = compression;
            this->crc_decompressed_value = calculate_crc(data);
            compress_data(data);
            this->crc_compressed_value = calculate_compressed_crc();
        }

        DataBlock(std::vector<uint8_t> compressed_data, std::string compression) {
            this->compression = compression;
            this->compressed_data = compressed_data;
            this->crc_compressed_value = calculate_compressed_crc();
        }

        std::vector<double_t> get_data() {
            auto data = decompress_data();
            return data;
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

        [[nodiscard]] uint32_t get_decompressed_crc() const {
            return crc_decompressed_value;
        }

        [[nodiscard]] uint32_t get_compressed_crc() const {
            return crc_compressed_value;
        }

        [[nodiscard]] uint32_t calculate_compressed_crc() const {
            return calculate_crc(compressed_data);
        }

        [[nodiscard]] uint32_t calculate_decompressed_crc() {
            std::vector<double_t> data = get_data();
            uint32_t crc = calculate_crc(data);
            return crc;
        }

        [[nodiscard]] static uint32_t calculate_crc(const std::vector<uint8_t>& data) {
            return neuro21::CRC::calculate(data);
        }

        [[nodiscard]] static uint32_t calculate_crc(std::vector<double_t> data) {
            std::vector<uint8_t> data_bytes = std::vector<uint8_t>(data.size() * sizeof(double_t));
            std::memcpy(data_bytes.data(), data.data(), data.size() * sizeof(double_t));
            return calculate_crc(data_bytes);
        }

    protected:
        std::vector<uint8_t> compressed_data;
        std::string compression;
        std::uint32_t crc_decompressed_value = 0;
        std::uint32_t crc_compressed_value = 0;

        void compress_data(std::vector<double_t>& data) {
            if (compression == "gorilla") {
                compressed_data = GorillaCompressor().compress(data);
            } else if (compression.empty()) {
                compressed_data = std::vector<uint8_t>(data.size() * sizeof(double_t));
                std::memcpy(compressed_data.data(), data.data(), data.size() * sizeof(double_t));
            }
            else {
                throw std::runtime_error("Unsupported compression type: " + compression);
            }
        }

        std::vector<double_t> decompress_data() {
            if (compression == "gorilla") {
                auto decompressed_data = GorillaDecompressor().decompress(compressed_data);
                return decompressed_data;
            } else if (compression.empty()) {
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
        static constexpr std::int64_t number_of_bytes_header = 2048;
        static constexpr std::int64_t number_of_bytes_str = 100;
        std::string version = "0.0.1";

    public:
        std::string file_type;

        std::string patient_id;
        std::string session_id;
        std::string channel_id;
        std::string compression;
        double_t sampling_rate = -1;

        FileHeader() = default;

        explicit FileHeader(
                const std::string &file_type,
                const std::string &version,
                const std::string &patient_id,
                const std::string &session_id,
                const std::string &channel_id,
                const std::string &compression,
                const double_t sampling_rate
                ) {

            this->file_type = file_type;
            this->version = version;

            this->patient_id = patient_id;
            this->session_id = session_id;
            this->channel_id = channel_id;

            this->compression = compression;
            this->sampling_rate = sampling_rate;
        }

        explicit FileHeader(const std::vector<uint8_t> &data) {
            uint64_t offset = 0;
            uint64_t data_start_byte = 0;

            file_type = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            version = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);

            patient_id = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            session_id = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            channel_id = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);

            compression = neuro21::SerializeString::deserialize(data, offset, number_of_bytes_str);
            sampling_rate = neuro21::SerializeDouble::deserialize(data, offset);

            data_start_byte = neuro21::SerializeUInt64::deserialize(data, offset);
            uint32_t crc_value = neuro21::SerializeUInt32::deserialize(data, offset);

            if (crc_value != this->crc()) {
                throw std::runtime_error("CRC mismatch");
            }
        }

        [[nodiscard]] std::vector<uint8_t> serialize_header_wo_crc() const {
            std::vector<uint8_t> data;
            data.reserve(number_of_bytes_header);

            neuro21::SerializeString::serialize(file_type, data, number_of_bytes_str);
            neuro21::SerializeString::serialize(version, data, number_of_bytes_str);

            neuro21::SerializeString::serialize(patient_id, data, number_of_bytes_str);
            neuro21::SerializeString::serialize(session_id, data, number_of_bytes_str);
            neuro21::SerializeString::serialize(channel_id, data, number_of_bytes_str);

            neuro21::SerializeString::serialize(compression, data, number_of_bytes_str);
            neuro21::SerializeDouble::serialize(sampling_rate, data);
            neuro21::SerializeUInt64::serialize(number_of_bytes_header, data);
            return data;
        }

        [[nodiscard]] std::vector<uint8_t> serialize_header() const {
            std::vector<uint8_t> data = serialize_header_wo_crc();
            neuro21::SerializeUInt32::serialize(neuro21::CRC::calculate(data), data);
            data.insert(data.end(), number_of_bytes_header - data.size(), 0);
            return data;
        }

        [[nodiscard]] std::uint64_t header_size_wo_crc() const {
            int64_t size_bytes = 0;

            size_bytes += number_of_bytes_str * 6; // file_type, version, patient_id, session_id, channel_id, compression
            size_bytes += sizeof(double_t); // sampling_rate
            return size_bytes;
        }

        [[nodiscard]] std::uint64_t header_size() const {
            return header_size_wo_crc() + sizeof(uint32_t);
        }

        [[nodiscard]] uint32_t crc() const {
            return neuro21::CRC::calculate(serialize_header_wo_crc());
        }

        [[nodiscard]] static uint64_t get_number_of_bytes_header()  {
            return number_of_bytes_header;
        }

        [[nodiscard]] std::string get_file_header_version() const {
            return version;
        }

        [[nodiscard]] uint64_t get_number_of_bytes_str() const {
            return number_of_bytes_str;
        }

        [[nodiscard]] std::string to_string() const {
            return "File type: " + file_type + "\n"
                   + "Version: " + version + "\n"
                     + "Patient ID: " + patient_id + "\n"
                     + "Session ID: " + session_id + "\n"
                     + "Channel ID: " + channel_id + "\n"
                   + "Compression: " + compression + "\n"
                   + "Sampling rate: " + std::to_string(sampling_rate) + "\n";
        }

        bool operator==(const FileHeader& other) const {
            return file_type == other.file_type
                   && version == other.version
                   && patient_id == other.patient_id
                   && session_id == other.session_id
                   && channel_id == other.channel_id
                   && compression == other.compression
                   && sampling_rate == other.sampling_rate
                   && get_number_of_bytes_header() == other.get_number_of_bytes_header()
                   && crc() == other.crc();
        }
    };

    class GenericFile {
    protected:
        fs::path path_file;
        std::fstream file;
        FileHeader header;
        bool file_initialized = false;

        void _readHeader() {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            std::vector<uint8_t> data(header.get_number_of_bytes_header());
            file.read(reinterpret_cast<char*>(data.data()), header.get_number_of_bytes_header());
            header = FileHeader(data);
        }

        void _writeHeader() {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            std::vector<uint8_t> data = header.serialize_header();
            file.write(reinterpret_cast<char*>(data.data()), header.get_number_of_bytes_header());
        }

        void _updateHeader() {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            std::vector<uint8_t> data = header.serialize_header();
            file.seekp(0, std::ios::beg); // Move to the beginning of the file
            file.write(reinterpret_cast<char*>(data.data()), header.get_number_of_bytes_header());
        }

        std::vector<uint8_t> _readDataBytes(uint64_t offset, uint64_t length) {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            std::vector<uint8_t> data(length);
            file.seekg(offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(data.data()), length);
            return data;

        }

        void _writeDataBytes(const std::vector<uint8_t>& data, uint64_t offset) {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }

            file.seekp(offset, std::ios::beg);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        void _appendDataBytes(const std::vector<uint8_t>& data) {
            if (!file.is_open()) {
                throw std::runtime_error("File is not open.");
            }
            file.seekp(0, std::ios::end);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        void openFile(const fs::path& path_file, const std::string& mode) {
            if (mode == "r") {
                // if does not exists, throw error
                if (!fs::exists(path_file)) {
                    throw std::runtime_error("File does not exist: " + path_file.string());
                }
                file.open(path_file, std::ios::in | std::ios::binary);
            } else if (mode == "w") {
                file.open(path_file, std::ios::out | std::ios::binary);
            } else if (mode == "a") {
                file.open(path_file, std::ios::in | std::ios::out | std::ios::binary);
            } else {
                throw std::invalid_argument("Invalid file mode. Use 'r', 'w', or 'a' for read, write or append.");
            }

            if (!file.is_open()) {
                throw std::ios_base::failure("Failed to open file: " + path_file.string());
            }
        }

        void closeFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void overWriteFileWithNewHeader() {
            if (!file.is_open()) {
                openFile(path_file, "w");
            }
            _writeHeader();
            closeFile();
            file_initialized = true;
        }

        void readHeader() {
            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            _readHeader();
            closeFile();
            file_initialized = true;
        }

    public:

        explicit GenericFile(const fs::path& path_file) {
            this->path_file = path_file;
            if (file_exists()) {
                readHeader();
            }
            else {
                throw std::runtime_error("File does not exist: " + path_file.string());
            }
        }

        explicit GenericFile(const fs::path& path_file, FileHeader header) {
            this->path_file = path_file;
            this->header = std::move(header);

            overWriteFileWithNewHeader();
        }

        ~GenericFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        neuro21::FileHeader getHeader() {
            return header;
        }

        void updateExistingHeader(neuro21::FileHeader new_header) {
            header = new_header;

            if (!file.is_open()) {
                openFile(path_file, "a");
            }
            _updateHeader();
            closeFile();
        }



        static bool file_exists(const fs::path& path_file) {
            return fs::exists(path_file);
        }

        static bool file_exists(const std::string& path_file) {
            fs::path path(path_file);
            return file_exists(path);
        }

        bool file_exists() {
            return file_exists(path_file);
        }





    };

    class MetadataFile : public GenericFile {
    public:

        explicit MetadataFile(const fs::path& path_file) : GenericFile(path_file) {}

        explicit MetadataFile(const fs::path& path_file, FileHeader header) : GenericFile(path_file, header) {}

        ~MetadataFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void writeMetadataBlock(const MetadataBlock& block) {
            // for streaming
            if (!file.is_open()) {
                openFile(path_file, "a");
            }
            std::vector<uint8_t> data = block.serialize();
            _appendDataBytes(data);
            closeFile();
        }

        void writeMultipleMetadataBlocks(const std::vector<MetadataBlock>& block) {
            // for bulk writing
            if (!file.is_open()) {
                openFile(path_file, "a");
            }
            for (const MetadataBlock& b : block) {
                std::vector<uint8_t> data = b.serialize();
                _appendDataBytes(data);
            }
            closeFile();
        }

        MetadataBlock readMetadataBlock(uint64_t block_index) {
            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            uint64_t offset = header.get_number_of_bytes_header() + block_index * MetadataBlock::get_number_of_metadata_block_bytes();

            std::vector<uint8_t> data = _readDataBytes(offset, MetadataBlock::get_number_of_metadata_block_bytes());
            MetadataBlock read_block = MetadataBlock(data);
            closeFile();
            return read_block;
        }

        std::vector<MetadataBlock> readMultipleMetadataBlocks(uint64_t block_index, uint64_t block_count) {
            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            std::vector<MetadataBlock> blocks;
            for (int i = 0; i < block_count; i++) {

                uint64_t offset = header.get_number_of_bytes_header() + i * MetadataBlock::get_number_of_metadata_block_bytes();
                uint64_t n_bytes = MetadataBlock::get_number_of_metadata_block_bytes();
                std::vector<uint8_t> data = _readDataBytes(offset, n_bytes);
                MetadataBlock read_block(data);

                blocks.push_back(
                        read_block
                        );
            }
            closeFile();
            return blocks;
        }
    };

    class DataFile : public GenericFile {
    public:
        explicit DataFile(const fs::path& path_file) : GenericFile(path_file) {}

        explicit DataFile(const fs::path& path_file, FileHeader header) : GenericFile(path_file, header) {}

        ~DataFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void writeDataBlock(const DataBlock& block) {
            // for streaming
            if (!file.is_open()) {
                openFile(path_file, "a");
            }
            std::vector<uint8_t> data = block.get_compressed_data();
            _appendDataBytes(data);
            closeFile();
        }

        DataBlock readDataBlock(MetadataBlock& metadata_block) {
            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            std::vector<uint8_t> data = _readDataBytes(metadata_block.get_block_start(), metadata_block.get_block_length());
            DataBlock read_block = DataBlock(data, header.compression);
            closeFile();
            return read_block;
        }





        void writeMultipleDataBlocks(const std::vector<DataBlock>& block) {
            // for bulk writing
            if (!file.is_open()) {
                openFile(path_file, "a");
            }
            for (const DataBlock& b : block) {
                std::vector<uint8_t> data = b.get_compressed_data();
                _appendDataBytes(data);
            }
            closeFile();
        }

        std::vector<DataBlock> readMultipleDataBlocks(const std::vector<MetadataBlock>& metadata_blocks) {
            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            std::vector<DataBlock> blocks;
            for (const MetadataBlock& metadata_block : metadata_blocks) {
                std::vector<uint8_t> data = _readDataBytes(metadata_block.get_block_start(), metadata_block.get_block_length());
                blocks.emplace_back(data, header.compression);
            }
            closeFile();
            return blocks;
        }


        uint64_t get_file_length() {
            if (!fs::exists(path_file)) {
                return 0;
            }

            if (!file.is_open()) {
                openFile(path_file, "r");
            }
            file.seekg(0, std::ios::end);
            uint64_t length = file.tellg();
            closeFile();
            return length;
        }
    };




} // neuro21


