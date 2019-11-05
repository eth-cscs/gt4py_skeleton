include config.mk

INCLUDES=-isystem $(PYTHON_PATH) -isystem $(GT_PATH) -isystem $(PYBIND11_PATH) -isystem $(BOOST_PATH) -DNDEBUG

all: test

gtcomputation_kji.o: gtcomputation_kji.cpp
	$(CC) -o gtcomputation_kji.o gtcomputation_kji.cpp -std=c++14 $(INCLUDES) -c -fPIC

gtcomputation_kji.so: gtcomputation_kji.o
	$(CC) -shared -o gtcomputation_kji.so gtcomputation_kji.o -l${PYTHON_LIB} -fPIC

gtcomputation.cpp: gtcomputation.cpp.in gen_cpp.py
	python3 gen_cpp.py

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

copy_dawn.o: dawn.cpp generated/copystencil.hpp
	$(CC) -o copy_dawn.o dawn.cpp -I$(GTCLANG_PATH) $(INCLUDES) -std=c++11 -c -fPIC -O0 -g

copy_dawn.so: copy_dawn.o
	$(CC) -shared -o copy_dawn.so copy_dawn.o -l${PYTHON_LIB} -fPIC

shift_dawn.o: dawn_shift.cpp generated/shift.hpp
	$(CC) -o shift_dawn.o dawn_shift.cpp -I$(GTCLANG_PATH) $(INCLUDES) -std=c++11 -c -fPIC -O0 -g

shift_dawn.so: shift_dawn.o
	$(CC) -shared -o shift_dawn.so shift_dawn.o -l${PYTHON_LIB} -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so copy_dawn.so shift_dawn.so
	pytest-3 -v test.py

.PHONY: clean
clean:
	rm -f gtcomputation.so copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so gtcomputation.o \
	 copy_simple.o gtcomputation_kji.o gtcomputation_struct.o gtboundary.o gtcomputation.cpp copy_dawn.o copy_dawn.so shift_dawn.so shift_dawn.o
