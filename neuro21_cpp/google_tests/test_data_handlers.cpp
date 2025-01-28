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
            "gorilla",
            44100.0
            );

    // Serialize the header
    std::vector<uint8_t> serialized_data = originalHdr.serialize_header();
    EXPECT_EQ(originalHdr.get_number_of_bytes_header(), serialized_data.size());

    // Deserialize the header
    neuro21::FileHeader deserializedHdr = neuro21::FileHeader(serialized_data);

    // Verify that the deserialized data matches the original
    EXPECT_EQ(deserializedHdr.file_type, "FileTypeA");
    EXPECT_EQ(deserializedHdr.get_file_header_version(), "0.0.1");
    EXPECT_EQ(deserializedHdr.patient_id, "pt_X");
    EXPECT_EQ(deserializedHdr.session_id, "session_Y");
    EXPECT_EQ(deserializedHdr.channel_id, "channel_Z");
    EXPECT_EQ(deserializedHdr.compression, "gorilla");
    EXPECT_DOUBLE_EQ(deserializedHdr.sampling_rate, 44100.0);

    EXPECT_EQ(originalHdr == deserializedHdr, true);

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
            "gorilla",
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
    int32_t block_crc1 = 12345;
    int32_t block_crc2 = 54321;

    neuro21::MetadataBlock originalBlock(start_uutc, end_uutc, block_start, block_length, block_crc1, block_crc2);

    // Serialize the block
    std::vector<uint8_t> serialized_data = originalBlock.serialize();

    // Deserialize the block
    neuro21::MetadataBlock deserializedBlock(serialized_data);

    // Verify that the deserialized data matches the original
    EXPECT_EQ(deserializedBlock.get_start_uutc(), start_uutc);
    EXPECT_EQ(deserializedBlock.get_end_uutc(), end_uutc);
    EXPECT_EQ(deserializedBlock.get_block_start(), block_start);
    EXPECT_EQ(deserializedBlock.get_block_length(), block_length);
    EXPECT_EQ(deserializedBlock.get_block_crc_decompressed(), block_crc1);
    EXPECT_EQ(deserializedBlock.get_block_crc_compressed(), block_crc2);
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
    neuro21::MetadataBlock block1(1672531200, 1672534800, 1024, 2048, 12345, 54321);
    neuro21::MetadataBlock block2(1672531200, 1672534800, 1024, 2048, 12345, 54321);
    neuro21::MetadataBlock block3(1672531200, 1672534800, 1024, 2048, 12345, 987654);

    auto crc1 = block1.get_metadata_crc();
    auto crc2 = block2.get_metadata_crc();
    auto crc3 = block3.get_metadata_crc();

    EXPECT_EQ(crc1, crc2);
    EXPECT_NE(crc1, crc3);
}




class DataFileTests : public ::testing::Test {
protected:
    fs::path temp_folder;

    int64_t start_uutc = neuro21::Timestamp::get_current_timestamp_usec();
    int64_t block_count = 10;
    double_t block_len_s = 10;
    double_t fs = 1000.0;

    double_t samples_total = block_count * fs * block_len_s;
    int64_t end_uutc = start_uutc + (samples_total * 1000000 / fs);

    neuro21::FileHeader originalHdr = neuro21::FileHeader(
    "FileTypeA",
    "0.0.1",
    "pt_X",
    "session_Y",
    "channel_Z",
    "gorilla",
    fs
    );

    std::vector<double_t> data = neuro21::Generate::generate_cumulative_random_data(samples_total);
    std::vector<neuro21::MetadataBlock> metadata_blocks;
    std::vector<neuro21::DataBlock> data_blocks;

    int n_byte_previous_blocks = neuro21::FileHeader::get_number_of_bytes_header();



