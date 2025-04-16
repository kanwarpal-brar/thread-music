# Thread Music Generator Makefile
# This Makefile compiles the Thread Music Generator project

# Compiler configuration
CXX = g++                       # C++ compiler
CXXFLAGS = -std=c++17 -Wall -O2 # Language standard, warnings, and optimization
LDFLAGS = -pthread              # Link with pthread library for thread support

# Include and library paths
INCLUDES = -I./external/midifile/include -I./include  # Header include paths
LIBPATHS = -L./external/midifile/lib                  # Library paths
LIBS = -lmidifile                                     # External MIDI library

# Source files
SOURCES = main.cpp src/music/MusicGeneration.cpp src/utils/Utils.cpp

# Output executable
EXECUTABLE = thread_music

# Default target
all: $(EXECUTABLE)

# Main program build rule
$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

# Clean up build artifacts
clean:
	rm -f $(EXECUTABLE)

.PHONY: all clean