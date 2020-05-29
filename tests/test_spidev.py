import pytest

def test_spidev_set_mode(spidev):
    spidev.mode = 0
    assert spidev.mode == 0
    spidev.mode = 1
    assert spidev.mode == 1

def test_spidev_bits_per_word_raises_typeerror(spidev):
    with pytest.raises(TypeError):
        spidev.bits_per_word = "1"
    try:
        bits = long(8)
        with pytest.raises(TypeError):
            spidev.bits_per_word = bits
    except NameError:
        pass

def test_spidev_mode_raises_typeerror(spidev):
    with pytest.raises(TypeError):
        spidev.mode = "1"
    try:
        mode = long(1)
        with pytest.raises(TypeError):
            spidev.mode = mode
    except NameError:
        pass

