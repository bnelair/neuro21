#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>

namespace neuro21{

class BitWriter {
    std::vector<uint8_t> buffer;
    uint8_t bit_buffer = 0;
    int bit_count = 0;

    public:
        void write_bit(uint8_t bit) {
            if (bit != 0 && bit != 1) throw std::runtime_error("Bit must be 0 or 1");
            bit_buffer = (bit_buffer << 1) | bit;
            bit_count++;
            if (bit_count == 8) {
                buffer.push_back(bit_buffer);
                bit_buffer = 0;
                bit_count = 0;
            }
        }

        void write_bits(uint64_t value, int n_bits) {
            for (int i = n_bits - 1; i >= 0; --i) write_bit((value >> i) & 1);
        }

        std::vector<uint8_t> get_bytes() {
            if (bit_count > 0) {
                bit_buffer <<= (8 - bit_count);
                buffer.push_back(bit_buffer);
            }
            return buffer;
        };
};

class BitReader {
    const std::vector<uint8_t>& data;
    size_t byte_pos = 0;
    int bit_pos = 0;

    public:
        explicit BitReader(const std::vector<uint8_t>& data) : data(data), byte_pos(0), bit_pos(0) {}

        uint8_t read_bit() {
            if (byte_pos >= data.size()) throw std::runtime_error("End of data reached");
            uint8_t bit = (data[byte_pos] >> (7 - bit_pos)) & 1;
            if (++bit_pos == 8) {
                bit_pos = 0;
                byte_pos++;
            }
            return bit;
        }

        uint64_t read_bits(int64_t n_bits) {
            uint64_t result = 0;
            for (int64_t i = 0; i < n_bits; ++i) result = (result << 1) | read_bit();
            return result;
        }
};

class GorillaCompressor {
    BitWriter out;
    uint64_t previous_value = 0;
    int previous_leading_zeroes = -1;
    int previous_trailing_zeroes = -1;
    bool has_previous_value = false;

    int count_leading_zero_bits(uint64_t x) const {
        return x == 0 ? 64 : __builtin_clzll(x);
    }

    int count_trailing_zero_bits(uint64_t x) const {
        return x == 0 ? 64 : __builtin_ctzll(x);
    }

    void compress_value(double_t value) {
        uint64_t value_long;
        std::memcpy(&value_long, &value, sizeof(double_t));
        if (!has_previous_value) {
            out.write_bits(value_long, 64);
            has_previous_value = true;
        } else {
            uint64_t xor_val = previous_value ^ value_long;
            if (xor_val == 0) {
                out.write_bit(0);
            } else {
                out.write_bit(1);
                int leading_zeroes = count_leading_zero_bits(xor_val);
                int trailing_zeroes = count_trailing_zero_bits(xor_val);
                int significant_bits = 64 - leading_zeroes - trailing_zeroes;

                if (previous_leading_zeroes != -1 &&
                    leading_zeroes >= previous_leading_zeroes &&
                    trailing_zeroes >= previous_trailing_zeroes) {
                    out.write_bit(0);
                    uint64_t value_bits = xor_val >> previous_trailing_zeroes;
                    out.write_bits(value_bits, 64 - previous_leading_zeroes - previous_trailing_zeroes);
                } else {
                    out.write_bit(1);
                    out.write_bits(leading_zeroes, 5);
                    out.write_bits(significant_bits - 1, 6);
                    uint64_t value_bits = xor_val >> trailing_zeroes;
                    out.write_bits(value_bits, significant_bits);
                    previous_leading_zeroes = leading_zeroes;
                    previous_trailing_zeroes = trailing_zeroes;
                }
            }
        }
        previous_value = value_long;
    }

public:
    std::vector<uint8_t> compress(const std::vector<double_t>& data_points) {
        out.write_bits(data_points.size(), 64);
        for (double_t value : data_points) compress_value(value);
        return out.get_bytes();
    }
};

class GorillaDecompressor {
    public:
        static std::vector<double_t> decompress(std::vector<uint8_t>& compressed_data) {
            BitReader reader(compressed_data);
            size_t len = reader.read_bits(64);
            std::vector<double_t> result(len);

            uint64_t previous_value = 0;
            int previous_leading_zeroes = -1;
            int previous_trailing_zeroes = -1;
            bool has_previous_value = false;

            for (size_t i = 0; i < len; ++i) {
                if (!has_previous_value) {
                    previous_value = reader.read_bits(64);
                    std::memcpy(&result[i], &previous_value, sizeof(double_t));
                    has_previous_value = true;
                } else {
                    if (reader.read_bit() == 0) {
                        std::memcpy(&result[i], &previous_value, sizeof(double_t));
                    } else {
                        if (reader.read_bit() == 0) {
                            int64_t nbits = 64 - previous_leading_zeroes - previous_trailing_zeroes;
                            uint64_t value_bits = reader.read_bits(nbits);
                            uint64_t xor_val = value_bits << previous_trailing_zeroes;
                            previous_value ^= xor_val;
                        } else {
                            int64_t leading_zeroes = reader.read_bits(5);
                            int64_t significant_bits = reader.read_bits(6) + 1;
                            int64_t trailing_zeroes = 64 - leading_zeroes - significant_bits;
                            uint64_t value_bits = reader.read_bits(significant_bits);
                            uint64_t xor_val = value_bits << trailing_zeroes;
                            previous_value ^= xor_val;
                            previous_leading_zeroes = leading_zeroes;
                            previous_trailing_zeroes = trailing_zeroes;
                        }
                        std::memcpy(&result[i], &previous_value, sizeof(double_t));
                    }
                }
            }

            return result;
        }
};

} // namespace neuro21
