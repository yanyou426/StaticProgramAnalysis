SRCS = sum.cpp easy.cpp exp.cpp
TARGETS = $(SRCS:%.cpp=%)
LLVM_CONFIG=-emit-llvm
CXX=clang++

all:
	@for target in $(TARGETS);\
	do \
		$(CXX) -c $(LLVM_CONFIG) $$target.cpp -o $$target.bc;\
	done

