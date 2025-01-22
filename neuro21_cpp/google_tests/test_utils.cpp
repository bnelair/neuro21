#include <vector>
#include <cstdlib>
#include <ctime>
#include <gtest/gtest.h>

#include "../src/utils.cpp"


// ##################################################
// ########## Cumulative sum generation #############
// ##################################################
TEST(DataGenerationTest, GenerateCumulativeRandomDataTest) {
    const size_t test_size = 10;
    std::vector<double> result = neuro21::Generate::generate_cumulative_random_data(test_size);
    ASSERT_EQ(result.size(), test_size) << "The result size should match the input size.";
}

TEST(DataGenerationTest, GebnerateRandomSamplesTest) {
    const size_t test_size = 10;
    std::vector<double> result = neuro21::Generate::generate_random_samples(test_size);
    ASSERT_EQ(result.size(), test_size) << "The result size should match the input size.";
}

// ##################################################
// ################## CRC32 func ####################
// ##################################################

// validation of the expected values towards a python zlib implementation.
TEST(CRCTest, TestEmptyData) {
    std::vector<uint8_t> data = {};
    uint32_t expected_crc = 0x00000000;  // CRC of an empty sequence
    EXPECT_EQ(neuro21::CRC::calculate(data), expected_crc);
}

TEST(CRCTest, TestSingleByte) {
    std::vector<uint8_t> data = {0xAB};
    uint32_t expected_crc = 0x930695ed;  // Precomputed CRC for the data
    EXPECT_EQ(neuro21::CRC::calculate(data), expected_crc);
}

TEST(CRCTest, TestMultipleBytes) {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    uint32_t expected_crc = 0x7c9ca35a;  // Precomputed CRC for the data
    EXPECT_EQ(neuro21::CRC::calculate(data), expected_crc);
}


// ##################################################
// ################## Serialization ####################
// ##################################################

TEST(Serialize, TestSerializeString) {
    std::string test_string = "Hello, World!";
    uint64_t max_str_len = 100;
    uint64_t offset = 0;
    std::vector<uint8_t> data;

    neuro21::SerializeString::serialize(test_string, data, max_str_len);
    std::string deserialized = neuro21::SerializeString::deserialize(data, offset, max_str_len);

    EXPECT_EQ(test_string, deserialized);
}

TEST(Serialize, TestSerializeDouble) {
    double test_double = 3.14159;
    uint64_t offset = 0;
    std::vector<uint8_t> data;

    neuro21::SerializeDouble::serialize(test_double, data);
    double deserialized = neuro21::SerializeDouble::deserialize(data, offset);

    EXPECT_EQ(test_double, deserialized);
}

TEST(Serialize, TestSerializeInt) {
    int64_t test_int = 42;
    uint64_t offset = 0;
    std::vector<uint8_t> data;

    neuro21::SerializeInt32::serialize(test_int, data);
    int64_t deserialized = neuro21::SerializeInt32::deserialize(data, offset);

    EXPECT_EQ(test_int, deserialized);
}




