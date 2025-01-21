
from dataclasses import dataclass
from typing import Final
import struct

# Constants
HEADER_BYTES_INDEX_FILE: Final[int] = 1024  # Total header size
FILE_TYPE_STR_LENGTH: Final[int] = 4
FILE_VERSION_STR_LENGTH: Final[int] = 4
COMPRESSION_STR_LENGTH: Final[int] = 10

@dataclass
class HeaderIdxFile:
    file_type: str  # 4-byte string
    file_version: str  # 4-byte string
    fsamp: float  # float64
    n_blocks: int  # int32
    compression: str  # 10-byte string

    # Enforce big-endian format
    STRUCT_FORMAT: Final[str] = (
        f">"  # Big-endian prefix
        f"{FILE_TYPE_STR_LENGTH}s"  # 4-byte string for file_type
        f"{FILE_VERSION_STR_LENGTH}s"  # 4-byte string for file_version
        "d"  # float64 for fsamp
        "i"  # int32 for n_blocks
        f"{COMPRESSION_STR_LENGTH}s"  # 10-byte string for neuro21_cpp
    )
    STRUCT_SIZE: Final[int] = struct.calcsize(STRUCT_FORMAT)

    def to_bytes(self) -> bytes:
        """
        Serialize the header to bytes using big-endian format.
        """
        return struct.pack(
            self.STRUCT_FORMAT,
            self.file_type.encode("utf-8")[:FILE_TYPE_STR_LENGTH],
            self.file_version.encode("utf-8")[:FILE_VERSION_STR_LENGTH],
            self.fsamp,
            self.n_blocks,
            self.compression.encode("utf-8")[:COMPRESSION_STR_LENGTH]
        )

    @classmethod
    def from_bytes(cls, data: bytes):
        """
        Deserialize bytes to a HeaderIdxFile instance using big-endian format.
        """
        unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:cls.STRUCT_SIZE])
        return cls(
            file_type=unpacked[0].decode("utf-8").strip("\x00"),
            file_version=unpacked[1].decode("utf-8").strip("\x00"),
            fsamp=unpacked[2],
            n_blocks=unpacked[3],
            compression=unpacked[4].decode("utf-8").strip("\x00")
        )

    def write_to_file(self, path: str):
        """
        Write the header to a binary file.
        """
        with open(path, "wb") as file:
            file.write(self.to_bytes())

    @classmethod
    def from_file(cls, path: str):
        """
        Read the header from a binary file.
        """
        with open(path, "rb") as file:
            hdr_bytes = file.read(cls.STRUCT_SIZE)
        return cls.from_bytes(hdr_bytes)

    def update_header_in_file(self, path: str):
        """
        Update the header in an existing binary file.
        """
        with open(path, "r+b") as file:
            # Write the updated header at the start of the file
            file.seek(0)  # Move to the beginning
            file.write(self.to_bytes())


# Example Usage
header = HeaderIdxFile(
    file_type="IDX",
    file_version="1.0",
    fsamp=256.0,
    n_blocks=100,
    compression="gzip"
)

# Write to file
header.write_to_file("header.idx")

# Read from file
loaded_header = HeaderIdxFile.from_file("header.idx")
print("Before update:", loaded_header)

# Update the header
loaded_header.file_version = "1.1"
loaded_header.fsamp = 512.0
loaded_header.update_header_in_file("header.idx")

# Verify the update
updated_header = HeaderIdxFile.from_file("header.idx")
print("After update:", updated_header)



