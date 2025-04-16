# Thread Music Generator

## Overview
This project generates MIDI music by simulating and sonifying CPU thread scheduling. It demonstrates how operating system thread scheduling can be translated into musical patterns, with each thread contributing musical phrases when it is scheduled by the CPU.

## How It Works

### Core Concept
The system runs multiple threads that compete for CPU resources. Using timing measurements, each thread detects when it is being scheduled by the operating system and plays musical notes only during those times. This creates a dynamic musical composition that directly reflects the thread scheduling behavior of your computer.

### Musical Structure
- **Phases**: The music is divided into distinct musical phases with different harmonic characteristics
- **Thread Roles**:
  - **Thread 0**: Dedicated drum thread that provides rhythmic structure
  - **Other Threads**: Melodic threads that play different musical snippets depending on their scheduling
- **Instrument Assignment**:
  - Bass instruments (low register)
  - Mid-range instruments (middle register)
  - Lead instruments (high register)

### Technical Implementation
1. **Thread Scheduling Detection**: Threads compare CPU time with wall clock time to determine when they're being scheduled
2. **MIDI Generation**: The project uses the MidiFile library to generate standard MIDI files
3. **Musical Logic**: 
   - Snippets of notes are generated for each melodic thread
   - Drum patterns are generated for the rhythm thread
   - Each thread plays its assigned musical content only when scheduled

## Usage

Compile the program:
```bash
g++ -o thread_music main.cpp src/music/MusicGeneration.cpp src/utils/Utils.cpp -Iexternal/midifile/include -Lexternal/midifile/lib -lmidifile
```

Run with default parameters:
```bash
./thread_music
```

Or customize with options:
```bash
./thread_music -n 6 -t 30 -p 5
```

### Command-line Options
- `-n, --num-threads`: Number of threads to create (default: 4)
- `-t, --time`: Duration in seconds (default: 60)
- `-p, --phases`: Number of musical phases (default: 3)

## Project Structure
- `main.cpp`: Main program that sets up thread configuration and starts thread execution
- `include/`: Header files
  - `Constants.h`: Musical and system constants
  - `Types.h`: Data structure definitions (Note, Snippet, DrumPattern, ThreadData)
  - `MusicGeneration.h`: Declarations for music generation functions
  - `Utils.h`: Utility function declarations
- `src/`: Source implementations
  - `music/`: Musical functionality
    - `MusicGeneration.cpp`: Implementation of music generation and thread functions
  - `utils/`: Helper functions
    - `Utils.cpp`: Utility function implementations
- `external/midifile/`: Third-party MIDI file library

## Output
The program produces a MIDI file with naming convention:
```
thread_music_[num_threads]threads_[duration]sec_[phases]phases.mid
```

You can play this file with any MIDI-compatible software or hardware.

## Musical Logic

### Scales and Harmony
- Each phase uses a different musical scale (major, minor, pentatonic)
- Root notes change between phases to create harmonic progression
- Bass parts emphasize root notes and fifths for harmonic stability

### Rhythm
- The drum thread provides consistent rhythmic foundation.
- Different phases use different drum patterns (standard, syncopated, half-time).
- Pattern complexity varies to create musical interest.
- Melodic thread phases are aligned to bar boundaries for smoother transitions, while the drum track's phase timing follows the overall duration division.

### Thread Roles
1. **Thread 0**: Percussion (MIDI channel 10/9) - provides rhythmic foundation
2. **1/3 of remaining threads**: Bass instruments - provide harmonic foundation
3. **1/3 of remaining threads**: Mid-range instruments - provide harmonic context
4. **1/3 of remaining threads**: Lead instruments - provide melodic interest