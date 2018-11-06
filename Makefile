GT_PATH=/home/lukas/documents/work/gridtools/include
PYTHON_PATH=/usr/include/python3.7m
PYBIND11_PATH=/home/lukas/packages/pybind11/2.2.3/include
BOOST_PATH=/home/lukas/packages/boost/1.67/include

INCLUDES=-isystem $(PYTHON_PATH) -isystem $(GT_PATH) -isystem $(PYBIND11_PATH) -isystem $(BOOST_PATH)

all: test

gtcomputation_kji.o: gtcomputation_kji.cpp
	g++ -o gtcomputation_kji.o gtcomputation_kji.cpp -std=c++11 $(INCLUDES) -c -fPIC

gtcomputation_kji.so: gtcomputation_kji.o
	g++ -shared -o gtcomputation_kji.so gtcomputation_kji.o -lpython3.7m -fPIC

gtcomputation.cpp: gtcomputation.cpp.in gen_cpp.py
	python gen_cpp.py

gtcomputation.o: gtcomputation.cpp
	g++ -o gtcomputation.o gtcomputation.cpp -std=c++11 $(INCLUDES) -c -fPIC

gtcomputation.so: gtcomputation.o
	g++ -shared -o gtcomputation.so gtcomputation.o -lpython3.7m -fPIC

copy_simple.o: copy_simple.cpp
	g++ -o copy_simple.o copy_simple.cpp -std=c++11 $(INCLUDES) -c -fPIC

copy_simple.so: copy_simple.o
	g++ -shared -o copy_simple.so copy_simple.o -lpython3.7m -fPIC

gtboundary.o: gtboundary.cpp
	g++ -o gtboundary.o gtboundary.cpp -std=c++11 $(INCLUDES) -c -fPIC

gtboundary.so: gtboundary.o
	g++ -shared -o gtboundary.so gtboundary.o -lpython3.7m -fPIC

gtcomputation_struct.o: gtcomputation_struct.cpp
	g++ -o gtcomputation_struct.o gtcomputation_struct.cpp -std=c++11 $(INCLUDES) -c -fPIC

gtcomputation_struct.so: gtcomputation_struct.o
	g++ -shared -o gtcomputation_struct.so gtcomputation_struct.o -lpython3.7m -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so
	pytest -v test.py

.PHONY: clean
clean:
	rm -f gtcomputation.so copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so gtcomputation.o copy_simple.o gtcomputation_kji.o gtcomputation_struct.o gtboundary.o gtcomputation.cpp
