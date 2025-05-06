.PHONY: dist upload-test upload clean cleardir distclean

PYTHON ?= python
TWINE ?= twine

all:
	$(PYTHON) setup.py build

install:
	$(PYTHON) setup.py install

upload-test:
	$(TWINE) upload --repository-url https://test.pypi.org/legacy/ dist/*

upload:
	$(TWINE) upload dist/*

dist:
	$(PYTHON) setup.py sdist
	$(PYTHON) setup.py bdist_wheel

clean:
	$(PYTHON) setup.py clean
	rm -rf build dist

cleandir distclean: clean
	$(PYTHON) setup.py clean -a

