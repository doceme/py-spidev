Python Spidev
=============

This project contains a python module for interfacing with SPI devices from user space via the spidev linux kernel driver.

All code is MIT licensed unless explicitly stated otherwise.

Usage
-----

```python
import spidev
spi = spidev.SpiDev()
spi.open_path(spidev_devicefile_path)
to_send = [0x01, 0x02, 0x03]
spi.xfer(to_send)
```
Settings
--------

```python
import spidev
spi = spidev.SpiDev()
spi.open_path("/dev/spidev0.0")

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
* `read0` - Read 0 bytes after transfer to lower CS if cshigh == True

Methods
-------

    open_path(filesystem_path)

Connects to the specified SPI device special file, following symbolic links if
appropriate (see note on deterministic SPI bus numbering in the Linux kernel
below for why this can be advantageous in some configurations).

    open(bus, device)

Equivalent to calling `open_path("/dev/spidev<bus>.<device>")`. n.b. **Either**
`open_path` or `open` should be used.

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
data will be split into smaller 4096 byte chunks and sent in multiple operations.

    close()

Disconnects from the SPI device.

The Linux kernel and SPI bus numbering and the role of udev
-----------------------------------------------------------

### Summary

If your code may interact with an SPI controller which is attached to the
system via the USB or PCI buses, **or** if you are maintaining a product which
is likely to change SoCs **or upgrade kernels** during its lifetime, then you
should consider using one or more udev rules to create symlinks to the SPI
controller spidev, and then use `open_path`, to open the device file via the
symlink in your code.

Consider allowing the end-user to configure their choice of full spidev path -
for example with the use of a command line argument to your Python script, or
an entry in a configuration file which your code reads and parses.

Additional udev actions can also set the ownership and file access permissions
on the spidev device node file (to increase the security of the system).  In
some instances, udev rules may also be needed to ensure that spidev device
nodes are created in the first place (by triggering the Linux `spidev` driver
to "bind" to an underlying SPI controller).

### Detailed Information

This section provides an overview of the Linux APIs which this extension uses.

**If your software might be used on systems with non-deterministic SPI bus
numbering**, then using the `open_path` method can allow those maintaining the
system to use mechanisms such as `udev` to create stable symbolic links to the
SPI device for the correct physical SPI bus.

See the example udev rule file `99-local-spi-example-udev.rules`.

This Python extension communicates with SPI devices by using the 'spidev'
[Linux kernel SPI userspace
API](https://www.kernel.org/doc/html/next/spi/spidev.html).

'spidev' in turn communicates with SPI bus controller hardware using the
kernel's internal SPI APIs and hardware device drivers.

If the system is configured to expose a particular SPI device to user space
(i.e. when an SPI device is "bound" to the spidev driver), then the spidev
driver registers this device with the kernel, and exposes its Linux kernel SPI
bus number and SPI chip select number to user space in the form of a POSIX
"character device" special file.

A user space program (usually 'udev') listens for kernel device creation
events, and creates a file system "device node" for user space software to
interact with.  By convention, for spidev, the device nodes are named
/dev/spidev<bus>.<device> is (where the *bus* is the Linux kernel's internal
SPI bus number (see below) and the *device* number corresponds to the SPI
controller "chip select" output pin that is connected to the SPI *device* 'chip
select' input pin.

The Linux kernel **may assign SPI bus numbers to a system's SPI controllers in
a non-deterministic way.** In some hardware configurations, the SPI bus number
of a particular hardware peripheral is:

- Not guaranteed to remain constant between different Linux kernel versions.
- Not guaranteed to remain constant between successive boots of the same kernel
  (due to race conditions during boot-time hardware enumeration, or dynamic
  kernel module loading).
- Not guaranteed to match the hardware manufacturer's SPI bus numbering scheme.

In the case of SPI controllers which are themselves connected to the system via
buses that are subject to hot-plug (such as USB, Thunderbolt, or PCI), the
SPI bus number should usually be expected to be non-deterministic.

The supported Linux mechanism which allows user space software to identify the
correct hardware, it to compose "udev rules" which create stable symbolic links
to device files. For example, most Linux distributions automatically create
symbolic links to allow identification of block storage devices e.g. see the
output of `ls -alR /dev/disk`.

`99-local-spi-example-udev.rules` included with py-spidev includes example udev
rules for creating stable symlink device paths (for use with `open_path`).

e.g. the following Python code could be used to communicate with an SPI device
attached to chip-select line 0 of an individual FTDI FT232H USB to SPI adapter
which has the USB serial number "1A8FG636":


```
#!/usr/bin/env python3
import spidev

spi = spidev.SpiDev()
spi.open_path("/dev/spi/by-path/usb-sernum-1A8FG636-cs-0")
# TODO: Useful stuff here
spi.close()

```

In the more general case, the example udev file should be modified as
appropriate to your needs, renamed to something descriptive of the purpose
and/or project, and placed in `/etc/udev/rules.d/` (or `/lib/udev/rules.d/` in
the case of rules files included with operating system packages).
