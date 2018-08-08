all: test

gtcomputation.cpp: gtcomputation.cpp.in gen_cpp.py
	python gen_cpp.py

gtcomputation.o: gtcomputation.cpp
	g++ -o gtcomputation.o gtcomputation.cpp -std=c++11 -isystem /usr/include/python3.6m -isystem /home/lukas/documents/work/gridtools/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

gtcomputation.so: gtcomputation.o
	g++ -shared -o gtcomputation.so gtcomputation.o -lpython3.6m -fPIC

copy_simple.o: copy_simple.cpp
	g++ -o copy_simple.o copy_simple.cpp -std=c++11 -isystem /usr/include/python3.6m -isystem /home/lukas/documents/work/gridtools/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

copy_simple.so: copy_simple.o
	g++ -shared -o copy_simple.so copy_simple.o -lpython3.6m -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so
	pytest -v test.py

