BINARY=rk6
OUTPUT_FOLDER=./bin
SOURCES=src/main.cpp src/fake6502.c
HEADERS=src/custombuild.h src/fake6502.h
OBJECTS=$(addprefix $(OUTPUT_FOLDER)/,$(addsuffix .o, $(basename $(basename $(SOURCES)))))
LIBRARIES=SDL2 SDL2main stdc++fs
LIBFLAGS=$(addprefix -l,$(LIBRARIES))
CXXFLAGS=-std=c++17

default:$(OUTPUT_FOLDER)/$(BINARY)
$(OUTPUT_FOLDER)/$(BINARY): $(OBJECTS) | $(OUTPUT_FOLDER)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LIBFLAGS) -o $@

run: $(OUTPUT_FOLDER)/$(BINARY)
	@cd $(OUTPUT_FOLDER) && ./$(BINARY)

$(OUTPUT_FOLDER)/src/%.o: src/%.cpp | $(OUTPUT_FOLDER)/src
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/src/%.o: src/%.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER):
	@if [[ ! -e $(OUTPUT_FOLDER) ]];then mkdir $(OUTPUT_FOLDER);fi
$(OUTPUT_FOLDER)/src:$(OUTPUT_FOLDER)
	@if [[ ! -e $(OUTPUT_FOLDER)/src ]];then mkdir $(OUTPUT_FOLDER)/src;fi

clean:
	$(RM) -r $(OUTPUT_FOLDER) 2> /dev/null

.PHONY: clean run

