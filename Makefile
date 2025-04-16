# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -O2

# Linker flags
LDFLAGS = -pthread

# Include paths for midifile library
INCLUDES = -I./external/midifile/include

# Midifile library path
MIDILIB = ./external/midifile/lib/libmidifile.a

# Main executable
EXECUTABLE = thread_music

# Default target
all: midifile $(EXECUTABLE)

# Build midifile library
midifile:
	$(MAKE) -C ./external/midifile library

# Main program
$(EXECUTABLE): main.cpp midifile
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(MIDILIB) $(LDFLAGS) -o $@

# Clean up
clean:
	rm -f $(EXECUTABLE)
	$(MAKE) -C ./external/midifile clean

.PHONY: all clean midifile