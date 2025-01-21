#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "src/gorilla_compression.cpp"
#include "src/utils.cpp"

int main() {
    // Generate 1000 random samples
    size_t sample_count = 1000;

//    std::vector<double> samples = generate_random_samples(sample_count);
    std::vector<double> samples = neuro21::Generate::generate_cumulative_random_data(60*60*5000);


    // Compress the samples
    neuro21::GorillaCompressor compressor;
    std::vector<uint8_t> compressed_data = compressor.compress(samples);

    neuro21::GorillaDecompressor decompressor;
    std::vector<double> decompressed_samples = decompressor.decompress(compressed_data);

    // Output the size of compressed data
    std::cout << "Original size: " << samples.size() * sizeof(double) << " bytes" << std::endl;
    std::cout << "Compressed size: " << compressed_data.size() << " bytes" << std::endl;


    // Compare original and decompressed samples
    size_t mismatches = 0;
    double tolerance = 1e-9; // Allow for slight floating-point inaccuracies
    for (size_t i = 0; i < samples.size(); ++i) {
        if (std::fabs(samples[i] - decompressed_samples[i]) > tolerance) {
            mismatches++;
            if (mismatches <= 10) {
                std::cout << "Mismatch at index " << i << ": "
                          << "original=" << samples[i] << ", decompressed=" << decompressed_samples[i] << std::endl;
            }
        }
    }

    if (mismatches == 0) {
        std::cout << "All samples match!" << std::endl;
    } else {
        std::cout << "Total mismatches: " << mismatches << " out of " << samples.size() << std::endl;
    }

    return 0;
}





