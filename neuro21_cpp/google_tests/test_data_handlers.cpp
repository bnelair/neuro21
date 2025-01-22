#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>

#include "../src/data_handlers.cpp"

TEST(FileHeaderTests, FileHeader) {
    // Create an instance of FileHeader
    neuro21::FileHeader originalHdr(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gzip",
            44100.0
            );

    // Serialize the header
    std::vector<uint8_t> serialized_data = originalHdr.serialize_header();
    EXPECT_EQ(originalHdr.get_number_of_bytes_header(), serialized_data.size());

    // Deserialize the header
    neuro21::FileHeader deserializedHdr = neuro21::FileHeader(serialized_data);

    // Verify that the deserialized data matches the original
    EXPECT_EQ(deserializedHdr.file_type, "FileTypeA");
    EXPECT_EQ(deserializedHdr.version, "0.0.1");
    EXPECT_EQ(deserializedHdr.patient_id, "pt_X");
    EXPECT_EQ(deserializedHdr.session_id, "session_Y");
    EXPECT_EQ(deserializedHdr.channel_id, "channel_Z");
    EXPECT_EQ(deserializedHdr.compression, "gzip");
    EXPECT_DOUBLE_EQ(deserializedHdr.sampling_rate, 44100.0);
}

TEST(FileHeaderTests, FileHeaderLongStr) {
    // Test with a string longer than the maximum allowed length
    std::string long_file_type(150, 'A'); // 150 'A's
    neuro21::FileHeader originalHdr(
            long_file_type,
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gzip",
            44100.0
    );

    // Serialize the header
    std::vector<uint8_t> serialized_data = originalHdr.serialize_header();

    // Deserialize the header
    neuro21::FileHeader deserialized(serialized_data);

    // Expect the file type to be truncated to `number_of_bytes_str`
    EXPECT_EQ(deserialized.file_type, std::string(100, 'A'));
}

TEST(DataBlockTests, DataBlockGorilla) {
    std::vector<double_t> input_data = neuro21::Generate::generate_cumulative_random_data(500);

    neuro21::DataBlock data_block(input_data, "gorilla");

    std::vector<double_t> decompressed_data = data_block.get_data();
    ASSERT_EQ(input_data, decompressed_data);

    std::vector<uint8_t> compressed_data = data_block.get_compressed_data();
    ASSERT_LT(compressed_data.size(), input_data.size());
}

TEST(MetadataBlockTests, MetadataBlock) {
    // Create an instance of MetadataBlock with test data
    int64_t start_uutc = 1672531200;
    int64_t end_uutc = 1672534800;
    uint64_t block_start = 1024;
    uint64_t block_length = 2048;
    uint64_t block_crc = 12345;

    neuro21::MetadataBlock originalBlock(start_uutc, end_uutc, block_start, block_length, block_crc);

    // Serialize the block
    std::vector<uint8_t> serialized_data = originalBlock.serialize();

    // Deserialize the block
    neuro21::MetadataBlock deserializedBlock(serialized_data);

    // Verify that the deserialized data matches the original
    EXPECT_EQ(deserializedBlock.get_start_uutc(), start_uutc);
    EXPECT_EQ(deserializedBlock.get_end_uutc(), end_uutc);
    EXPECT_EQ(deserializedBlock.get_block_start(), block_start);
    EXPECT_EQ(deserializedBlock.get_block_length(), block_length);
    EXPECT_EQ(deserializedBlock.get_block_crc(), block_crc);
}

TEST(MetadataBlockTests, MetadataBlockInvalidDeserialization) {
    // Create invalid serialized data (too short)
    std::vector<uint8_t> invalid_data(10);

    // Attempt to deserialize and expect an exception
    EXPECT_THROW({
                     neuro21::MetadataBlock invalidBlock(invalid_data);
                 }, std::out_of_range);
}

TEST(MetadataBlockTests, MetadataBlockIntegrity) {
    // Test serialization produces consistent results
    neuro21::MetadataBlock block1(1672531200, 1672534800, 1024, 2048, 12345);
    neuro21::MetadataBlock block2(1672531200, 1672534800, 1024, 2048, 12345);

    std::vector<uint8_t> serialized1 = block1.serialize();
    std::vector<uint8_t> serialized2 = block2.serialize();

    EXPECT_EQ(serialized1, serialized2);
}


namespace fs = std::filesystem;

class MetadataFileTest : public ::testing::Test {
protected:
    fs::path temp_folder;

    void SetUp() override {
        // Create a temporary folder for the test
        temp_folder = fs::temp_directory_path() / "test_temp_folder";
        create_directories(temp_folder);
    }

    void TearDown() override {
        // Remove the temporary folder and its contents
        if (exists(temp_folder)) {
            remove_all(temp_folder);
        }
    }
};

TEST(MetaDataFileTests, MetadataFile) {
    // Test serialization produces consistent results
    std::string path = "test_file.b";

    neuro21::FileHeader originalHdr(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gzip",
            44100.0
    );

    neuro21::MetadataFile metadataFileWrite(path);

    metadataFileWrite.setHeader(originalHdr);
    metadataFileWrite.writeHeader();

    neuro21::MetadataFile metadataFileRead(path);
    neuro21::FileHeader emptyHdr = metadataFileRead.getHeader();

    metadataFileRead.readHeader();
    neuro21::FileHeader readHdr = metadataFileRead.getHeader();

    EXPECT_EQ(originalHdr.file_type, readHdr.file_type);
    EXPECT_EQ(originalHdr.version, readHdr.version);
    EXPECT_EQ(originalHdr.patient_id, readHdr.patient_id);
    EXPECT_EQ(originalHdr.session_id, readHdr.session_id);
    EXPECT_EQ(originalHdr.channel_id, readHdr.channel_id);
    EXPECT_EQ(originalHdr.compression, readHdr.compression);
    EXPECT_DOUBLE_EQ(originalHdr.sampling_rate, readHdr.sampling_rate);
    EXPECT_EQ(originalHdr.get_number_of_bytes_header(), readHdr.get_number_of_bytes_header());
    EXPECT_EQ(originalHdr.crc(), readHdr.crc());
}


