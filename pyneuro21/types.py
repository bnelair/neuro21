from dataclasses import dataclass
import struct

from pyneuro21.cpp import write_file_header_cpp, read_file_header_cpp

@dataclass
class FileHeader:
    '''Corresponds to Headers in C++'''
    _number_of_bytes_str = 100
    _number_of_bytes_header = 2048  # Fixed typo

    file_type: str
    version: str
    patient_id: str
    session_id: str
    channel_id: str
    compression: str
    sampling_rate: float

    def to_bytes(self):
        """Converts the file header to a fixed-size byte representation (C++ compatible)."""
        # Encode fixed-size 100-byte strings, padding with null bytes (`b'\x00'`)
        header_bytes = (
            self.file_type.encode().ljust(self._number_of_bytes_str, b'\x00') +
            self.version.encode().ljust(self._number_of_bytes_str, b'\x00') +
            self.patient_id.encode().ljust(self._number_of_bytes_str, b'\x00') +
            self.session_id.encode().ljust(self._number_of_bytes_str, b'\x00') +
            self.channel_id.encode().ljust(self._number_of_bytes_str, b'\x00') +
            self.compression.encode().ljust(self._number_of_bytes_str, b'\x00')
        )

        # Pack the float as a little-endian 64-bit double (`<d`)
        header_bytes += struct.pack("<d", self.sampling_rate)

        # Add padding to reach 2048 bytes
        return header_bytes.ljust(self._number_of_bytes_header, b'\x00')

    @classmethod
    def from_bytes(cls, data):
        """Parses a FileHeader object from a bytes representation (C++ compatible)."""
        fields = [
            data[i * 100:(i + 1) * 100].decode().strip("\x00")
            for i in range(6)
        ]
        sampling_rate = struct.unpack("<d", data[600:608])[0]

        return cls(*fields, sampling_rate)

    def __eq__(self, other):
        """Compares two FileHeader objects."""
        if not isinstance(other, FileHeader):
            return False
        return (
            self.file_type == other.file_type and
            self.version == other.version and
            self.patient_id == other.patient_id and
            self.session_id == other.session_id and
            self.channel_id == other.channel_id and
            self.compression == other.compression and
            self.sampling_rate == other.sampling_rate
        )


def write_file_header(path: str, header: FileHeader):
    """Writes a file header to a file."""
    write_file_header_cpp(
        path,
        header.file_type,
        header.version,
        header.patient_id,
        header.session_id,
        header.channel_id,
        header.compression,
        header.sampling_rate
    )

def read_file_header(path: str) -> FileHeader:
    """Reads a file header from a file."""
    return FileHeader(*read_file_header_cpp(path))

