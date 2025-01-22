#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <random>
#include <gtest/gtest.h>
#include "../src/gorilla_compression.cpp"
#include "../src/utils.cpp"


TEST(GorillaCompressionTest, CompressionDecompressionTest) {
    size_t sample_count = 1000;
    std::vector<double_t> samples = neuro21::Generate::generate_cumulative_random_data(sample_count);

    // Compress the samples
    neuro21::GorillaCompressor compressor;
    std::vector<uint8_t> compressed_data = compressor.compress(samples);

    // Decompress the samples
    neuro21::GorillaDecompressor decompressor;
    std::vector<double_t> decompressed_samples = decompressor.decompress(compressed_data);

    // Assert that decompressed data matches original
    double tolerance = 1e-9;
    for (size_t i = 0; i < samples.size(); ++i) {
    ASSERT_NEAR(samples[i], decompressed_samples[i], tolerance)
    << "Mismatch at index " << i;
    }
}




