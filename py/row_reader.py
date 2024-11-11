import struct
from io import BytesIO

import arrow

COLUMNS = ["timestamp"]
PADDING_LENGTH_BYTES = 4 + 4 + 2
TIMESTAMP_LENGTH_BYTES = 4
SHUNT_LENGTH_BYTES = 4 + 4
SHUNT_COUNT = 5
CHECKSUM_LENGTH = 8
NEWLINE_LENGTH = 2
SNAPSHOT_LENGTH = TIMESTAMP_LENGTH_BYTES + (SHUNT_LENGTH_BYTES * SHUNT_COUNT) + CHECKSUM_LENGTH + NEWLINE_LENGTH

# 2.5 uV per LSB
SHUNT_VOLTS_PER_LSB = 0.0000025

# 1.25 mV per LSB
BUS_VOLTS_PER_LSB = 0.00125

SHUNT_OHMS = 0.0001


SNAPSHOT_LENGTH += PADDING_LENGTH_BYTES * 12


class RowReader:
    def __init__(self, f):
        self.f = f
        self.checksum = 0

    def read(self, bio: BytesIO, length: int):
        z = struct.unpack("<l", bio.read(4))[0]
        if z != 0:
            raise Exception(f"Expected 0, got {z}")
        l = struct.unpack("<h", bio.read(2))[0]
        if l != length:
            raise Exception(f"Expected {length}, got {l}")
        b = bio.read(length)
        z = struct.unpack("<l", bio.read(4))[0]
        if z != 0:
            raise Exception(f"Expected 0, got {z}")
        return b

    def read_int64(self, bio: BytesIO):
        i = struct.unpack("<q", self.read(bio, 8))[0]
        self.checksum += i
        return i

    def read_checksum64(self, bio: BytesIO):
        i = struct.unpack("<q", self.read(bio, 8))[0]
        return i

    def read_int32(self, bio: BytesIO):
        i = struct.unpack("<l", self.read(bio, 4))[0]
        self.checksum += i
        return i

    def read_uint32(self, bio: BytesIO):
        i = struct.unpack("<L", self.read(bio, 4))[0]
        self.checksum += i
        return i

    def read_int16(self, bio: BytesIO):
        i = struct.unpack("<h", self.read(bio, 2))[0]
        self.checksum += i
        return i

    def read_char(self, bio: BytesIO):
        c = struct.unpack("<c", self.read(bio, 1))[0]
        self.checksum += c
        return c.decode("utf-8")

    def rows(self):
        snapshot_data = self.f.read(SNAPSHOT_LENGTH)
        while snapshot_data:
            bio = BytesIO(snapshot_data)
            row = {}
            self.checksum = 0
            unix_timestamp = self.read_int32(bio)
            row["timestamp"] = arrow.get(unix_timestamp).format("YYYY-MM-DD HH:mm:ss")
            for deviceId in range(1, 6):  # Read bus and shunt voltages for each of 5 devices
                bus_raw_voltage = self.read_uint32(bio)
                shunt_raw_voltage = self.read_int32(bio)
                shunt_volts = SHUNT_VOLTS_PER_LSB * shunt_raw_voltage
                bus_volts = BUS_VOLTS_PER_LSB * bus_raw_voltage
                bus_amps = shunt_volts / SHUNT_OHMS
                bus_watts = bus_amps * bus_volts
                row[f"bus_voltage_{deviceId}"] = bus_volts
                row[f"shunt_voltage_{deviceId}"] = shunt_volts
                row[f"current_{deviceId}"] = bus_amps
                row[f"power_{deviceId}"] = bus_watts
            checksum = self.read_checksum64(bio)
            if checksum != self.checksum:
                raise Exception(f"Checksum mismatch: {checksum} != {self.checksum}")
            yield row
            new_line_chars = bio.read(2)
            if new_line_chars != b"\r\n":
                raise Exception(f"Expected new line, got {new_line_chars}")
            excess = bio.read()
            if len(excess) > 0:
                raise Exception(f"Excess data: {excess}")
            snapshot_data = self.f.read(SNAPSHOT_LENGTH)
