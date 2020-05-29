import pytest

def test_xfer3_loopback(spidev):
    data = [n for n in range(255)]
    result = spidev.xfer3(data)
    assert len(result) == len(data)
    assert list(result) == data

def test_xfer3_empty_list_raises_type_error(spidev):
    data = []
    with pytest.raises(TypeError):
        spidev.xfer3(data)

def test_xfer3_invalid_list_entry_raises_type_error(spidev):
    data = ["1"]
    with pytest.raises(TypeError):
        spidev.xfer3(data)
    try:
        data = [long(1)]
        with pytest.raises(TypeError):
            spidev.xfer3(data)
    except NameError:
        pass

