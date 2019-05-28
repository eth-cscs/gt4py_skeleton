PYTHON_LIB=python3.7m
GT_PATH=/home/lukas/documents/work/gridtools5/include
PYTHON_PATH=/usr/include/python3.7m
PYBIND11_PATH=/home/lukas/packages/pybind11/2.2.3/include
BOOST_PATH=/home/lukas/packages/boost/1.67/include
CC=g++

INCLUDES=-isystem $(PYTHON_PATH) -isystem $(GT_PATH) -isystem $(PYBIND11_PATH) -isystem $(BOOST_PATH) -DNDEBUG

all: test

gtcomputation_kji.o: gtcomputation_kji.cpp
	$(CC) -o gtcomputation_kji.o gtcomputation_kji.cpp -std=c++14 $(INCLUDES) -c -fPIC

gtcomputation_kji.so: gtcomputation_kji.o
	$(CC) -shared -o gtcomputation_kji.so gtcomputation_kji.o -l${PYTHON_LIB} -fPIC

gtcomputation.cpp: gtcomputation.cpp.in gen_cpp.py
	python gen_cpp.py

gtcomputation.o: gtcomputation.cpp
	$(CC) -o gtcomputation.o gtcomputation.cpp -std=c++14 $(INCLUDES) -c -fPIC

gtcomputation.so: gtcomputation.o
	$(CC) -shared -o gtcomputation.so gtcomputation.o -l${PYTHON_LIB} -fPIC

copy_simple.o: copy_simple.cpp
	$(CC) -o copy_simple.o copy_simple.cpp -std=c++14 $(INCLUDES) -c -fPIC

copy_simple.so: copy_simple.o
	$(CC) -shared -o copy_simple.so copy_simple.o -l${PYTHON_LIB} -fPIC

gtboundary.o: gtboundary.cpp
	$(CC) -o gtboundary.o gtboundary.cpp -std=c++14 $(INCLUDES) -c -fPIC

gtboundary.so: gtboundary.o
	$(CC) -shared -o gtboundary.so gtboundary.o -l${PYTHON_LIB} -fPIC

gtcomputation_struct.o: gtcomputation_struct.cpp
	$(CC) -o gtcomputation_struct.o gtcomputation_struct.cpp -std=c++14 $(INCLUDES) -c -fPIC

gtcomputation_struct.so: gtcomputation_struct.o
	$(CC) -shared -o gtcomputation_struct.so gtcomputation_struct.o -l${PYTHON_LIB} -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so
	pytest -v test.py

.PHONY: clean
clean:
	rm -f gtcomputation.so copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so gtcomputation.o copy_simple.o gtcomputation_kji.o gtcomputation_struct.o gtboundary.o gtcomputation.cpp
