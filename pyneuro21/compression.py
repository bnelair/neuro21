
import numpy as np

from pyneuro21.cpp import gorilla_ccompress, gorilla_cdecompress


def compress_gorilla(data):
    """
    Uses Cpp code for gorilla compression. The data has to be a numpy ndarray with 1 dimension.

    :param data: np.ndarray(ndims=1, dtype=np.float64)
    :return: bytes
    """

    if data.ndim != 1:
        raise ValueError("Data must be a 1D array")

    if data.dtype != np.float64:
        data = data.astype(np.float64)

    return gorilla_ccompress(data)

def decompress_gorilla(data):
    """
    Uses Cpp code with 1 dimension.

    :param data: bytes
    :return: np.ndarray
    """

    return gorilla_cdecompress(data)


__all__ = [
    "compress_gorilla",
    "decompress_gorilla"
]