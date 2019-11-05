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

gt4py_gt/computation.cpp: gt4py_gt/computation.cpp.in gt4py_gt/gen.py
	python gt4py_gt/gen.py gt4py_gt/computation.cpp.in gt4py_gt/computation.cpp
	clang-format -i gt4py_gt/computation.cpp
gt4py_gt/computation.hpp: gt4py_gt/computation.hpp.in gt4py_gt/gen.py
	python gt4py_gt/gen.py gt4py_gt/computation.hpp.in gt4py_gt/computation.hpp
	clang-format -i gt4py_gt/computation.hpp
gt4py_gt/bindings.cpp: gt4py_gt/bindings.cpp.in gt4py_gt/gen.py
	python gt4py_gt/gen.py gt4py_gt/bindings.cpp.in gt4py_gt/bindings.cpp
	clang-format -i gt4py_gt/bindings.cpp
gt4py_gt_bindings.o: gt4py_gt/bindings.cpp gt4py_gt/computation.hpp
	$(CC) -o gt4py_gt_bindings.o gt4py_gt/bindings.cpp -std=c++14 $(INCLUDES) -c -fPIC
gt4py_gt_computation.o: gt4py_gt/computation.cpp gt4py_gt/computation.hpp
	$(CC) -o gt4py_gt_computation.o gt4py_gt/computation.cpp -std=c++14 $(INCLUDES) -c -fPIC
gt4py_gt_computation.so: gt4py_gt_computation.o gt4py_gt_bindings.o
	$(CC) -shared -o gt4py_gt_computation.so gt4py_gt_computation.o gt4py_gt_bindings.o -l${PYTHON_LIB} -fPIC

gt4py_dawn_bindings.o: gt4py_dawn/bindings.cpp gt4py_dawn/computation.hpp
	$(CC) -o gt4py_dawn_bindings.o gt4py_dawn/bindings.cpp -std=c++14 $(INCLUDES) -c -fPIC
gt4py_dawn_computation.o: gt4py_dawn/computation.cpp gt4py_dawn/computation.hpp
	$(CC) -o gt4py_dawn_computation.o gt4py_dawn/computation.cpp -std=c++14 $(INCLUDES) -I$(GTCLANG_PATH) -c -fPIC
gt4py_dawn_computation.so: gt4py_dawn_computation.o gt4py_dawn_bindings.o
	$(CC) -shared -o gt4py_dawn_computation.so gt4py_dawn_computation.o gt4py_dawn_bindings.o -l${PYTHON_LIB} -fPIC

generated/copy_stencil.hpp: generated/copy_stencil.in.hpp
	$(GTCLANG) -o generated/copy_stencil.hpp generated/copy_stencil.in.hpp -backend=c++-naive
dawn_copy.o: dawn_copy.cpp generated/copy_stencil.hpp
	$(CC) -o dawn_copy.o dawn_copy.cpp -I$(GTCLANG_PATH) $(INCLUDES) -std=c++11 -c -fPIC
dawn_copy.so: dawn_copy.o
	$(CC) -shared -o dawn_copy.so dawn_copy.o -l${PYTHON_LIB} -fPIC

generated/shift_stencil.hpp: generated/shift_stencil.in.hpp
	$(GTCLANG) -o generated/shift_stencil.hpp generated/shift_stencil.in.hpp -backend=c++-naive
dawn_shift.o: dawn_shift.cpp generated/shift_stencil.hpp
	$(CC) -o dawn_shift.o dawn_shift.cpp -I$(GTCLANG_PATH) $(INCLUDES) -std=c++11 -c -fPIC
dawn_shift.so: dawn_shift.o
	$(CC) -shared -o dawn_shift.so dawn_shift.o -l${PYTHON_LIB} -fPIC

.PHONY: test
test: gtcomputation.so test.py copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so gt4py_gt_computation.so gt4py_dawn_computation.so dawn_copy.so dawn_shift.so
	pytest -v test.py

.PHONY: clean
clean:
	rm -f gtcomputation.so copy_simple.so gtcomputation_kji.so gtboundary.so gtcomputation_struct.so gtcomputation.o copy_simple.o \
		gtcomputation_kji.o gtcomputation_struct.o gtboundary.o gtcomputation.cpp \
		gt4py_gt_bindings.o gt4py_gt_computation.o gt4py_gt_computation.so gt4py_gt/bindings.cpp gt4py_gt/computation.hpp gt4py_gt/computation.cpp \
		gt4py_dawn_bindings.o gt4py_dawn_computation.o gt4py_dawn_computation.so gt4py_dawn/bindings.cpp gt4py_dawn/computation.hpp gt4py_dawn/computation.cpp \
		dawn_copy.o dawn_copy.so dawn_shift.so dawn_shift.o generated/copy_stencil.hpp generated/shift_stencil.hpp
