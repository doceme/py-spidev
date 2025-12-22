from typing import TYPE_CHECKING
from contextlib import suppress

from spidev._spidev import _SpiDev

from collections.abc import Sequence
from os import PathLike
from types import TracebackType


class SpiDev:

    def __init__(self, bus: int | None = None, device: int | None = None) -> None:
        if bus is not None:
            self._bus = int(bus)
        if device is not None:
            self._device = int(device)

        self._cmod = _SpiDev(self._bus, self._device)

    @property
    def bus(self) -> int:
        return self._bus

    @property
    def device(self) -> int:
        return self._device

    @property
    def mode(self) -> int:
        return self._cmod.mode

    @mode.setter
    def mode(self, value: int, /) -> None:
        self._cmod.mode = int(value)

    @property
    def cshigh(self) -> bool:
        return self._cmod.cshigh

    @cshigh.setter
    def cshigh(self, value: bool, /) -> None:
        self._cmod.cshigh = bool(value)

    @property
    def threewire(self) -> bool:
        return self._cmod.threewire

    @threewire.setter
    def threewire(self, value: bool, /) -> None:
        self._cmod.threewire = bool(value)

    @property
    def lsbfirst(self) -> bool:
        return self._cmod.lsbfirst

    @lsbfirst.setter
    def lsbfirst(self, value: bool, /) -> None:
        self._cmod.lsbfirst = bool(value)

    @property
    def loop(self) -> bool:
        return self._cmod.loop

    @loop.setter
    def loop(self, value: bool, /) -> None:
        self._cmod.loop = bool(value)

    @property
    def no_cs(self) -> bool:
        return self._cmod.no_cs

    @no_cs.setter
    def no_cs(self, value: bool, /) -> None:
        self._cmod.no_cs = bool(value)

    @property
    def bits_per_word(self) -> int:
        return self._cmod.bits_per_word

    @bits_per_word.setter
    def bits_per_word(self, value: int, /) -> None:
        self._cmod.bits_per_word = int(value)

    @property
    def max_speed_hz(self) -> int:
        return self._cmod.max_speed_hz

    @max_speed_hz.setter
    def max_speed_hz(self, value: int, /) -> None:
        self._cmod.max_speed_hz = int(value)

    @property
    def read0(self) -> bool:
        return self._cmod.read0

    @read0.setter
    def read0(self, value: bool, /) -> None:
        self._cmod.read0 = bool(value)

    @property
    def mosi_idle_low(self) -> bool:
        return self._cmod.mosi_idle_low

    @mosi_idle_low.setter
    def mosi_idle_low(self, value: bool, /) -> None:
        self._cmod.mosi_idle_low = bool(value)

    def close(self) -> None:
        self._cmod.close()

    def open(self, bus: int, device: int) -> None:
        self._cmod.open(bus, device)

    def open_path(self, path: str) -> None:
        self._cmod.open_path(path)

    def fileno(self) -> int:
        return self._cmod.fileno()

    def readbytes(self, length: int) -> list[int]:
        return self._cmod.readbytes(length)

    def writebytes(self, values: Sequence[int]) -> None:
        self._cmod.writebytes(values)

    # TODO: also accepts collections.abc.Buffer type, but that's 3.12+...
    def writebytes2(self, values: Sequence[int]) -> None:
        self._cmod.writebytes2(values)

    def xfer(
        self,
        values: Sequence[int],
        speed_hz: int = 0,
        delay_usecs: int = 0,
        bits_per_word: int = 0,
    ) -> list[int]:
        return self._cmod.xfer(values, speed_hz, delay_usecs, bits_per_word)

    def xfer2(
        self,
        values: Sequence[int],
        speed_hz: int = 0,
        delay_usecs: int = 0,
        bits_per_word: int = 0,
    ) -> list[int]:
        return self._cmod.xfer2(values, speed_hz, delay_usecs, bits_per_word)

    def xfer3(
        self,
        values: Sequence[int],
        speed_hz: int = 0,
        delay_usecs: int = 0,
        bits_per_word: int = 0,
    ) -> tuple[int, ...]:
        return self._cmod.xfer3(values, speed_hz, delay_usecs, bits_per_word)

    def __del__(self) -> None:
        with suppress(AttributeError):
            del self._cmod

    def __enter__(self) -> "SpiDev":
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> None:
        self._cmod.__exit__(exc_type, exc_value, exc_tb)
