Python Spidev
=============

This project contains a python module for interfacing with SPI devices from user space via the spidev linux kernel driver.

This is a modified version of the code originally found [here](http://elk.informatik.fh-augsburg.de/da/da-49/trees/pyap7k/lang/py-spi)

All code is GPLv2 licensed unless explicitly stated otherwise.

Usage
-----

```python
import spidev
spi = spidev.SpiDev()
spi.open(bus, device)
to_send = [0x01, 0x02, 0x03]
spi.xfer(to_send)
```
Settings
--------

```python
import spidev
spi = spidev.SpiDev()
spi.open(bus, device)

# Settings (for example)
spi.max_speed_hz = 5000
spi.mode = 0b01

...
```

* `bits_per_word`
* `cshigh`
* `loop` - Set the "SPI_LOOP" flag to enable loopback mode
* `no_cs` - Set the "SPI_NO_CS" flag to disable use of the chip select (although the driver may still own the CS pin)
* `lsbfirst`
* `max_speed_hz`
* `mode` - SPI mode as two bit pattern of clock polarity and phase [CPOL|CPHA], min: 0b00 = 0, max: 0b11 = 3
* `threewire` - SI/SO signals shared

Methods
-------

    open(bus, device)

Connects to the specified SPI device, opening `/dev/spidev<bus>.<device>`

    readbytes(n)

Read n bytes from SPI device.

    writebytes(list of values)

Writes a list of values to SPI device.

    xfer(list of values[, speed_hz, delay_usec, bits_per_word])

Performs an SPI transaction. Chip-select should be released and reactivated between blocks.
Delay specifies the delay in usec between blocks.

    xfer2(list of values[, speed_hz, delay_usec, bits_per_word])

Performs an SPI transaction. Chip-select should be held active between blocks.

    close()

Disconnects from the SPI device.
