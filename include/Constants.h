#ifndef THREAD_MUSIC_CONSTANTS_H
#define THREAD_MUSIC_CONSTANTS_H

#include <vector>
#include <map> // Include map for duration weights

// Musical parameters
// TPQ (Ticks Per Quarter Note) defines the time resolution of the MIDI file.
// Higher values allow for more precise timing.
const int TPQ = 480;          // Ticks per quarter note (high resolution)

// TEMPO defines the intended playback speed in Beats Per Minute (BPM).
// IMPORTANT: This value MUST be written as a Tempo Meta Event (0xFF 0x51 0x03 tt tt tt)
// usually at the beginning (tick 0) of the MIDI file track 0 during generation.
// The midifile library function `MidiFile::addTempo(track, tick, tempo)` handles this.
// If the Tempo event is missing, MIDI players often default to 120 BPM.
const int TEMPO = 180;        // BPM (Reverted to original value)

// BEATS_PER_BAR and BARS_PER_PHASE define the musical structure.
// Note event timings (in ticks) must be calculated based on these and TPQ.
// E.g., a quarter note duration = TPQ ticks.
// E.g., a bar duration = BEATS_PER_BAR * TPQ ticks.
const int BEATS_PER_BAR = 4;  // 4/4 time signature
const int BARS_PER_PHASE = 4; // Each phase is 4 bars long

// Note Duration Weights (Higher value = more frequent)
// Used in generateSnippet to control rhythmic density.
// Adjust these values to change the probability of different note lengths.

// Melodic Parts
const std::map<int, double> MELODY_DURATION_WEIGHTS = {
    {TPQ / 4, 1.0}, // Sixteenth note weight
    {TPQ / 2, 2.0}, // Eighth note weight
    {TPQ,     2.0}, // Quarter note weight
    {TPQ * 2, 1.0}  // Half note weight
    // {TPQ * 4, 0.0} // Whole note weight (currently disabled)
};

// Bass Parts
const std::map<int, double> BASS_DURATION_WEIGHTS = {
    {TPQ / 2, 1.0}, // Eighth note weight
    {TPQ,     2.0}  // Quarter note weight
    // {TPQ * 2, 0.0} // Half note weight (currently disabled)
};

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