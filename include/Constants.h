#ifndef THREAD_MUSIC_CONSTANTS_H
#define THREAD_MUSIC_CONSTANTS_H

#include <vector>
#include <map>

// MIDI and Musical Configuration
const int TPQ = 480;          // Ticks Per Quarter note - higher values enable more precise timing
const int TEMPO = 160;        // Beats Per Minute - controls playback speed
const int BEATS_PER_BAR = 4;  // 4/4 time signature
const int BARS_PER_PHASE = 4; // Musical structure: each phase consists of 4 bars

// Note Duration Weights - higher values increase probability of selection
// Controls rhythmic density in different musical parts

// Melodic Parts - varied note durations for melodic interest
const std::map<int, double> MELODY_DURATION_WEIGHTS = {
    {TPQ / 4, 1.0}, // Sixteenth notes
    {TPQ / 2, 2.0}, // Eighth notes
    {TPQ,     2.0}, // Quarter notes
    {TPQ * 2, 1.0}  // Half notes
};

// Bass Parts - longer durations for harmonic stability
const std::map<int, double> BASS_DURATION_WEIGHTS = {
    {TPQ / 2, 1.0}, // Eighth notes
    {TPQ,     2.0}  // Quarter notes
};

// Thread Scheduling Parameters
const double SCHEDULE_THRESHOLD = 0.001; // CPU/wall time ratio for schedule detection
const int THREAD_SLEEP_MS = 1;           // Thread sleep duration in milliseconds

// Thread work parameters - controls CPU load simulation
const int BUSY_WORK_MIN = 500;   // Minimum work iterations
const int BUSY_WORK_MAX = 10000; // Maximum work iterations

// MIDI note ranges - defines instrument register boundaries
const int BASS_LOW = 36;     // C2
const int BASS_HIGH = 48;    // C3
const int MID_LOW = 48;      // C3
const int MID_HIGH = 60;     // C4
const int HIGH_LOW = 60;     // C4
const int HIGH_HIGH = 72;    // C5

// Standard MIDI drum note numbers
const int KICK = 36;        // Bass Drum
const int SNARE = 38;       // Acoustic Snare
const int CLOSED_HAT = 42;  // Closed Hi-Hat
const int OPEN_HAT = 46;    // Open Hi-Hat
const int CRASH = 49;       // Crash Cymbal

// Musical scales - semitone patterns from root note
const std::vector<int> MAJOR_SCALE = {0, 2, 4, 5, 7, 9, 11}; // C major
const std::vector<int> MINOR_SCALE = {0, 2, 3, 5, 7, 8, 10}; // C minor
const std::vector<int> PENTA_SCALE = {0, 2, 4, 7, 9};        // C pentatonic

// Root notes for different phases - creates harmonic progression
const std::vector<int> PHASE_ROOTS = {0, 7, 5, 2}; // C, G, F, D

#endif // THREAD_MUSIC_CONSTANTS_H