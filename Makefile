./compiled/bin/r6k: ./intermed/main.o ./intermed/fake6502.o | ./compiled/bin
	@echo "Linking..."
	g++-8 ./intermed/main.o ./intermed/fake6502.o \
		-lSDL2 -lSDL2main -lstdc++fs \
		-o ./compiled/bin/r6k
	@echo "\nBuild successful. Launch Retro 6k by navigating to ./compiled/bin"
	@echo "and them running ./r6k"

./compiled/bin:
	@mkdir ./compiled/bin 2> /dev/null || echo > /dev/null

./intermed/main.o: ./src/main.cpp ./src/custombuild.h ./src/fake6502.h \
		./src/main.h ./src/strcpys.h ./src/sys.h | ./intermed
	@echo "Compiling main..."
	g++-8 -c ./src/main.cpp -std=c++17 -o ./intermed/main.o

./intermed/fake6502.o: ./src/fake6502.c | ./intermed
	@echo "Compiling fake6502..."
	gcc-8 -c ./src/fake6502.c -o ./intermed/fake6502.o

./intermed:
	@mkdir ./intermed 2> /dev/null || echo > /dev/null

clean:
	@rm -r ./intermed 2> /dev/null || echo > /dev/null

cleanall:
	@rm -r ./intermed 2> /dev/null || echo > /dev/null
	@rm -r ./compiled/bin 2> /dev/null || echo > /dev/null

install: ./compiled/bin/r6k ./compiled/rom/bankf.rom
	@echo "Sorry, we haven't yet figured out how to 'install' Retro 6k."

.PHONY: clean cleanall install

