#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>

#include "../src/data_handlers.cpp"

//TEST(DataHandlersTests, SerializeAndDeserialize) {
//    EXPECT_EQ("a", "a");
//}


TEST(DataHandlersTests, SerializeAndDeserialize) {
    // Create an instance of FileHeader
    neuro21::FileHeader originalHdr("FileTypeA", "1.0", "gzip", 44100.0);

    // Serialize the header
    std::vector<uint8_t> serialized_data = originalHdr.serialize_header();

    // Deserialize the header
    neuro21::FileHeader deserializedHdr = neuro21::FileHeader(serialized_data);

    // Verify that the deserialized data matches the original
    EXPECT_EQ(deserializedHdr.file_type, "FileTypeA");
    EXPECT_EQ(deserializedHdr.version, "1.0");
    EXPECT_EQ(deserializedHdr.compression, "gzip");
    EXPECT_DOUBLE_EQ(deserializedHdr.sampling_rate, 44100.0);
}


TEST(DataHandlersTests, TruncatedStrings) {
    // Test with a string longer than the maximum allowed length
    std::string long_file_type(150, 'A'); // 150 'A's
    neuro21::FileHeader original(long_file_type, "1.0", "gzip", 44100.0);

    // Serialize the header
    std::vector<uint8_t> serialized_data = original.serialize_header();

    // Deserialize the header
    neuro21::FileHeader deserialized(serialized_data);

    // Expect the file type to be truncated to `number_of_bytes_str`
    EXPECT_EQ(deserialized.file_type, std::string(100, 'A'));
}

TEST(DataHandlersTests, PaddedStrings) {
    // Test with a string shorter than the maximum allowed length
    neuro21::FileHeader original("Short", "1.0", "gzip", 44100.0);

    // Serialize the header
    std::vector<uint8_t> serialized_data = original.serialize_header();

    // Deserialize the header
    neuro21::FileHeader deserialized(serialized_data);

    // Expect the file type to match exactly
    EXPECT_EQ(deserialized.file_type, "Short");
}
