class BitWriter:
    def __init__(self):
        self.buffer = bytearray()
        self.bit_buffer = 0  # bits are stored in the higher bits
        self.bit_count = 0   # number of bits in bit_buffer

    def write_bit(self, bit):
        if bit not in (0, 1):
            raise ValueError("Bit must be 0 or 1")
        # Add the bit to the bit_buffer
        self.bit_buffer = (self.bit_buffer << 1) | bit
        self.bit_count += 1
        if self.bit_count == 8:
            # Buffer is full, append to buffer
            self.buffer.append(self.bit_buffer)
            self.bit_buffer = 0
            self.bit_count = 0

    def write_bits(self, value, n_bits):
        if n_bits < 0:
            raise ValueError("Number of bits must be non-negative")
        for i in range(n_bits - 1, -1, -1):
            bit = (value >> i) & 1
            self.write_bit(bit)

    def get_bytes(self):
        # Flush remaining bits
        if self.bit_count > 0:
            self.bit_buffer <<= (8 - self.bit_count)
            self.buffer.append(self.bit_buffer)
            self.bit_buffer = 0
            self.bit_count = 0
        return bytes(self.buffer)

class BitReader:
    def __init__(self, data):
        self.buffer = data
        self.byte_index = 0
        self.bit_buffer = 0
        self.bit_count = 0

    def read_bit(self):
        if self.bit_count == 0:
            if self.byte_index >= len(self.buffer):
                raise EOFError("No more data")
            self.bit_buffer = self.buffer[self.byte_index]
            self.byte_index += 1
            self.bit_count = 8
        self.bit_count -= 1
        bit = (self.bit_buffer >> self.bit_count) & 1
        return bit

    def read_bits(self, n_bits):
        value = 0
        for _ in range(n_bits):
            value = (value << 1) | self.read_bit()
        return value

def count_leading_zero_bits(x):
    n = 0
    for i in range(63, -1, -1):
        if (x >> i) & 1 == 0:
            n += 1
        else:
            break
    return n

def count_trailing_zero_bits(x):
    n = 0
    for i in range(0, 64):
        if (x >> i) & 1 == 0:
            n += 1
        else:
            break
    return n