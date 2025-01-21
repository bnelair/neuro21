import numpy as np
from pyneuro21 import compress, decompress
import glob
import tqdm
from zlib import crc32
import pathlib

class GorillaGenerator:
    def __init__(self, directory):
        self.directory = directory

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

        with open(self.directory + f"/{channel}.n21_idx", "ab") as file_n21_idx, \
                open(self.directory + f"/{channel}.n21_dat", "ab") as file_n21_dat:

            file_n21_idx.write(np.array(uutc_start, dtype=np.uint64).tobytes())
            file_n21_idx.write(np.array(uutc_start + 1000000 * (len(data) - 1) / fsamp, dtype=np.uint64).tobytes())
            file_n21_idx.write(np.array(file_n21_dat.tell(), dtype=np.uint64).tobytes())
            file_n21_idx.write(np.array(len(bytearray), dtype=np.uint64).tobytes())
            file_n21_idx.write(np.array(crc, dtype=np.uint64).tobytes())
            file_n21_idx.write(np.array(fsamp, dtype=np.float64).tobytes())

            file_n21_dat.write(bytearray)

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


class GorillaSession:
    def __init__(self, directory):
        self.directory = directory
        self._channels = sorted(glob.glob(self.directory + "/*.n21_idx"))
        self._channels = [pathlib.Path(p).stem for p in self._channels]

        self._fsamp = None
        self._uutc_start = None
        self._uutc_stop = None

        hdr = read_n21_idx(self.directory + f"/{self.channels[0]}.n21_idx")
        self._fsamp = hdr[0]['fsamp']
        self._uutc_start = hdr[0]['uuct_left']
        self._uutc_stop = hdr[-1]['uuct_stop']

        self._segments = {}

    @property
    def channels(self):
        return self._channels

    @property
    def fsamp(self):
        return self._fsamp

    @property
    def uutc_start(self):
        return self._uutc_start

    @property
    def uutc_stop(self):
        return self._uutc_stop

    def read_channel(self, channel, uutc_start=None, uutc_stop=None):
        if channel not in self.channels:
            raise ValueError(f"Channel {channel} not found")

        if uutc_start is None:
            uutc_start = self.uutc_start
        if uutc_stop is None:
            uutc_stop = self.uutc_stop

        if channel not in self._segments:
            self._segments[channel] = read_n21_idx(self.directory + f"/{channel}.n21_idx")

        data = []
        time = []
        for segment in tqdm.tqdm(self._segments[channel]):
            if segment['uuct_left'] >= uutc_stop or segment['uuct_stop'] <= uutc_start:
                continue
            tmp = read_n21_dat(self.directory + f"/{channel}.n21_dat", segment)
            data.append(tmp)
            time.append(np.linspace(segment['uuct_left'], segment['uuct_stop'], len(tmp)))
        data = np.concatenate(data)
        time = np.concatenate(time)
        data = data[(time >= uutc_start) & (time <= uutc_stop)]
        return data

    def find_discontinuity(self, channel, uutc_start=None, uutc_stop=None):
        if channel not in self.channels:
            raise ValueError(f"Channel {channel} not found")

        segments = read_n21_idx(self.directory + f"/{channel}.n21_idx")
        if not segments:
            return []

        discontinuities = []
        for i in range(1, len(segments)):
            prev_segment = segments[i - 1]
            current_segment = segments[i]
            if int(current_segment['uuct_left'] / 1000000) - int(prev_segment['uuct_stop'] / 1000000) > 1:
                discontinuities.append((i,
                                        prev_segment['uuct_stop'],
                                        current_segment['uuct_left'],
                                        (current_segment['uuct_left'] / 1000000 - prev_segment['uuct_stop'] / 1000000)))

        return discontinuities

    def read_ts_channels_uutc(self, channel_map, uutc_map):
        output = []
        for channel in channel_map:
            output.append(self.read_channel(channel, uutc_map[0], uutc_map[1]))
        return output

    def read_ts_channel_basic_info(self):
        output = []
        for channel in self.channels:
            output.append({'name': channel,
                           'fsamp': self.fsamp,
                           'uutc_start': self.uutc_start,
                           'uutc_stop': self.uutc_sto,
                           'timezone': 'no_tz'})
        return output


def read_n21_idx(file_path):
    # Each record is 48 bytes: 5 uint64 fields (8 bytes each) + 1 float64 field (8 bytes)
    record_size = 48

    with open(file_path, "rb") as file_n21_idx:
        # Read the entire file into memory
        file_data = file_n21_idx.read()

    # Ensure the file size is a multiple of record_size
    if len(file_data) % record_size != 0:
        raise ValueError("File size is not a multiple of record size.")

    # Calculate the number of records
    num_records = len(file_data) // record_size

    # Use numpy to interpret the entire binary data as a 1D array of uint64
    all_uint64 = np.frombuffer(file_data, dtype=np.uint64)

    # Extract fields
    uuct_left = all_uint64[0::6]  # Every 6th value starting from 0
    uuct_stop = all_uint64[1::6]  # Every 6th value starting from 1
    file_n21_dat_position = all_uint64[2::6]  # Every 6th value starting from 2
    bytearray_length = all_uint64[3::6]  # Every 6th value starting from 3
    crc = all_uint64[4::6]  # Every 6th value starting from 4

    # Extract the last field (float64) separately
    fsamp = np.frombuffer(file_data, dtype=np.float64)[5::6]  # Every 6th value starting from 5

    # Combine the fields into a list of dictionaries
    segments = [
        {
            "uuct_left": uuct_left[i],
            "uuct_stop": uuct_stop[i],
            "byte_offset": file_n21_dat_position[i],
            "byte_length": bytearray_length[i],
            "fsamp": fsamp[i],
            "crc": crc[i],
        }
        for i in range(num_records)
    ]

    return segments


def read_n21_dat(file_path, segment):
    with open(file_path, "rb") as file_n21_dat:
        file_n21_dat.seek(segment['byte_offset'])
        data = file_n21_dat.read(segment['byte_length'])
        data = decompress(data)
        crc = crc32(data)
        if crc != segment['crc']:
            print(f"CRC mismatch for segment {segment}")
        return data