    void SetUp() override {
        // Create a temporary folder for the test
        temp_folder = fs::temp_directory_path() / "neuro21_test";
        create_directories(temp_folder);




        for (int i = 0; i < samples_total; i=i+(fs*block_len_s)) {
            int ie = i + (fs*block_len_s);

            int64_t start_block_uutc = start_uutc + (i * 1000000 / fs);
            int64_t end_block_uutc = start_uutc + (ie * 1000000 / fs);

            auto data_block_write = std::vector<double_t>(data.begin() + i, data.begin() + ie);
            neuro21::DataBlock data_block(data_block_write, "gorilla");
            int64_t  n_bytes_block = data_block.get_compressed_data().size();

            neuro21::MetadataBlock metadata_block(
                    start_block_uutc,
                    end_block_uutc,
                    n_byte_previous_blocks,
                    n_bytes_block,
                    data_block.get_decompressed_crc(),
                    data_block.get_compressed_crc()
            );

            metadata_blocks.push_back(metadata_block);
            data_blocks.push_back(data_block);
            n_byte_previous_blocks += n_bytes_block;
        }


    }

    void TearDown() override {
        // Remove the temporary folder and its contents
        if (exists(temp_folder)) {
            remove_all(temp_folder);
        }
    }
};


TEST_F(DataFileTests, GenericFileHeaderReadWrite) {
    // Test serialization produces consistent results
    fs::path temp_file_path = temp_folder / "metadata_test_file_0.b";

    neuro21::FileHeader origHdr = neuro21::FileHeader(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gorilla",
            44100.0
    );

    EXPECT_EQ(neuro21::GenericFile::file_exists(temp_file_path), false);
    neuro21::GenericFile dataFileWrite(temp_file_path, origHdr);
    EXPECT_EQ(neuro21::GenericFile::file_exists(temp_file_path), true);

    neuro21::MetadataFile genericFileRead(temp_file_path);

    neuro21::FileHeader readHdr = genericFileRead.getHeader();
    EXPECT_EQ(readHdr == origHdr, true);

    EXPECT_EQ(origHdr.file_type, readHdr.file_type);
    EXPECT_EQ(origHdr.get_file_header_version(), readHdr.get_file_header_version());
    EXPECT_EQ(origHdr.patient_id, readHdr.patient_id);
    EXPECT_EQ(origHdr.session_id, readHdr.session_id);
    EXPECT_EQ(origHdr.channel_id, readHdr.channel_id);
    EXPECT_EQ(origHdr.compression, readHdr.compression);
    EXPECT_DOUBLE_EQ(origHdr.sampling_rate, readHdr.sampling_rate);
    EXPECT_EQ(origHdr.get_number_of_bytes_header(), readHdr.get_number_of_bytes_header());
    EXPECT_EQ(origHdr.crc(), readHdr.crc());
}

TEST_F(DataFileTests, GenericFileHeaderReadNonExistent) {
    // Test serialization produces consistent results
    fs::path temp_file_path = temp_folder / "metadata_test_file_1.b";


    // read empty file -> std::runtime_error
    EXPECT_THROW({
                     neuro21::MetadataFile fileRead(temp_file_path);
                 }, std::runtime_error);
}

TEST_F(DataFileTests, GenericFileModifyHeader) {
    // Test serialization produces consistent results
    fs::path temp_file_path = temp_folder / "metadata_test_file_3.b";

    neuro21::FileHeader hdr1 = neuro21::FileHeader(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gorilla",
            44100.0
    );

    neuro21::FileHeader hdr2 = neuro21::FileHeader(
            "FileTypeB",
            "0.0.2",
            "pt_A",
            "session_B",
            "channel_C",
            "gorilla",
            22050.0
    );

    neuro21::GenericFile dataFileWrite(temp_file_path, hdr1);

    neuro21::GenericFile dataFileRead = neuro21::GenericFile(temp_file_path);
    neuro21::FileHeader readHdr1 = dataFileRead.getHeader();
    EXPECT_EQ(readHdr1 == hdr1, true);

    dataFileWrite.updateExistingHeader(hdr2);

    neuro21::GenericFile dataFileRead2 = neuro21::GenericFile(temp_file_path);
    neuro21::FileHeader readHdr2 = dataFileRead2.getHeader();
    EXPECT_EQ(readHdr2 == hdr2, true);


}



TEST_F(DataFileTests, MetaReadWriteHeader) {
    // Test serialization produces consistent results
    fs::path temp_file_path = temp_folder / "metadata_test_file_4.b";

    neuro21::FileHeader originalHdr(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gorilla",
            44100.0
    );

    neuro21::MetadataFile dataFileWrite(temp_file_path, originalHdr);

    neuro21::MetadataFile dataFileRead(temp_file_path);
    neuro21::FileHeader readHdr = dataFileRead.getHeader();

    EXPECT_EQ(readHdr == originalHdr, true);
}

