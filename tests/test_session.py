
import numpy as np
import pytest


from pyneuro21 import GorillaGenerator, GorillaSession


def test_random():
    np.random.seed(0)
    data = {}
    data['ch_A'] = np.around(np.cumsum(np.random.randn(60 * 60 * 5000)))
    data['ch_B'] = np.around(np.cumsum(np.random.randn(60 * 60 * 5000)))

    path = "test/example.n21_ses"
    generator = GorillaGenerator(directory=path)
    for ch, data in data.items():
        generator.append_channel(channel=ch, uutc_start=0, fsamp=5000, data=data)

    session = GorillaSession(directory=path)
    for ch in session.channels:
        data = session.read_channel(channel=ch, uutc_start=session.uutc_start, uutc_stop=session.uutc_stop)


if __name__ == '__main__':
    pytest.main([__file__])
