# Thread Music Generator

## Overview
This project generates MIDI music by sonifying CPU thread scheduling. It translates operating system thread scheduling into musical patterns, with each thread contributing musical phrases when scheduled by the CPU.

## How It Works

### Core Concept
The system runs multiple threads that compete for CPU resources. Each thread detects when it is being scheduled by comparing CPU time with wall clock time, and plays musical notes only during those periods. This creates a dynamic musical composition that directly reflects the thread scheduling behavior of the operating system.

### Musical Structure
- **Phases**: Music is divided into distinct phases with different harmonic characteristics
- **Thread Roles**:
  - **Thread 0**: Dedicated drum thread for rhythmic structure
  - **Other Threads**: Melodic threads assigned to bass, mid-range, or lead roles
- **Instrument Assignment**:
  - Bass instruments: Provide harmonic foundation in low register
  - Mid-range instruments: Provide harmonic context in middle register
  - Lead instruments: Provide melodic interest in high register

### Technical Implementation
1. **Thread Scheduling Detection**: Threads compare CPU time with wall clock time to determine scheduling status
2. **MIDI Generation**: Uses the MidiFile library to create standard MIDI files
3. **Musical Logic**: 
   - Snippets of notes are generated for each melodic thread based on register and role
   - Drum patterns vary by phase for rhythmic interest
   - Each thread plays only when scheduled by the OS

## Usage

Compile with the provided Makefile:
```bash
make
```

Run with default parameters:
```bash
./thread_music
```

Run with custom options:
```bash
./thread_music -n 6 -t 30 -p 5
```

### Command-line Options
- `-n, --num-threads`: Number of threads to create (default: 4)
- `-t, --time`: Duration in seconds (default: 60)
- `-p, --phases`: Number of musical phases (default: 3)

## Project Structure
- `main.cpp`: Sets up thread configuration and starts thread execution
- `include/`: Header files
  - `Constants.h`: Musical and system constants
  - `Types.h`: Data structure definitions
  - `MusicGeneration.h`: Music generation function declarations
  - `Utils.h`: Utility function declarations
- `src/`: Source implementations
  - `music/MusicGeneration.cpp`: Music generation and thread functions
  - `utils/Utils.cpp`: Utility function implementations
- `external/midifile/`: Third-party MIDI file library

## Output
The program produces a MIDI file named:
```
thread_music_[num_threads]threads_[duration]sec_[phases]phases_[timestamp].mid
```

The output can be played with any MIDI-compatible software or hardware.

## Musical Logic

### Scales and Harmony
- Each phase uses different musical scales (major, minor, pentatonic)
- Root notes change between phases for harmonic progression
- Bass parts emphasize root notes and fifths for stability

### Rhythm
- Drum thread provides consistent rhythmic foundation
- Phases use different drum patterns (standard, syncopated, half-time)
- Melodic thread phases align to bar boundaries for smoother transitions