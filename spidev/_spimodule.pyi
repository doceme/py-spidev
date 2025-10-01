# I generated this with ChatGPT and think it's mostly correct. It is not meant
# to be permanent and is their to help with uplifting most of the C module's
# code to a .py file while making sure the properties and method signature stay
# the same.
from collections.abc import Buffer, Sequence

class SpiDev:
    def __init__(self, bus: int | None = ..., client: int | None = ...) -> None: ...
    def open_path(self, path: str) -> None: ...
    def close(self) -> None: ...
    def fileno(self) -> int: ...
    def readbytes(self, length: int) -> list[int]: ...
    def writebytes(self, values: Sequence[int]) -> None: ...
    def writebytes2(self, values: Sequence[int] | Buffer) -> None: ...
    def xfer(
        self,
        values: Sequence[int],
        speed_hz: int | None = ...,
        delay_usecs: int | None = ...,
        bits_per_word: int | None = ...,
    ) -> list[int]: ...
    def xfer2(
        self,
        values: Sequence[int],
        speed_hz: int | None = ...,
        delay_usecs: int | None = ...,
        bits_per_word: int | None = ...,
    ) -> list[int]: ...
    def xfer3(
        self,
        values: Sequence[int],
        speed_hz: int | None = ...,
        delay_usecs: int | None = ...,
        bits_per_word: int | None = ...,
    ) -> tuple[int, ...]: ...

    mode: int
    cshigh: bool
    threewire: bool
    lsbfirst: bool
    loop: bool
    no_cs: bool
    bits_per_word: int
    max_speed_hz: int
    read0: bool

__version__: str
