
import os
from distutils.file_util import write_file

import numpy as np
import pandas as pd

from pyneuro21.types import FileHeader, write_file_header, read_file_header


def test_header_write_read(temp_dir):
    path_hdr = os.path.join(temp_dir, "header.b")
    hdr = FileHeader(
        file_type="n21_ses",
        version="0.1",
        patient_id="123456",
        session_id="7890",
        channel_id="1",
        compression="none",
        sampling_rate=1000
    )

    write_file_header(path_hdr, hdr)
    hdr2 = read_file_header(path_hdr)
    assert hdr == hdr2