TEST_F(DataFileTests, MetaFileReadWrite) {
    // Test serialization produces consistent results
    fs::path temp_file_path_meta = temp_folder / "metadata_test_file_3.n21_m";
    fs::path temp_file_path_data = temp_folder / "metadata_test_file_3.n21_d";

    neuro21::MetadataFile metadataFile(temp_file_path_meta, originalHdr);
    metadataFile.writeMetadataBlock(metadata_blocks[0]);
    metadataFile.writeMetadataBlock(metadata_blocks[1]);
    metadataFile.writeMetadataBlock(metadata_blocks[2]);

    neuro21::MetadataFile metadataRdFile(temp_file_path_meta);
    neuro21::MetadataBlock metaBlockRd1 = metadataRdFile.readMetadataBlock(0);
    neuro21::MetadataBlock metaBlockRd2 = metadataRdFile.readMetadataBlock(1);
    neuro21::MetadataBlock metaBlockRd3 = metadataRdFile.readMetadataBlock(2);

    auto crc_w_1 = metadata_blocks[0].get_metadata_crc();
    auto crc_w_2 = metadata_blocks[1].get_metadata_crc();
    auto crc_w_3 = metadata_blocks[2].get_metadata_crc();

    auto crc_r_1 = metaBlockRd1.get_metadata_crc();
    auto crc_r_2 = metaBlockRd2.get_metadata_crc();
    auto crc_r_3 = metaBlockRd3.get_metadata_crc();

    EXPECT_EQ(crc_w_1, crc_r_1);
    EXPECT_EQ(crc_w_2, crc_r_2);
    EXPECT_EQ(crc_w_3, crc_r_3);

}

TEST_F(DataFileTests, MetaFileMultipleReadWrites)  {
    // Test serialization produces consistent results
    fs::path temp_file_path_meta = temp_folder / "metadata_test_file_3.n21_m";
    fs::path temp_file_path_data = temp_folder / "metadata_test_file_3.n21_d";

    neuro21::MetadataFile metadataFile(temp_file_path_meta, originalHdr);
    metadataFile.writeMultipleMetadataBlocks(metadata_blocks);

    neuro21::MetadataFile metadataRdFile(temp_file_path_meta);

    for (int i = 0; i < metadata_blocks.size(); i++) {
        auto metaBlockRd = metadataRdFile.readMetadataBlock(i);
        auto crc_w = metadata_blocks[i].get_metadata_crc();
        auto crc_r = metaBlockRd.get_metadata_crc();
        EXPECT_EQ(crc_w, crc_r);
    }


    auto metaBlocksRd = metadataRdFile.readMultipleMetadataBlocks(0, metadata_blocks.size());
    for (int i = 0; i < metadata_blocks.size(); i++) {
        auto crc_w = metadata_blocks[i].get_metadata_crc();
        auto crc_r = metaBlocksRd[i].get_metadata_crc();
        EXPECT_EQ(crc_w, crc_r);
    }

}


TEST_F(DataFileTests, DataFileReadWriteHeader) {
    // Test serialization produces consistent results
    fs::path temp_file_path = temp_folder / "metadata_test_file_4.b";

    neuro21::FileHeader originalHdr(
            "FileTypeA",
            "0.0.1",
            "pt_X",
            "session_Y",
            "channel_Z",
            "gorilla",
            44100.0
    );

    neuro21::DataFile dataFileWrite(temp_file_path, originalHdr);

    neuro21::DataFile dataFileRead(temp_file_path);
    neuro21::FileHeader readHdr = dataFileRead.getHeader();

    EXPECT_EQ(readHdr == originalHdr, true);
}

