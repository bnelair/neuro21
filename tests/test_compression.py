import numpy as np
import pandas as pd
from numpy import dtype

# from pyneuro21.cpp import compress, decompress
from pyneuro21.compression import compress_gorilla, decompress_gorilla


def test_gorilla_compression():
    data_f64 = np.around(np.cumsum(np.random.randn(60 * 5000))).astype(np.float64)
    data_f64 = np.round(data_f64*1000)

    compressed_f64 = compress_gorilla(data_f64)
    decompressed_f64 = decompress_gorilla(compressed_f64)
    compression_rate_f64 = compressed_f64.__len__()/data_f64.nbytes

    assert compression_rate_f64 < 1.0 # this is true bcs of cumsum data, not for white noise
    assert np.all(data_f64 == decompressed_f64) # compression v decompression -> lossless

    data_i64 = data_f64.astype(np.int64)
    compressed_i64 = compress_gorilla(data_i64)
    compression_rate_i64 = compressed_i64.__len__()/data_i64.nbytes
    decompressed_i64 = decompress_gorilla(compressed_i64)

    assert compression_rate_i64 < 1.0
    assert np.all(data_i64 == decompressed_i64)
    assert np.all(decompressed_f64 == decompressed_i64)
    assert np.isclose(compression_rate_i64, compression_rate_f64, 2)



