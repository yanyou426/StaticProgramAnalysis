LLVM_CONFIG=llvm-config

CXX=clang++
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`

all: PtrA.so test.so

test.so: test.o
	$(CXX) -shared test.o -o test.so $(LDFLAGS)

test.o: test.cpp
	$(CXX) -c test.cpp -o test.o $(CXXFLAGS)

PtrA.so: PtrA.o
	$(CXX) -shared PtrA.o -o PtrA.so $(LDFLAGS)

PtrA.o: PtrA.cpp
	$(CXX) -c PtrA.cpp -o PtrA.o $(CXXFLAGS)

clean:
	rm -f *.o *.so

# first: make
# second: opt -load ./PtrA.so -PtrA  < ./test/test1.bc > /dev/null 2>./result/test1_output_PtrA.txt
# second: opt -load ./test.so -test  < ./test/test{2-9}.bc > /dev/null 2>./result/test{2-9}_output_test.txt
