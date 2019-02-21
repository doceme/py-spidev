Python Spidev
=============

This project contains a python module for interfacing with SPI devices from user space via the spidev linux kernel driver.

All code is MIT licensed unless explicitly stated otherwise.

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

    writebytes2(list of values)

Similar to `writebytes` but accepts arbitrary large lists.
If list size exceeds buffer size (which is read from `/sys/module/spidev/parameters/bufsiz`),
data will be split into smaller chunks and sent in multiple operations.

Also, `writebytes2` understands [buffer protocol](https://docs.python.org/3/c-api/buffer.html)
so it can accept numpy byte arrays for example without need to convert them with `tolist()` first.
This offers much better performance where you need to transfer frames to SPI-connected displays for instance.

    xfer(list of values[, speed_hz, delay_usec, bits_per_word])

Performs an SPI transaction. Chip-select should be released and reactivated between blocks.
Delay specifies the delay in usec between blocks.

    xfer2(list of values[, speed_hz, delay_usec, bits_per_word])

Performs an SPI transaction. Chip-select should be held active between blocks.

    xfer3(list of values[, speed_hz, delay_usec, bits_per_word])

Similar to `xfer2` but accepts arbitrary large lists.
If list size exceeds buffer size (which is read from `/sys/module/spidev/parameters/bufsiz`),
data will be split into smaller chunks and sent in multiple operations.

    close()

Disconnects from the SPI device.
