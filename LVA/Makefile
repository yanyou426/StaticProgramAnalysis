LLVM_CONFIG=llvm-config

CXX=clang++
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`

all: LVA.so

LVA.so: LVA.o
	$(CXX) -shared LVA.o -o LVA.so $(LDFLAGS)

LVA.o: LVA.cpp
	$(CXX) -c LVA.cpp -o LVA.o $(CXXFLAGS)

clean:
	rm -f *.o *.so

# first: make
# second: opt -load ./LVA.so -LVA  < ../tests/sum.bc > /dev/null 2>../tests/sum_output_LVA.txt
