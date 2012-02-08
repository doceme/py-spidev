#!/usr/bin/env python

from distutils.core import setup, Extension

setup(	name="spidev",
	version="2.0",
	description="Python bindings for Linux SPI access through spidev",
	author="Volker Thoms",
	author_email="unconnected@gmx.de",
	maintainer="Stephen Caudle",
	maintainer_email="scaudle@doceme.com",
	license="GPLv2",
	url="http://github.com/doceme/py-spidev",
	include_dirs=["/usr/src/linux/include"],
	ext_modules=[Extension("spidev", ["spidev_module.c"])])
