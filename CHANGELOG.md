Changelog
---------

Unreleased
====

* Added open_path method to accommodate dynamic SPI bus number allocation

3.6
====

* Added read0 flag to enable reading 0 bytes after transfer to lower CS when cshigh == True

3.5
====

* Fixed memory leaks

3.4
=====

* Changed license to MIT

3.0.1
=====

* Fixed README.md and CHANGELOG.md formatting, hopefully

3.0
===

* Memset fix recommended by Dougie Lawson
* Fixes for Kernel 3.15+ from https://github.com/chrillomat/py-spidev
* Fixes for Python 3/2 compatibility.
* Added subclassing support - https://github.com/doceme/py-spidev/issues/10

2.0
===

Code sourced from http://elk.informatik.fh-augsburg.de/da/da-49/trees/pyap7k/lang/py-spi
and modified.

Pre 2.0
=======

spimodule.c originally uathored by Volker Thoms, 2009.
