/*
 * spidev_module.c - Python bindings for Linux SPI access through spidev
 * Copyright (C) 2009 Volker Thoms <unconnected@gmx.de>
 * Copyright (C) 2012 Stephen Caudle <scaudle@doceme.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <Python.h>
#include "structmember.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#define _VERSION_ "3.3"
#define SPIDEV_MAXPATH 4096


#if PY_MAJOR_VERSION < 3
#define PyLong_AS_LONG(val) PyInt_AS_LONG(val)
#define PyLong_AsLong(val) PyInt_AsLong(val)
#endif

// Macros needed for Python 3
#ifndef PyInt_Check
#define PyInt_Check			PyLong_Check
#define PyInt_FromLong	PyLong_FromLong
#define PyInt_AsLong		PyLong_AsLong
#define PyInt_Type			PyLong_Type
#endif

PyDoc_STRVAR(SpiDev_module_doc,
	"This module defines an object type that allows SPI transactions\n"
	"on hosts running the Linux kernel. The host kernel must have SPI\n"
	"support and SPI device interface support.\n"
	"All of these can be either built-in to the kernel, or loaded from\n"
	"modules.\n"
	"\n"
	"Because the SPI device interface is opened R/W, users of this\n"
	"module usually must have root permissions.\n");

typedef struct {
	PyObject_HEAD

	int fd;	/* open file descriptor: /dev/spidevX.Y */
	uint8_t mode;	/* current SPI mode */
	uint8_t bits_per_word;	/* current SPI bits per word setting */
	uint32_t max_speed_hz;	/* current SPI max speed setting in Hz */
} SpiDevObject;

static PyObject *
SpiDev_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	SpiDevObject *self;
	if ((self = (SpiDevObject *)type->tp_alloc(type, 0)) == NULL)
		return NULL;

	self->fd = -1;
	self->mode = 0;
	self->bits_per_word = 0;
	self->max_speed_hz = 0;

	Py_INCREF(self);
	return (PyObject *)self;
}

PyDoc_STRVAR(SpiDev_close_doc,
	"close()\n\n"
	"Disconnects the object from the interface.\n");

