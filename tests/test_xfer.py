import pytest

def test_xfer_loopback(spidev):
    result = spidev.xfer([n for n in range(255)])
    assert result == [n for n in range(255)]

def test_xfer_empty_list_raises_type_error(spidev):
    data = []
    with pytest.raises(TypeError):
        spidev.xfer(data)

def test_xfer_oversized_list_raises_overflow_error(spidev):
    data = [0 for _ in range(4097)]
    with pytest.raises(OverflowError):
        spidev.xfer(data)

def test_xfer_invalid_list_entry_raises_type_error(spidev):
    data = ["1"]
    with pytest.raises(TypeError):
        spidev.xfer(data)
    try:
        data = [long(1)]
        with pytest.raises(TypeError):
            spidev.xfer(data)
    except NameError:
        pass

