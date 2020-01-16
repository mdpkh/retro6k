build: ./src/fake6502.c ./src/main.cpp
	@echo "creating ./compiled/bin"
	@mkdir ./compiled/bin 2> /dev/null || echo "./compiled/bin already exists"
	gcc-8 -c ./src/fake6502.c -o ./src/fake6502.o
	g++-8 ./src/main.cpp -lSDL2 -lSDL2main -std=c++17 -lstdc++fs ./src/fake6502.o -o ./compiled/bin/r6k
	rm ./src/fake6502.o
	@echo "\nBuild successful. Launch Retro 6k by navigating to ./compiled/bin and them running ./r6k"
