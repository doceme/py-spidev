import sys
sys.path = ['.'] + sys.path

# https://pypi.org/project/spidev/
import spidev
import numpy
import timeit
class testspi (object):
    def __init__(self):
        spi=spidev.SpiDev()
        spi.open(0, 0)
        print("devspi readbytes", spi.readbytes(50))
        spibytes = spi.readbytesb(50)
        npbytes = numpy.frombuffer(spibytes, dtype=numpy.int16)
        print("devspi readbytesb", npbytes)
        bytes = 1000
        buffer = bytearray(1000)
        count = 1000
        def read(): spi.readbytes(bytes)
        def readb(): spi.readbytesb(bytes)
        def readbuffer(): spi.readbuffer(buffer)
        duration = timeit.timeit(read, number=count)
        print('list: total dur %f; secs/byte=%f; bytes/sec=%f' % (duration, duration/(bytes*count), (bytes*count)/duration))
        duration = timeit.timeit(readb, number=count)
        print('bytes: total dur %f; secs/byte=%f; bytes/sec=%f' % (duration, duration/(bytes*count), (bytes*count)/duration))
        duration = timeit.timeit(readbuffer, number=count)
        print('bytes: total dur %f; secs/byte=%f; bytes/sec=%f' % (duration, duration/(bytes*count), (bytes*count)/duration))
        spi.close()


    def buffertests(self):
        spi=spidev.SpiDev()
        spi.open(0, 0)
        buffer = bytearray(range(20))
        print(buffer)
        spi.readbuffer(buffer, 5)
        print(buffer)
        spi.readbuffer(buffer, 5, 10)
        print(buffer)
        spi.readbuffer(buffer, 0, 15)
        print(buffer)

        buffer = bytearray(range(20))
        print(buffer)
        spi.readbuffer(buffer)
        print(buffer)

testspi().buffertests()