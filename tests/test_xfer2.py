import pytest

def test_xfer2_loopback(spidev):
    result = spidev.xfer2([n for n in range(255)])
    assert result == [n for n in range(255)]

def test_xfer2_empty_list_raises_type_error(spidev):
    data = []
    with pytest.raises(TypeError):
        spidev.xfer2(data)

def test_xfer2_oversized_list_raises_overflow_error(spidev):
    data = [0 for _ in range(4097)]
    with pytest.raises(OverflowError):
        spidev.xfer2(data)

def test_xfer2_invalid_list_entry_raises_type_error(spidev):
    data = ["1"]
    with pytest.raises(TypeError):
        spidev.xfer2(data)
    try:
        data = [long(1)]
        with pytest.raises(TypeError):
            spidev.xfer2(data)
    except NameError:
        pass

