BINARY=r6k
OUTPUT_FOLDER=./compiled/bin
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
	@mkdir -p $(OUTPUT_FOLDER)
$(OUTPUT_FOLDER)/src:$(OUTPUT_FOLDER)
	@mkdir -p $(OUTPUT_FOLDER)/src

cleanall:
	$(RM) -r $(OUTPUT_FOLDER) 2> /dev/null
clean:
	$(RM) -r $(OUTPUT_FOLDER)/src 2> /dev/null

.PHONY: clean cleanall run

