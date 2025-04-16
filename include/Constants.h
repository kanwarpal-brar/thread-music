#ifndef THREAD_MUSIC_CONSTANTS_H
#define THREAD_MUSIC_CONSTANTS_H

#include <vector>

// Musical parameters
const int TPQ = 480;          // Ticks per quarter note (high resolution)
const int TEMPO = 200;        // BPM
const int BEATS_PER_BAR = 4;  // 4/4 time signature
const int BARS_PER_PHASE = 4; // Each phase is 4 bars long

// Schedule detection
const double SCHEDULE_THRESHOLD = 0.001; // CPU/wall time ratio to detect scheduling

// Thread sleep time (ms)
const int THREAD_SLEEP_MS = 1;

// Thread work parameters (to create CPU load)
const int BUSY_WORK_MIN = 500;
const int BUSY_WORK_MAX = 10000;

// MIDI note ranges
const int BASS_LOW = 36;     // C2
const int BASS_HIGH = 48;    // C3
const int MID_LOW = 48;      // C3
const int MID_HIGH = 60;     // C4
const int HIGH_LOW = 60;     // C4
const int HIGH_HIGH = 72;    // C5

// Drum notes
const int KICK = 36;        // Bass Drum
const int SNARE = 38;       // Acoustic Snare
const int CLOSED_HAT = 42;  // Closed Hi-Hat
const int OPEN_HAT = 46;    // Open Hi-Hat
const int CRASH = 49;       // Crash Cymbal

// Musical scales
const std::vector<int> MAJOR_SCALE = {0, 2, 4, 5, 7, 9, 11}; // C major
const std::vector<int> MINOR_SCALE = {0, 2, 3, 5, 7, 8, 10}; // C minor
const std::vector<int> PENTA_SCALE = {0, 2, 4, 7, 9};        // C pentatonic

// Root notes for different phases (in semitones from C)
const std::vector<int> PHASE_ROOTS = {0, 7, 5, 2}; // C, G, F, D

#endif // THREAD_MUSIC_CONSTANTS_H