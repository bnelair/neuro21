
import numpy as np
from pyneuro21.cpp import compress
from zlib import crc32
import pathlib








class Neuro21Writer():
    def __init__(self, path):
        self.directory = path
        self.file = open(path)

        if ".n21_ses" not in self.directory:
            self.directory += ".n21_ses"

        pathlib.Path(self.directory).mkdir(parents=True, exist_ok=False)

    def append_segment(self, channel, uutc_start, fsamp, data):
        if data.ndim != 1:
            raise ValueError("Data must be a 1D array")

        if data.dtype != np.float64:
            data = data.astype(np.float64)

        crc = crc32(data)

        bytearray = compress(data)

        with open(self.directory + f"/{channel}.n21_idx", "ab") as fid_idx, \
                open(self.directory + f"/{channel}.n21_dat", "ab") as fid_dat:

            uutc_start = np.array(uutc_start, dtype=np.uint64)
            fid_idx.write(uutc_start.tobytes())

            uutc_end = np.array(uutc_start + 1000000 * (len(data) - 1) / fsamp, dtype=np.uint64)
            fid_idx.write(uutc_end.tobytes())

            dat_idx_block_start = np.array(fid_dat.tell(), dtype=np.uint64)
            fid_idx.write(dat_idx_block_start.tobytes())

            dat_idx_block_end = np.array(len(bytearray), dtype=np.uint64)
            fid_idx.write(dat_idx_block_end.tobytes())

            block_crc = np.array(crc, dtype=np.uint64)
            fid_idx.write(block_crc.tobytes())

            fsamp = np.array(fsamp, dtype=np.float64)
            fid_idx.write(fsamp.tobytes())

            fid_dat.write(bytearray)

    def append_channel(self, channel, uutc_start, fsamp, data):
        if data.ndim != 1:
            raise ValueError("Data must be a 1D array")

        if data.dtype != np.float64:
            data = data.astype(np.float64)

        for i in range(0, data.shape[0], int(fsamp)):
            self.append_segment(channel=channel,
                                uutc_start=int(uutc_start + 1000000 * i / fsamp),
                                fsamp=fsamp,
                                data=data[i:i + int(fsamp)])