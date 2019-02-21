import sys
sys.path = ['.'] + sys.path
import spidev as spidev

spi = spidev.SpiDev()
spi.open(0,0)
print(spi.readbytes(10))
b = spi.readbytesb(10)
print(type(b), len(b))
print(b)
b = spi.xfer2(bytes(range(10)))
print(type(b), len(b))
print(b)

b = bytearray(range(10))
print (b)
spi.bidi (b, 1)
print (b)
spi.bidi (b)
print (b)

import numpy
b = bytearray(range(48))
nlinear = numpy.frombuffer(b, dtype=numpy.uint16)
n64 = numpy.reshape(nlinear, (6, 4))
n38 = numpy.reshape(nlinear, (3, 8))
print(nlinear.view(numpy.uint8))
print(n64)
#b[0] = 1;
#print(n2)
spi.bidi(n38[0,:], 1)
spi.bidi(n38[1,:], 50)
spi.bidi(n38[2,:], 100)
print(n64)
print(nlinear.view(numpy.uint8))
print(b)

# print(n)
# b = bytearray(n[:8])
# print(b)
# spi.bidi(b, 1)
# print(n)