TEST_F(DataFileTests, DataFileReadWriteDataBlock) {
    fs::path temp_file_data = temp_folder / "data_test_file.d";

    neuro21::DataFile dataWrite(temp_file_data, originalHdr);

    dataWrite.writeDataBlock(data_blocks[0]);
    dataWrite.writeDataBlock(data_blocks[1]);
    dataWrite.writeDataBlock(data_blocks[2]);

    neuro21::DataFile dataRead(temp_file_data);

    auto dataBlockRd1 = dataRead.readDataBlock(metadata_blocks[0]);
    auto dataBlockRd2 = dataRead.readDataBlock(metadata_blocks[1]);
    auto dataBlockRd3 = dataRead.readDataBlock(metadata_blocks[2]);

    auto crc_wc_1 = data_blocks[0].get_compressed_crc();
    auto crc_wc_2 = data_blocks[1].get_compressed_crc();
    auto crc_wc_3 = data_blocks[2].get_compressed_crc();

    auto crc_rc_1 = dataBlockRd1.get_compressed_crc();
    auto crc_rc_2 = dataBlockRd2.get_compressed_crc();
    auto crc_rc_3 = dataBlockRd3.get_compressed_crc();

    EXPECT_EQ(crc_wc_1, crc_rc_1);
    EXPECT_EQ(crc_wc_2, crc_rc_2);
    EXPECT_EQ(crc_wc_3, crc_rc_3);

    auto crc_wd_1 = data_blocks[0].calculate_decompressed_crc();
    auto crc_wd_2 = data_blocks[1].calculate_decompressed_crc();
    auto crc_wd_3 = data_blocks[2].calculate_decompressed_crc();

    auto crc_rd_1 = dataBlockRd1.calculate_decompressed_crc();
    auto crc_rd_2 = dataBlockRd2.calculate_decompressed_crc();
    auto crc_rd_3 = dataBlockRd3.calculate_decompressed_crc();

    EXPECT_EQ(crc_wd_1, crc_rd_1);
    EXPECT_EQ(crc_wd_2, crc_rd_2);
    EXPECT_EQ(crc_wd_3, crc_rd_3);
}

TEST_F(DataFileTests, DataFileMultipleReadWriteDataBlock) {
    fs::path temp_file_data = temp_folder / "data_test_file.d";

    neuro21::DataFile dataWrite(temp_file_data, originalHdr);
    dataWrite.writeMultipleDataBlocks(data_blocks);

    neuro21::DataFile dataRead(temp_file_data);
    for (int i = 0; i < data_blocks.size(); i++) {
        auto dataBlockRd = dataRead.readDataBlock(metadata_blocks[i]);

        auto crc_rc = dataBlockRd.get_compressed_crc();
        auto crc_wc = data_blocks[i].get_compressed_crc();
        EXPECT_EQ(crc_wc, crc_rc);

        auto crc_rd = dataBlockRd.calculate_decompressed_crc();
        auto crc_wd = data_blocks[i].calculate_decompressed_crc();
        EXPECT_EQ(crc_wd, crc_rd);
    }

    auto dataBlocksRd = dataRead.readMultipleDataBlocks(metadata_blocks);
    for (int i = 0; i < data_blocks.size(); i++) {
        auto crc_rc = dataBlocksRd[i].get_compressed_crc();
        auto crc_wc = data_blocks[i].get_compressed_crc();
        EXPECT_EQ(crc_wc, crc_rc);

        auto crc_rd = dataBlocksRd[i].calculate_decompressed_crc();
        auto crc_wd = data_blocks[i].calculate_decompressed_crc();
        EXPECT_EQ(crc_wd, crc_rd);
    }
}


TEST_F(DataFileTests, EndtoEnd) {
    fs::path temp_file_path_meta = temp_folder / "metadata_test_file_3.n21_m";
    fs::path temp_file_path_data = temp_folder / "metadata_test_file_3.n21_d";

    neuro21::MetadataFile metadataFile(temp_file_path_meta, originalHdr);
    metadataFile.writeMultipleMetadataBlocks(metadata_blocks);

    neuro21::DataFile dataFile(temp_file_path_data, originalHdr);
    dataFile.writeMultipleDataBlocks(data_blocks);

    neuro21::MetadataFile metadataRdFile(temp_file_path_meta);
    neuro21::DataFile dataRdFile(temp_file_path_data);

    auto metadata = metadataRdFile.readMultipleMetadataBlocks(0, metadata_blocks.size());
    auto data = dataRdFile.readMultipleDataBlocks(metadata);

    for (int i = 0; i < data_blocks.size(); i++) {
        EXPECT_EQ(data_blocks[i].get_data() == data[i].get_data(), true);
    }
}






