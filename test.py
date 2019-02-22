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
        bytes = 1000
        count = 1000

        buffer = bytearray(1000)
        print("devspi readbytes", spi.readbytes(10))
        print("devspi readbytes", numpy.array(spi.readbytes(10), dtype=numpy.int16))
        print("devspi readbytesb", numpy.frombuffer(spi.readbytesb(10), dtype=numpy.int16))
        spi.readbuffer(buffer, 10);
        print("devspi readbuffer", numpy.frombuffer(buffer[:10], dtype=numpy.int16))

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