import sys
import pytest

@pytest.fixture
def spidev():
    import spidev
    dev = spidev.SpiDev(0, 0)
    # Slow down SPI bus to ensure a stable result for loopback testing
    dev.max_speed_hz = 100000
    yield dev
    del sys.modules["spidev"]

