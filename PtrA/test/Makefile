SRCS = test1.cpp test2.cpp test3.cpp test4.cpp test5.cpp test6.cpp test7.cpp test8.cpp test9.cpp
TARGETS = $(SRCS:%.cpp=%)
LLVM_CONFIG=-emit-llvm
CXX=clang++

all:
	@for target in $(TARGETS);\
	do \
		$(CXX) -c $(LLVM_CONFIG) $$target.cpp -o $$target.bc;\
	done

