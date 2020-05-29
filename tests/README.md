Python Spidev Tests
===================

These tests are intended to validate the behaviour of Spidev's functions and properties.

They have been run in Python 2.7 and 3 on a Raspberry Pi 4.

A short jumper wire is required between pins BCM9 (MISO) and BCM10 (MOSI) in order for loopback tests to pass, you must also enable SPI.

Tests depend upon pytest and can be run against spidev in a virtual environment with:

```
python setup.py develop
python -m py.test -v -r wsx
```
