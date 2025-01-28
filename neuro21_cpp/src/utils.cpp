#include <cstdint>
#include <vector>

#include <cmath>
#include <memory>
#include <random>

namespace neuro21 {

    class Timestamp {
    public:
        static int64_t get_current_timestamp_usec() {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            return microseconds;
        }

    };

    class Generate{
    public:
        static std::vector<double_t> generate_cumulative_random_data(size_t size) {
            std::vector<double_t> random_data(size);
            std::vector<double_t> cumulative_data(size);

            std::mt19937 gen(42);  // Use a fixed seed for reproducibility
            std::normal_distribution<> dist(0.0, 1.0); // Standard normal distribution

            double_t cumulative_sum = 0.0;
            for (size_t i = 0; i < size; ++i) {
                random_data[i] = dist(gen);
                cumulative_sum += random_data[i];
                cumulative_data[i] = std::round(cumulative_sum);
            }

            return cumulative_data;
        }

        static std::vector<double_t> generate_random_samples(size_t count) {
            std::vector<double_t> samples;
            samples.reserve(count);
            srand(static_cast<unsigned>(time(nullptr))); // Seed for random number generation
            for (size_t i = 0; i < count; ++i) {
                samples.push_back(static_cast<double_t>(rand()) / RAND_MAX * 100.0); // Random doubles between 0 and 100
            }
            return samples;
        }
    };

    class CRC {
    public:
        static uint32_t calculate(const std::vector<uint8_t>& data, uint32_t initialValue = 0xFFFFFFFF) {
            const uint32_t polynomial = 0xEDB88320;
            uint32_t crc = initialValue;

            for (uint8_t byte : data) {
                crc ^= byte;  // XOR the input byte into the least significant byte of the CRC
                for (int i = 0; i < 8; ++i) {  // Process 8 bits
                    if (crc & 1) {  // Check if LSB is set
                        crc = (crc >> 1) ^ polynomial;  // XOR with polynomial
                    } else {
                        crc >>= 1;  // Just shift right
                    }
                }
            }

            return crc ^ 0xFFFFFFFF;  // Final XOR to invert the bits
        };
    };

    class SerializeString {
    public:
        static void serialize(const std::string& str, std::vector<uint8_t>& data, uint64_t max_str_len) {
            if (max_str_len == 0) {
                throw std::invalid_argument("max_str_len must be greater than 0");
            }

            if (str.size() >= max_str_len) {
                data.reserve(max_str_len);
                data.insert(data.end(), str.begin(), str.begin() + max_str_len);
            } else {
                data.reserve(max_str_len);
                data.insert(data.end(), str.begin(), str.end());
                data.insert(data.end(), max_str_len - str.size(), '\0');
            }
        }

        static std::string deserialize(const std::vector<uint8_t>& data, uint64_t& offset, uint64_t str_len) {
            if (str_len == 0) {
                throw std::invalid_argument("max_str_len must be greater than 0");
            }
            if (offset + str_len > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }

            std::string str(data.begin() + offset, data.begin() + offset + str_len);
            offset += str_len;

            while (!str.empty() && str.back() == '\0') {
                str.pop_back();
            }
            return str;
        }
    };

    class SerializeDouble {
    public:
        static void serialize(const double& value, std::vector<uint8_t>& data) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                        reinterpret_cast<const uint8_t*>(&value) + sizeof(double));
        }

        static double deserialize(const std::vector<uint8_t>& data, uint64_t& offset) {
            if (offset + sizeof(double) > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }

            double value = *reinterpret_cast<const double*>(&data[offset]);
            offset += sizeof(double);
            return value;
        }
    };

    class SerializeInt32 {
    public:
        static void serialize(const int32_t& value, std::vector<uint8_t>& data) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                        reinterpret_cast<const uint8_t*>(&value) + sizeof(int64_t));
        }

        static int32_t deserialize(const std::vector<uint8_t>& data, uint64_t& offset) {
            if (offset + sizeof(int32_t) > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }
            int32_t value = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += sizeof(int32_t);
            return value;
        }
    };

    class SerializeUInt32 {
    public:
        static void serialize(const uint32_t& value, std::vector<uint8_t>& data) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                        reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
        }

        static uint32_t deserialize(const std::vector<uint8_t>& data, uint64_t& offset) {
            if (offset + sizeof(uint32_t) > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }
            uint32_t value = *reinterpret_cast<const uint32_t*>(&data[offset]);
            offset += sizeof(uint32_t);
            return value;
        }
    };

    class SerializeInt64 {
    public:
        static void serialize(const int64_t& value, std::vector<uint8_t>& data) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                        reinterpret_cast<const uint8_t*>(&value) + sizeof(int64_t));
        }

        static int64_t deserialize(const std::vector<uint8_t>& data, uint64_t& offset) {
            if (offset + sizeof(int64_t) > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }
            int64_t value = *reinterpret_cast<const int64_t*>(&data[offset]);
            offset += sizeof(int64_t);
            return value;
        }
    };

    class SerializeUInt64 {
    public:
        static void serialize(const uint64_t& value, std::vector<uint8_t>& data) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                        reinterpret_cast<const uint8_t*>(&value) + sizeof(uint64_t));
        }

        static uint64_t deserialize(const std::vector<uint8_t>& data, uint64_t& offset) {
            if (offset + sizeof(uint64_t) > data.size()) {
                throw std::out_of_range("Insufficient data for deserialization");
            }
            uint64_t value = *reinterpret_cast<const uint64_t*>(&data[offset]);
            offset += sizeof(uint64_t);
            return value;
        }
    };


} // namespace neuro21