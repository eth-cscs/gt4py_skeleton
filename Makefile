all: test

gtcomputation_kji.o: gtcomputation_kji.cpp
	g++ -o gtcomputation_kji.o gtcomputation_kji.cpp -std=c++11 -isystem /usr/include/python3.7m -isystem /home/lukas/documents/work/gridtools3/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

gtcomputation_kji.so: gtcomputation_kji.o
	g++ -shared -o gtcomputation_kji.so gtcomputation_kji.o -lpython3.7m -fPIC

gtcomputation.cpp: gtcomputation.cpp.in gen_cpp.py
	python gen_cpp.py

gtcomputation.o: gtcomputation.cpp
	g++ -o gtcomputation.o gtcomputation.cpp -std=c++11 -isystem /usr/include/python3.7m -isystem /home/lukas/documents/work/gridtools3/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

gtcomputation.so: gtcomputation.o
	g++ -shared -o gtcomputation.so gtcomputation.o -lpython3.7m -fPIC

copy_simple.o: copy_simple.cpp
	g++ -o copy_simple.o copy_simple.cpp -std=c++11 -isystem /usr/include/python3.7m -isystem /home/lukas/documents/work/gridtools3/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

copy_simple.so: copy_simple.o
	g++ -shared -o copy_simple.so copy_simple.o -lpython3.7m -fPIC

gtboundary.o: gtboundary.cpp
	g++ -o gtboundary.o gtboundary.cpp -std=c++11 -isystem /usr/include/python3.7m -isystem /home/lukas/documents/work/gridtools3/include -isystem /home/lukas/packages/pybind11/2.2.3/include -isystem /home/lukas/packages/boost/1.67/include -c -fPIC

gtboundary.so: gtboundary.o
	g++ -shared -o gtboundary.so gtboundary.o -lpython3.7m -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so gtcomputation_kji.so gtboundary.so
	pytest -v test.py

