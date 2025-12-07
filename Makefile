bplus_main_compile:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib -Wl,-rpath,./lib ./examples/bplus_main.c ./src/*.c -lbf -o ./build/bp_main -O2;

bplus_main_run: bplus_main_compile
	@echo " Running bf_main ..."
	rm -f *.db
	./build/bp_main

test: bplus_main_compile
	@echo " Compiling and running tests ..."
	rm -f *.db
	./build/bp_main

test_clean:
	@echo " Cleaning test files ..."
	rm -f *.db
	rm -f ./build/bp_main

test_full: test_clean bplus_main_compile test

