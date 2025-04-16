# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -O2

# Linker flags
LDFLAGS = -pthread

# Include paths for header files
INCLUDES = -I./external/midifile/include -I./include

# Library paths
LIBPATHS = -L./external/midifile/lib

# Libraries to link
LIBS = -lmidifile

# Source files
SOURCES = main.cpp src/music/MusicGeneration.cpp src/utils/Utils.cpp

# Main executable
EXECUTABLE = thread_music

# Default target
all: $(EXECUTABLE)

# Main program
$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

# Clean up
clean:
	rm -f $(EXECUTABLE)

.PHONY: all clean