#!/usr/bin/env python

from distutils.core import setup, Extension

version = "0.0"

lines = [x for x in open("spidev_module.c").read().split("\n") if "#define" in x and "_VERSION_" in x and "\"" in x]

if len(lines) > 0:
    version = lines[0].split("\"")[1]
else:
    raise Exception("Unable to find _VERSION_ in spidev_module.c")


classifiers = ['Development Status :: 5 - Production/Stable',
               'Operating System :: POSIX :: Linux',
               'License :: OSI Approved :: GNU General Public License v2 (GPLv2)',
               'Intended Audience :: Developers',
               'Programming Language :: Python :: 2.6',
               'Programming Language :: Python :: 2.7',
               'Programming Language :: Python :: 3',
               'Topic :: Software Development',
               'Topic :: System :: Hardware',
               'Topic :: System :: Hardware :: Hardware Drivers']

setup(	name		= "spidev",
	version		= version,
	description	= "Python bindings for Linux SPI access through spidev",
	long_description= open('README.md').read() + "\n" + open('CHANGELOG.md').read(),
	author		= "Volker Thoms",
	author_email	= "unconnected@gmx.de",
	maintainer	= "Stephen Caudle",
	maintainer_email= "scaudle@doceme.com",
	license		= "GPLv2",
	classifiers	= classifiers,
	url		= "http://github.com/doceme/py-spidev",
	ext_modules	= [Extension("spidev", ["spidev_module.c"])]
)
