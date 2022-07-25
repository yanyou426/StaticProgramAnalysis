LLVM_CONFIG=llvm-config

CXX=clang++
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`

all: RDA.so

RDA.so: RDA.o
	$(CXX) -shared RDA.o -o RDA.so $(LDFLAGS)

RDA.o: RDA.cpp
	$(CXX) -c RDA.cpp -o RDA.o $(CXXFLAGS)

clean:
	rm -f *.o *.so

# first: make
# second: opt -load ./RDA.so -RDA  < ../tests/sum.bc > /dev/null 2>../tests/sum_output_RDA.txt
