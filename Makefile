build: fake6502.c main.cpp
	gcc-8 -c fake6502.c -o fake6502.o
	g++-8 main.cpp -lSDL2 -lSDL2main -std=c++17 -lstdc++fs fake6502.o -o ./compiled/bin/r6k
	@echo "\nBuild successful. Launch Retro 6k by navigating to ./compiled/bin and them running ./r6k"

clean: fake6502.o
	rm fake6502.o
