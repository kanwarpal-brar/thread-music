#ifndef THREAD_MUSIC_MUSIC_GENERATION_H
#define THREAD_MUSIC_MUSIC_GENERATION_H

#include <vector>
#include <mutex>
#include <atomic>
#include "Types.h"
#include "Constants.h"

// Create a note in a specific scale
int createNoteInScale(const std::vector<int>& scale, int octave, int scaleIndex, int rootNote);

// Generate a snippet for a thread based on range and scale
Snippet generateSnippet(int lowNote, int highNote, const std::vector<int>& scale, int rootNote, bool isBass);

// Generate a drum pattern for a specific phase
DrumPattern generateDrumPattern(int phaseNumber);

// Thread function declarations
void drumThreadFunction(ThreadData data, int durationSec, int numPhases);
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases);

// External declarations for globals
extern std::mutex midi_mutex;
extern std::atomic<bool> running;

#endif // THREAD_MUSIC_MUSIC_GENERATION_H