static PyObject *
SpiDev_close(SpiDevObject *self)
{
	if ((self->fd != -1) && (close(self->fd) == -1)) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	self->fd = -1;
	self->mode = 0;
	self->bits_per_word = 0;
	self->max_speed_hz = 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static void
SpiDev_dealloc(SpiDevObject *self)
{
	PyObject *ref = SpiDev_close(self);
	Py_XDECREF(ref);

	Py_TYPE(self)->tp_free((PyObject *)self);
}

static char *wrmsg_list0 = "Empty argument list.";
static char *wrmsg_listmax = "Argument list size exceeds %d bytes.";
static char *wrmsg_val = "Non-Int/Long value in arguments: %x.";

PyDoc_STRVAR(SpiDev_write_doc,
	"write([values]) -> None\n\n"
	"Write bytes to SPI device.\n");

static PyObject *
SpiDev_writebytes(SpiDevObject *self, PyObject *args)
{
	int		status;
	uint16_t	ii, len;
	uint8_t	buf[SPIDEV_MAXPATH];
	PyObject	*obj;
	PyObject	*seq;
	char	wrmsg_text[4096];

	if (!PyArg_ParseTuple(args, "O:write", &obj))
		return NULL;

	seq = PySequence_Fast(obj, "expected a sequence");
	len = PySequence_Fast_GET_SIZE(obj);
	if (!seq || len <= 0) {
		PyErr_SetString(PyExc_TypeError, wrmsg_list0);
		return NULL;
	}

	if (len > SPIDEV_MAXPATH) {
		snprintf(wrmsg_text, sizeof (wrmsg_text) - 1, wrmsg_listmax, SPIDEV_MAXPATH);
		PyErr_SetString(PyExc_OverflowError, wrmsg_text);
		return NULL;
	}

	for (ii = 0; ii < len; ii++) {
		PyObject *val = PySequence_Fast_GET_ITEM(seq, ii);
#if PY_MAJOR_VERSION < 3
		if (PyInt_Check(val)) {
			buf[ii] = (__u8)PyInt_AS_LONG(val);
		} else
#endif
		{
			if (PyLong_Check(val)) {
				buf[ii] = (__u8)PyLong_AS_LONG(val);
			} else {
				snprintf(wrmsg_text, sizeof (wrmsg_text) - 1, wrmsg_val, val);
				PyErr_SetString(PyExc_TypeError, wrmsg_text);
				return NULL;
			}
		}
	}

	Py_DECREF(seq);

	status = write(self->fd, &buf[0], len);

	if (status < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	if (status != len) {
		perror("short write");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(SpiDev_read_doc,
	"read(len) -> [values]\n\n"
	"Read len bytes from SPI device.\n");

static PyObject *
SpiDev_readbytes(SpiDevObject *self, PyObject *args)
{
	uint8_t	rxbuf[SPIDEV_MAXPATH];
	int		status, len, ii;
	PyObject	*list;

	if (!PyArg_ParseTuple(args, "i:read", &len))
		return NULL;

	/* read at least 1 byte, no more than SPIDEV_MAXPATH */
	if (len < 1)
		len = 1;
	else if ((unsigned)len > sizeof(rxbuf))
		len = sizeof(rxbuf);

	memset(rxbuf, 0, sizeof rxbuf);
	status = read(self->fd, &rxbuf[0], len);

	if (status < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	if (status != len) {
		perror("short read");
		return NULL;
	}

	list = PyList_New(len);

	for (ii = 0; ii < len; ii++) {
		PyObject *val = Py_BuildValue("l", (long)rxbuf[ii]);
		PyList_SET_ITEM(list, ii, val);
	}

	return list;
}

PyDoc_STRVAR(SpiDev_xfer_doc,
	"xfer([values]) -> [values]\n\n"
	"Perform SPI transaction.\n"
	"CS will be released and reactivated between blocks.\n"
	"delay specifies delay in usec between blocks.\n");

static PyObject *
SpiDev_xfer(SpiDevObject *self, PyObject *args)
{
	uint16_t ii, len;
	int status;
	uint16_t delay_usecs = 0;
	uint32_t speed_hz = 0;
	uint8_t bits_per_word = 0;
	PyObject *obj;
	PyObject *seq;
#ifdef SPIDEV_SINGLE
	struct spi_ioc_transfer *xferptr;
	memset(&xferptr, 0, sizeof(xferptr));
#else
	struct spi_ioc_transfer xfer;
	memset(&xfer, 0, sizeof(xfer));
#endif
	uint8_t *txbuf, *rxbuf;
	char	wrmsg_text[4096];

	if (!PyArg_ParseTuple(args, "O|IHB:xfer", &obj, &speed_hz, &delay_usecs, &bits_per_word))
		return NULL;

	seq = PySequence_Fast(obj, "expected a sequence");
	len = PySequence_Fast_GET_SIZE(obj);
	if (!seq || len <= 0) {
		PyErr_SetString(PyExc_TypeError, wrmsg_list0);
		return NULL;
	}

	if (len > SPIDEV_MAXPATH) {
		snprintf(wrmsg_text, sizeof(wrmsg_text) - 1, wrmsg_listmax, SPIDEV_MAXPATH);
		PyErr_SetString(PyExc_OverflowError, wrmsg_text);
		return NULL;
	}

	txbuf = malloc(sizeof(__u8) * len);
	rxbuf = malloc(sizeof(__u8) * len);

#ifdef SPIDEV_SINGLE
	xferptr = (struct spi_ioc_transfer*) malloc(sizeof(struct spi_ioc_transfer) * len);

	for (ii = 0; ii < len; ii++) {
		PyObject *val = PySequence_Fast_GET_ITEM(seq, ii);
#if PY_MAJOR_VERSION < 3
		if (PyInt_Check(val)) {
			txbuf[ii] = (__u8)PyInt_AS_LONG(val);
		} else
#endif
		{
			if (PyLong_Check(val)) {
				txbuf[ii] = (__u8)PyLong_AS_LONG(val);
			} else {
				snprintf(wrmsg_text, sizeof(wrmsg_text) - 1, wrmsg_val, val);
				PyErr_SetString(PyExc_TypeError, wrmsg_text);
				free(xferptr);
				free(txbuf);
				free(rxbuf);
				return NULL;
			}
		}
		xferptr[ii].tx_buf = (unsigned long)&txbuf[ii];
		xferptr[ii].rx_buf = (unsigned long)&rxbuf[ii];
		xferptr[ii].len = 1;
		xferptr[ii].delay_usecs = delay;
		xferptr[ii].speed_hz = speed_hz ? speed_hz : self->max_speed_hz;
		xferptr[ii].bits_per_word = bits_per_word ? bits_per_word : self->bits_per_word;
#ifdef SPI_IOC_WR_MODE32
		xferptr[ii].tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
		xferptr[ii].rx_nbits = 0;
#endif
	}

	status = ioctl(self->fd, SPI_IOC_MESSAGE(len), xferptr);
	if (status < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		free(xferptr);
		free(txbuf);
		free(rxbuf);
		return NULL;
	}
#else
	for (ii = 0; ii < len; ii++) {
		PyObject *val = PySequence_Fast_GET_ITEM(seq, ii);
#if PY_MAJOR_VERSION < 3
		if (PyInt_Check(val)) {
			txbuf[ii] = (__u8)PyInt_AS_LONG(val);
		} else
#endif
		{
			if (PyLong_Check(val)) {
				txbuf[ii] = (__u8)PyLong_AS_LONG(val);
			} else {
				snprintf(wrmsg_text, sizeof(wrmsg_text) - 1, wrmsg_val, val);
				PyErr_SetString(PyExc_TypeError, wrmsg_text);
				free(txbuf);
				free(rxbuf);
				return NULL;
			}
		}
	}

	if (PyTuple_Check(obj)) {
		Py_DECREF(seq);
		seq = PySequence_List(obj);
	}

	xfer.tx_buf = (unsigned long)txbuf;
	xfer.rx_buf = (unsigned long)rxbuf;
	xfer.len = len;
	xfer.delay_usecs = delay_usecs;
	xfer.speed_hz = speed_hz ? speed_hz : self->max_speed_hz;
	xfer.bits_per_word = bits_per_word ? bits_per_word : self->bits_per_word;
#ifdef SPI_IOC_WR_MODE32
	xfer.tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
	xfer.rx_nbits = 0;
#endif

	status = ioctl(self->fd, SPI_IOC_MESSAGE(1), &xfer);
	if (status < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		free(txbuf);
		free(rxbuf);
		return NULL;
	}
#endif

	for (ii = 0; ii < len; ii++) {
		PyObject *val = Py_BuildValue("l", (long)rxbuf[ii]);
		PySequence_SetItem(seq, ii, val);
	}

	// WA:
	// in CS_HIGH mode CS isn't pulled to low after transfer, but after read
	// reading 0 bytes doesnt matter but brings cs down
	// tomdean:
	// Stop generating an extra CS except in mode CS_HOGH
	if (self->mode & SPI_CS_HIGH) status = read(self->fd, &rxbuf[0], 0);

	free(txbuf);
	free(rxbuf);

	if (PyTuple_Check(obj)) {
		PyObject *old = seq;
		seq = PySequence_Tuple(seq);
		Py_DECREF(old);
	}

	return seq;
}


PyDoc_STRVAR(SpiDev_xfer2_doc,
	"xfer2([values]) -> [values]\n\n"
	"Perform SPI transaction.\n"
	"CS will be held active between blocks.\n");

static PyObject *
SpiDev_xfer2(SpiDevObject *self, PyObject *args)
{
	int status;
	uint16_t delay_usecs = 0;
	uint32_t speed_hz = 0;
	uint8_t bits_per_word = 0;
	uint16_t ii, len;
	PyObject *obj;
	PyObject *seq;
	struct spi_ioc_transfer xfer;
	Py_BEGIN_ALLOW_THREADS
	memset(&xfer, 0, sizeof(xfer));
	Py_END_ALLOW_THREADS
	uint8_t *txbuf, *rxbuf;
	char	wrmsg_text[4096];

	if (!PyArg_ParseTuple(args, "O|IHB:xfer2", &obj, &speed_hz, &delay_usecs, &bits_per_word))
		return NULL;

	seq = PySequence_Fast(obj, "expected a sequence");
	len = PySequence_Fast_GET_SIZE(obj);
	if (!seq || len <= 0) {
		PyErr_SetString(PyExc_TypeError, wrmsg_list0);
		return NULL;
	}

	if (len > SPIDEV_MAXPATH) {
		snprintf(wrmsg_text, sizeof(wrmsg_text) - 1, wrmsg_listmax, SPIDEV_MAXPATH);
		PyErr_SetString(PyExc_OverflowError, wrmsg_text);
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	txbuf = malloc(sizeof(__u8) * len);
	rxbuf = malloc(sizeof(__u8) * len);
	Py_END_ALLOW_THREADS

	for (ii = 0; ii < len; ii++) {
		PyObject *val = PySequence_Fast_GET_ITEM(seq, ii);
#if PY_MAJOR_VERSION < 3
		if (PyInt_Check(val)) {
			txbuf[ii] = (__u8)PyInt_AS_LONG(val);
		} else
#endif
		{
			if (PyLong_Check(val)) {
				txbuf[ii] = (__u8)PyLong_AS_LONG(val);
			} else {
				snprintf(wrmsg_text, sizeof (wrmsg_text) - 1, wrmsg_val, val);
				PyErr_SetString(PyExc_TypeError, wrmsg_text);
				free(txbuf);
				free(rxbuf);
				return NULL;
			}
		}
	}

	if (PyTuple_Check(obj)) {
		Py_DECREF(seq);
		seq = PySequence_List(obj);
	}

	Py_BEGIN_ALLOW_THREADS
	xfer.tx_buf = (unsigned long)txbuf;
	xfer.rx_buf = (unsigned long)rxbuf;
	xfer.len = len;
	xfer.delay_usecs = delay_usecs;
	xfer.speed_hz = speed_hz ? speed_hz : self->max_speed_hz;
	xfer.bits_per_word = bits_per_word ? bits_per_word : self->bits_per_word;

	status = ioctl(self->fd, SPI_IOC_MESSAGE(1), &xfer);
	Py_END_ALLOW_THREADS
	if (status < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		free(txbuf);
		free(rxbuf);
		return NULL;
	}

	for (ii = 0; ii < len; ii++) {
		PyObject *val = Py_BuildValue("l", (long)rxbuf[ii]);
		PySequence_SetItem(seq, ii, val);
	}
	// WA:
	// in CS_HIGH mode CS isnt pulled to low after transfer
	// reading 0 bytes doesn't really matter but brings CS down
	// tomdean:
	// Stop generating an extra CS except in mode CS_HOGH
	if (self->mode & SPI_CS_HIGH) status = read(self->fd, &rxbuf[0], 0);

	Py_BEGIN_ALLOW_THREADS
	free(txbuf);
	free(rxbuf);
	Py_END_ALLOW_THREADS


	if (PyTuple_Check(obj)) {
		PyObject *old = seq;
		seq = PySequence_Tuple(seq);
		Py_DECREF(old);
	}

	return seq;
}

static int __spidev_set_mode( int fd, __u8 mode) {
	__u8 test;
	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return -1;
	}
	if (ioctl(fd, SPI_IOC_RD_MODE, &test) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return -1;
	}
	if (test != mode) {
		return -1;
	}
	return 0;
}

PyDoc_STRVAR(SpiDev_fileno_doc,
	"fileno() -> integer \"file descriptor\"\n\n"
	"This is needed for lower-level file interfaces, such as os.read().\n");

static PyObject *
SpiDev_fileno(SpiDevObject *self)
{
	PyObject *result = Py_BuildValue("i", self->fd);
	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_mode(SpiDevObject *self, void *closure)
{
	PyObject *result = Py_BuildValue("i", (self->mode & (SPI_CPHA | SPI_CPOL) ) );
	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_cshigh(SpiDevObject *self, void *closure)
{
	PyObject *result;

	if (self->mode & SPI_CS_HIGH)
		result = Py_True;
	else
		result = Py_False;

	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_lsbfirst(SpiDevObject *self, void *closure)
{
	PyObject *result;

	if (self->mode & SPI_LSB_FIRST)
		result = Py_True;
	else
		result = Py_False;

	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_3wire(SpiDevObject *self, void *closure)
{
	PyObject *result;

	if (self->mode & SPI_3WIRE)
		result = Py_True;
	else
		result = Py_False;

	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_loop(SpiDevObject *self, void *closure)
{
	PyObject *result;

	if (self->mode & SPI_LOOP)
		result = Py_True;
	else
		result = Py_False;

	Py_INCREF(result);
	return result;
}

static PyObject *
SpiDev_get_no_cs(SpiDevObject *self, void *closure)
{
        PyObject *result;

        if (self->mode & SPI_NO_CS)
                result = Py_True;
        else
                result = Py_False;

        Py_INCREF(result);
        return result;
}


static int
SpiDev_set_mode(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t mode, tmp;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(val)) {
		mode = PyInt_AS_LONG(val);
	} else
#endif
	{
		if (PyLong_Check(val)) {
			mode = PyLong_AS_LONG(val);
		} else {
			PyErr_SetString(PyExc_TypeError,
				"The mode attribute must be an integer");
			return -1;
		}
	}


	if ( mode > 3 ) {
		PyErr_SetString(PyExc_TypeError,
			"The mode attribute must be an integer"
				 "between 0 and 3.");
		return -1;
	}

	// clean and set CPHA and CPOL bits
	tmp = ( self->mode & ~(SPI_CPHA | SPI_CPOL) ) | mode ;

	__spidev_set_mode(self->fd, tmp);

	self->mode = tmp;
	return 0;
}

static int
SpiDev_set_cshigh(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t tmp;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
	else if (!PyBool_Check(val)) {
		PyErr_SetString(PyExc_TypeError,
			"The cshigh attribute must be boolean");
		return -1;
	}

	if (val == Py_True)
		tmp = self->mode | SPI_CS_HIGH;
	else
		tmp = self->mode & ~SPI_CS_HIGH;

	__spidev_set_mode(self->fd, tmp);

	self->mode = tmp;
	return 0;
}

static int
SpiDev_set_lsbfirst(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t tmp;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
	else if (!PyBool_Check(val)) {
		PyErr_SetString(PyExc_TypeError,
			"The lsbfirst attribute must be boolean");
		return -1;
	}

	if (val == Py_True)
		tmp = self->mode | SPI_LSB_FIRST;
	else
		tmp = self->mode & ~SPI_LSB_FIRST;

	__spidev_set_mode(self->fd, tmp);

	self->mode = tmp;
	return 0;
}

static int
SpiDev_set_3wire(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t tmp;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
	else if (!PyBool_Check(val)) {
		PyErr_SetString(PyExc_TypeError,
			"The 3wire attribute must be boolean");
		return -1;
	}

	if (val == Py_True)
		tmp = self->mode | SPI_3WIRE;
	else
		tmp = self->mode & ~SPI_3WIRE;

	__spidev_set_mode(self->fd, tmp);

	self->mode = tmp;
	return 0;
}

static int
SpiDev_set_no_cs(SpiDevObject *self, PyObject *val, void *closure)
{
        uint8_t tmp;

        if (val == NULL) {
                PyErr_SetString(PyExc_TypeError,
                        "Cannot delete attribute");
                return -1;
        }
        else if (!PyBool_Check(val)) {
                PyErr_SetString(PyExc_TypeError,
                        "The no_cs attribute must be boolean");
                return -1;
        }

        if (val == Py_True)
                tmp = self->mode | SPI_NO_CS;
        else
                tmp = self->mode & ~SPI_NO_CS;

        __spidev_set_mode(self->fd, tmp);

        self->mode = tmp;
        return 0;
}


static int
SpiDev_set_loop(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t tmp;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
	else if (!PyBool_Check(val)) {
		PyErr_SetString(PyExc_TypeError,
			"The loop attribute must be boolean");
		return -1;
	}

	if (val == Py_True)
		tmp = self->mode | SPI_LOOP;
	else
		tmp = self->mode & ~SPI_LOOP;

	__spidev_set_mode(self->fd, tmp);

	self->mode = tmp;
	return 0;
}

static PyObject *
SpiDev_get_bits_per_word(SpiDevObject *self, void *closure)
{
	PyObject *result = Py_BuildValue("i", self->bits_per_word);
	Py_INCREF(result);
	return result;
}

static int
SpiDev_set_bits_per_word(SpiDevObject *self, PyObject *val, void *closure)
{
	uint8_t bits;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(val)) {
		bits = PyInt_AS_LONG(val);
	} else
#endif
	{
		if (PyLong_Check(val)) {
			bits = PyLong_AS_LONG(val);
		} else {
			PyErr_SetString(PyExc_TypeError,
				"The bits_per_word attribute must be an integer");
			return -1;
		}
	}

		if (bits < 8 || bits > 16) {
		PyErr_SetString(PyExc_TypeError,
			"invalid bits_per_word (8 to 16)");
		return -1;
	}

	if (self->bits_per_word != bits) {
		if (ioctl(self->fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
			PyErr_SetFromErrno(PyExc_IOError);
			return -1;
		}
		self->bits_per_word = bits;
	}
	return 0;
}

static PyObject *
SpiDev_get_max_speed_hz(SpiDevObject *self, void *closure)
{
	PyObject *result = Py_BuildValue("i", self->max_speed_hz);
	Py_INCREF(result);
	return result;
}

static int
SpiDev_set_max_speed_hz(SpiDevObject *self, PyObject *val, void *closure)
{
	uint32_t max_speed_hz;

	if (val == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"Cannot delete attribute");
		return -1;
	}
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(val)) {
		max_speed_hz = PyInt_AS_LONG(val);
	} else
#endif
	{
		if (PyLong_Check(val)) {
			max_speed_hz = PyLong_AS_LONG(val);
		} else {
			PyErr_SetString(PyExc_TypeError,
				"The max_speed_hz attribute must be an integer");
			return -1;
		}
	}

	if (self->max_speed_hz != max_speed_hz) {
		if (ioctl(self->fd, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed_hz) == -1) {
			PyErr_SetFromErrno(PyExc_IOError);
			return -1;
		}
		self->max_speed_hz = max_speed_hz;
	}
	return 0;
}

static PyGetSetDef SpiDev_getset[] = {
	{"mode", (getter)SpiDev_get_mode, (setter)SpiDev_set_mode,
			"SPI mode as two bit pattern of \n"
			"Clock Polarity  and Phase [CPOL|CPHA]\n"
			"min: 0b00 = 0 max: 0b11 = 3\n"},
	{"cshigh", (getter)SpiDev_get_cshigh, (setter)SpiDev_set_cshigh,
			"CS active high\n"},
	{"threewire", (getter)SpiDev_get_3wire, (setter)SpiDev_set_3wire,
			"SI/SO signals shared\n"},
	{"lsbfirst", (getter)SpiDev_get_lsbfirst, (setter)SpiDev_set_lsbfirst,
			"LSB first\n"},
	{"loop", (getter)SpiDev_get_loop, (setter)SpiDev_set_loop,
			"loopback configuration\n"},
	{"no_cs", (getter)SpiDev_get_no_cs, (setter)SpiDev_set_no_cs,
			"disable chip select\n"},
	{"bits_per_word", (getter)SpiDev_get_bits_per_word, (setter)SpiDev_set_bits_per_word,
			"bits per word\n"},
	{"max_speed_hz", (getter)SpiDev_get_max_speed_hz, (setter)SpiDev_set_max_speed_hz,
			"maximum speed in Hz\n"},
	{NULL},
};

PyDoc_STRVAR(SpiDev_open_doc,
	"open(bus, device)\n\n"
	"Connects the object to the specified SPI device.\n"
	"open(X,Y) will open /dev/spidev<X>.<Y>\n");

static PyObject *
SpiDev_open(SpiDevObject *self, PyObject *args, PyObject *kwds)
{
	int bus, device;
	char path[SPIDEV_MAXPATH];
	uint8_t tmp8;
	uint32_t tmp32;
	static char *kwlist[] = {"bus", "device", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii:open", kwlist, &bus, &device))
		return NULL;
	if (snprintf(path, SPIDEV_MAXPATH, "/dev/spidev%d.%d", bus, device) >= SPIDEV_MAXPATH) {
		PyErr_SetString(PyExc_OverflowError,
			"Bus and/or device number is invalid.");
		return NULL;
	}
	if ((self->fd = open(path, O_RDWR, 0)) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	if (ioctl(self->fd, SPI_IOC_RD_MODE, &tmp8) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	self->mode = tmp8;
	if (ioctl(self->fd, SPI_IOC_RD_BITS_PER_WORD, &tmp8) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	self->bits_per_word = tmp8;
	if (ioctl(self->fd, SPI_IOC_RD_MAX_SPEED_HZ, &tmp32) == -1) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	self->max_speed_hz = tmp32;

	Py_INCREF(Py_None);
	return Py_None;
}

static int
SpiDev_init(SpiDevObject *self, PyObject *args, PyObject *kwds)
{
	int bus = -1;
	int client = -1;
	static char *kwlist[] = {"bus", "client", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ii:__init__",
			kwlist, &bus, &client))
		return -1;

	if (bus >= 0) {
		SpiDev_open(self, args, kwds);
		if (PyErr_Occurred())
			return -1;
	}

	return 0;
}


PyDoc_STRVAR(SpiDevObjectType_doc,
	"SpiDev([bus],[client]) -> SPI\n\n"
	"Return a new SPI object that is (optionally) connected to the\n"
	"specified SPI device interface.\n");

static
PyObject *SpiDev_enter(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Py_INCREF(self);
    return self;
}

static
PyObject *SpiDev_exit(SpiDevObject *self, PyObject *args)
{

    PyObject *exc_type = 0;
    PyObject *exc_value = 0;
    PyObject *traceback = 0;
    if (!PyArg_UnpackTuple(args, "__exit__", 3, 3, &exc_type, &exc_value,
                           &traceback)) {
        return 0;
    }

    SpiDev_close(self);
    Py_RETURN_FALSE;
}

static PyMethodDef SpiDev_methods[] = {
	{"open", (PyCFunction)SpiDev_open, METH_VARARGS | METH_KEYWORDS,
		SpiDev_open_doc},
	{"close", (PyCFunction)SpiDev_close, METH_NOARGS,
		SpiDev_close_doc},
	{"fileno", (PyCFunction)SpiDev_fileno, METH_NOARGS,
		SpiDev_fileno_doc},
	{"readbytes", (PyCFunction)SpiDev_readbytes, METH_VARARGS,
		SpiDev_read_doc},
	{"writebytes", (PyCFunction)SpiDev_writebytes, METH_VARARGS,
		SpiDev_write_doc},
	{"xfer", (PyCFunction)SpiDev_xfer, METH_VARARGS,
		SpiDev_xfer_doc},
	{"xfer2", (PyCFunction)SpiDev_xfer2, METH_VARARGS,
		SpiDev_xfer2_doc},
	{"__enter__", (PyCFunction)SpiDev_enter, METH_VARARGS,
		NULL},
	{"__exit__", (PyCFunction)SpiDev_exit, METH_VARARGS,
		NULL},
	{NULL},
};

static PyTypeObject SpiDevObjectType = {
#if PY_MAJOR_VERSION >= 3
	PyVarObject_HEAD_INIT(NULL, 0)
#else
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
#endif
	"SpiDev",			/* tp_name */
	sizeof(SpiDevObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	(destructor)SpiDev_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	0,				/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	SpiDevObjectType_doc,		/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	SpiDev_methods,			/* tp_methods */
	0,				/* tp_members */
	SpiDev_getset,			/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)SpiDev_init,		/* tp_init */
	0,				/* tp_alloc */
	SpiDev_new,			/* tp_new */
};

static PyMethodDef SpiDev_module_methods[] = {
	{NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"spidev",
	SpiDev_module_doc,
	-1,
	SpiDev_module_methods,
	NULL,
	NULL,
	NULL,
	NULL,
};
#else
#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_spidev(void)
#else
void initspidev(void)
#endif
{
	PyObject* m;

	if (PyType_Ready(&SpiDevObjectType) < 0)
#if PY_MAJOR_VERSION >= 3
		return NULL;
#else
		return;
#endif

#if PY_MAJOR_VERSION >= 3
	m = PyModule_Create(&moduledef);
	PyObject *version = PyUnicode_FromString(_VERSION_);
#else
	m = Py_InitModule3("spidev", SpiDev_module_methods, SpiDev_module_doc);
	PyObject *version = PyString_FromString(_VERSION_);
#endif

	PyObject *dict = PyModule_GetDict(m);
	PyDict_SetItemString(dict, "__version__", version);
	Py_DECREF(version);

	Py_INCREF(&SpiDevObjectType);
	PyModule_AddObject(m, "SpiDev", (PyObject *)&SpiDevObjectType);

#if PY_MAJOR_VERSION >= 3
	return m;
#endif
}
