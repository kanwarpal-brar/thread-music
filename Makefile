# Compiler
CXX = g++

# C++ Standard
CPP_STANDARD = -std=c++20

# Include paths
INCLUDES = -Iexternal/midifile/include

# Compiler flags (add -g for debugging if needed)
CXXFLAGS = -Wall -Wextra -pedantic $(CPP_STANDARD) $(INCLUDES) -O2

# Linker flags
LDFLAGS =

# Executable name
TARGET = music_generator

# --- Source Files ---
# Find all .cpp files in the current directory
PROJECT_SRCS = $(wildcard *.cpp)

# Add Midifile library source files needed for linking
# (Even if you only *use* headers, these implementations are required)
MIDIFILE_SRCS = external/midifile/src/MidiFile.cpp \
                external/midifile/src/MidiEvent.cpp \
                external/midifile/src/MidiEventList.cpp \
                external/midifile/src/MidiMessage.cpp \
                external/midifile/src/Binasc.cpp \
                external/midifile/src/Options.cpp

SRCS = $(PROJECT_SRCS) $(MIDIFILE_SRCS)

# Generate object file names from source files
OBJS = $(SRCS:.cpp=.o)

# --- Targets ---

# Default target: build the executable
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build finished: $@"

# Rule to compile source files into object files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target: remove object files and the executable
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean finished."

# Phony targets (targets that aren't actual files)
.PHONY: all clean