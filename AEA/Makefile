LLVM_CONFIG=llvm-config

CXX=clang++
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`

all: AEA.so

AEA.so: AEA.o
	$(CXX) -shared AEA.o -o AEA.so $(LDFLAGS)

AEA.o: AEA.cpp
	$(CXX) -c AEA.cpp -o AEA.o $(CXXFLAGS)

clean:
	rm -f *.o *.so

# first: make
# second: opt -load ./AEA.so -AEA  < ../tests/exp.bc > /dev/null 2>../tests/exp_output_AEA.txt
