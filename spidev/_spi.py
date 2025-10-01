from __future__ import annotations

from contextlib import suppress
from typing import TYPE_CHECKING, Any, Callable, Self, TypeVar, overload
from warnings import deprecated

from . import _spimodule

if TYPE_CHECKING:
    from collections.abc import Buffer, Sequence
    from os import PathLike
    from types import TracebackType

    StrPath = str | PathLike[str]
    T = TypeVar("T")


def try_convert(val: object, typename: Callable[[Any], T], varname: str = "Value") -> T:
    """Try to convert `val` to `typename`. Raise a TypeError if conversion fails."""
    try:
        return typename(val)
    except (TypeError, ValueError) as err:
        msg = f"{varname} must have type {typename}, but is {type(val)}"
        raise TypeError(msg) from err


class SpiDev:
    """Connect to a SPI device.

    Examples:
        >>> SpiDev(0, 1) # connect to /dev/spidev0.1

        >>> SpiDev(path='/dev/myspi') # connect to /dev/myspi

    """

    def __init__(  # noqa: PLR0913
        self,
        bus: int | None = None,
        device: int | None = None,
        *,
        path: StrPath | None = None,
        mode: int | None = None,
        bits_per_word: int | None = None,
        max_speed_hz: int | None = None,
        read0: bool | None = None,
    ) -> None:
        self._cmod = _spimodule.SpiDev(bus, device)

        self.bus = bus
        self.device = device
        if path and (bus or device):
            raise ValueError(
                "both path and bus/device number of SPI device are provided",
            )
        self.path = path

        if mode is not None:
            self.mode = mode
        if bits_per_word is not None:
            self.bits_per_word = bits_per_word
        if max_speed_hz is not None:
            self.max_speed_hz = max_speed_hz
        if read0 is not None:
            self.read0 = read0

        # TODO: open() here? It's what the original implementation did
        if (bus and device) or path:
            self.open()

    def _resolve_path(self) -> str:
        """Construct path from bus and device numbers or from given path.

        Returns:
            str: Resolved path to the SPI device.

        Raises:
            ValueError: if bus+device or path not set

        """
        if self.bus is not None and self.device is not None:
            return f"/dev/spidev{self.bus:d}.{self.device:d}"
        if self.path is not None:
            return str(self.path)
        raise ValueError("bus/device or path not set")

    @property
    def closed(self) -> bool:
        """Return True if the connection is not opened."""
        try:
            self.fileno()
            return True
        except ValueError:
            return False

    @property
    def mode(self) -> int:
        """SPI mode.

        A two bit pattern of clock polarity and phase [CPOL|CPHA],
        min: 0b00 = 0, max: 0b11 = 3
        """
        return self._cmod.mode

    @mode.setter
    def mode(self, value: int, /) -> None:
        v = try_convert(value, int, "mode")

        # two bits max, thus value can only be 0-3
        if v not in range(4):
            msg = f"mode {v} has more than two bits"
            raise ValueError(msg)

        self._cmod.mode = v

    @property
    def bits_per_word(self) -> int:
        """Bits per word used in the xfer methods."""
        return self._cmod.bits_per_word

    @bits_per_word.setter
    def bits_per_word(self, value: int, /) -> None:
        v = try_convert(value, int, "bits_per_word")

        if v not in (8, 16, 32):
            msg = f"bits_per_word must be 8, 16 or 32, is {v}"
            raise ValueError(msg)

        self._cmod.bits_per_word = v

    @property
    def max_speed_hz(self) -> int:
        """Max speed (in Hertz)."""
        return self._cmod.max_speed_hz

    @max_speed_hz.setter
    def max_speed_hz(self, value: int, /) -> None:
        self._cmod.max_speed_hz = try_convert(value, int, "max_speed_hz")

    @property
    def read0(self) -> bool:
        """Read 0 bytes after transfer to lower CS if cshigh is set."""
        return self._cmod.read0

    @read0.setter
    def read0(self, value: bool, /) -> None:
        self._cmod.read0 = try_convert(value, bool, "read0")

    @property
    def cshigh(self) -> bool:
        return self._cmod.cshigh

    @cshigh.setter
    def cshigh(self, value: bool, /) -> None:
        self._cmod.cshigh = try_convert(value, bool, "cshigh")

    @property
    def threewire(self) -> bool:
        """SI/SO signals shared."""
        return self._cmod.threewire

    @threewire.setter
    def threewire(self, value: bool, /) -> None:
        self._cmod.threewire = try_convert(value, bool, "threewire")

    @property
    def lsbfirst(self) -> bool:
        return self._cmod.lsbfirst

    @lsbfirst.setter
    def lsbfirst(self, value: bool, /) -> None:
        self._cmod.lsbfirst = try_convert(value, bool, "lsbfirst")

    @property
    def loop(self) -> bool:
        """Sets the SPI_LOOP flag to enable loopback mode."""
        return self._cmod.loop

    @loop.setter
    def loop(self, value: bool, /) -> None:
        self._cmod.loop = try_convert(value, bool, "loop")

    @property
    def no_cs(self) -> bool:
        """Sets the SPI_NO_CS flag to disable use of the chip select."""
        return self._cmod.no_cs

    @no_cs.setter
    def no_cs(self, value: bool, /) -> None:
        self._cmod.no_cs = try_convert(value, bool, "no_cs")

    def close(self) -> None:
        """Close the object from the interface."""
        self._cmod.close()

    def fileno(self) -> int:
        """Return the file descriptor if it exists.

        Returns:
            int: File descriptor number.

        Raises:
            ValueError: if the connection is not open.

        """
        fd = self._cmod.fileno()
        if fd < 0:
            raise ValueError("I/O operation on closed file")
        return fd

    @overload
    def open(self) -> None: ...

    @overload
    @deprecated("use SpiDev(bus, device).open()")
    def open(self, bus: int, device: int) -> None: ...

    def open(self, bus: int | None = None, device: int | None = None) -> None:
        """Connect to the SPI device special file.

        If bus and device are provided it opens "/dev/spidev<bus.<device>". If
        path is provided it opens the SPI device at given path. Symbolic links
        are followed.

        Args:
            bus: Bus number.
            device: Device number.

        Raises:
            ValueError: If bus/device or path is not provided.

        """
        if bus:
            self.bus = bus
        if device:
            self.device = device

        # TODO: If Cmod's open() would return the fd we can lift a lot of logic
        # to here.
        self._cmod.open_path(self._resolve_path())

    @deprecated("use SpiDev(path='...').open()")
    def open_path(self, path: StrPath | None = None) -> None:
        """Open SPI device at given path.

        Args:
            path: Path to SPI device.

        Raises:
            IOError

        """
        if path:
            self.path = path
        self._cmod.open_path(self._resolve_path())

    def read(self, size: int = -1, /) -> list[int]:
        """Read and return up to _size_ bytes.

        If size is omitted or negative, 1 byte is read.

        Returns:
            list[int]: _size_ number of bytes.

        Raises:
            OSError: If device is closed.

        """
        if not self.readable():
            raise OSError("device closed")
        # TODO: IOBase.read(-1) means "read as much as
        # possible". Can we mimic this behavior?
        if size < 1:
            size = 1
        return self._cmod.readbytes(size)

    def readable(self) -> bool:
        """Return True if the SPI device is currently open."""
        return not self.closed

    @deprecated("use SpiDev().read()")
    def readbytes(self, length: int) -> list[int]:
        return self._cmod.readbytes(length)

    def writeable(self) -> bool:
        """Return True if the SPI connection is currently open."""
        return not self.closed

    def write(self, b: Sequence[int] | Buffer, /) -> None:
        """Write bytes to SPI device.

        Accepts arbitrary large lists. If list size exceeds buffer size (read
        from /sys/module/spidev/parameters/bufsiz), data will be
        split into smaller chunks and sent in multiple operations.

        Args:
            b: Sequence of bytes or Buffer to write.

        Raises:
            OSError: If device is closed.

        """
        if not self.writeable():
            raise OSError("device closed")
        # TODO: return number of bytes written like RawIOBase does
        self._cmod.writebytes2(b)

    @deprecated("use SpiDev().write()")
    def writebytes(self, values: Sequence[int]) -> None:
        self._cmod.writebytes(values)

    @deprecated("use SpiDev().write()")
    def writebytes2(self, values: Sequence[int] | Buffer) -> None:
        self._cmod.writebytes2(values)

    def xfer(
        self,
        values: Sequence[int],
        speed_hz: int | None = None,
        delay_usecs: int | None = None,
        bits_per_word: int | None = None,
    ) -> list[int]:
        """Performs an SPI transaction.

        NOTE: Chip-select should be released and reactivated between blocks.

        Args:
            values: Bytes to write.
            speed_hz: Speed to use.
            delay_usecs: Delay in microseconds between blocks.
            bits_per_word: Bits per word.

        Returns:
            TODO

        """
        return self._cmod.xfer(values, speed_hz, delay_usecs, bits_per_word)

    def xfer2(
        self,
        values: Sequence[int],
        speed_hz: int | None = None,
        delay_usecs: int | None = None,
        bits_per_word: int | None = None,
    ) -> list[int]:
        """Performs an SPI transaction.

        NOTE: Chip-select should be held active between blocks.

        Args:
            values: Bytes to write.
            speed_hz: Speed to use.
            delay_usecs: Delay in microseconds between blocks.
            bits_per_word: Bits per word.

        Returns:
            TODO

        """
        return self._cmod.xfer2(values, speed_hz, delay_usecs, bits_per_word)

    def xfer3(
        self,
        values: Sequence[int],
        speed_hz: int | None = None,
        delay_usecs: int | None = None,
        bits_per_word: int | None = None,
    ) -> tuple[int, ...]:
        """Performs an SPI transaction.

        Accepts arbitrary large lists. If list size exceeds buffer size (read
        from /sys/module/spidev/parameters/bufsiz), data will be split
        into smaller chunks and sent in multiple operations.

        Args:
            values: Bytes to write.
            speed_hz: Speed to use.
            delay_usecs: Delay in microseconds between blocks.
            bits_per_word: Bits per word.

        Returns:
            TODO

        """
        return self._cmod.xfer3(values, speed_hz, delay_usecs, bits_per_word)

    def __enter__(self) -> Self:
        # TODO: make open() idempotent and raise IOError if opening fails
        with suppress(ValueError):
            self.open()

        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self._cmod.close()

    def __str__(self) -> str:
        if self.bus is not None and self.device is not None:
            # SpiDev(0, 1)
            return f"{self.__class__.__name__}({self.bus}, {self.device})"
        if self.path is not None:
            # SpiDev('/dev/myspi')
            return f"{self.__class__.__name__}({self.path!r})"

        # SpiDev()
        return f"{self.__class__.__name__}()"

    def __repr__(self) -> str:
        # e.g. SpiDev(bus=0, device=1, bits_per_word=8)
        args = ", ".join(
            f"{a}={getattr(self, a)!r}"
            for a in (
                "bus",
                "device",
                "path",
                "mode",
                "bits_per_word",
                "max_speed_hz",
                "read0",
            )
            if getattr(self, a) is not None
        )
        return f"{self.__class__.__name__}({args})"

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, type(self)):
            return False

        try:
            return self._resolve_path() == other._resolve_path()
        except ValueError:
            # return False if one of the instances is uninitiated
            return False

    def __del__(self) -> None:
        del self._cmod
