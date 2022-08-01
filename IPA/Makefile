
output 			= 	draw_callgraph
output_bitcode		=	test.bc
target_code		=	test.c

run:
	./$(output) $(output_bitcode)

#dot -Tpng graph.dot > output.png

all: clean foo
	clang++ -std=c++11 main.cpp -o $(output) \
	`llvm-config --cxxflags --libs --ldflags --system-libs`

foo:
	clang -S -emit-llvm $(target_code) -o $(output_bitcode) 

clean:
	rm -f $(output) *.ll *.bc *.dot *.png

# clang -S -emit-llvm -o $(output_bitcode) -c $(target_code)