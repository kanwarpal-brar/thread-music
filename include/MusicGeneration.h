#ifndef THREAD_MUSIC_MUSIC_GENERATION_H
#define THREAD_MUSIC_MUSIC_GENERATION_H

#include <vector>
#include <mutex>
#include <atomic>
#include "Types.h"
#include "Constants.h"

/**
 * Creates a MIDI note pitch within a specified scale
 * 
 * @param scale Vector of scale intervals (semitones from root)
 * @param octave Base octave number
 * @param scaleIndex Position within the scale
 * @param rootNote Root note pitch class (0-11, where 0 is C)
 * @return MIDI note number
 */
int createNoteInScale(const std::vector<int>& scale, int octave, int scaleIndex, int rootNote);

/**
 * Generates a musical snippet for a thread based on its register and role
 * 
 * @param lowNote Lower bound of the note range
 * @param highNote Upper bound of the note range
 * @param scale Musical scale to use for note selection
 * @param rootNote Root note of the scale
 * @param isBass Whether this snippet is for a bass instrument
 * @return Snippet containing a sequence of musical notes
 */
Snippet generateSnippet(int lowNote, int highNote, const std::vector<int>& scale, int rootNote, bool isBass);

/**
 * Generates a drum pattern appropriate for a specific musical phase
 * 
 * @param phaseNumber Current musical phase (affects pattern complexity)
 * @return DrumPattern with kick, snare, and hi-hat patterns
 */
DrumPattern generateDrumPattern(int phaseNumber);

/**
 * Thread function for the drum/rhythm thread
 * 
 * @param data Thread configuration data
 * @param durationSec Total duration in seconds
 * @param numPhases Number of musical phases
 */
void drumThreadFunction(ThreadData data, int durationSec, int numPhases);

/**
 * Thread function for melodic instrument threads
 * 
 * @param data Thread configuration data
 * @param durationSec Total duration in seconds
 * @param numPhases Number of musical phases
 */
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases);

// External declarations for synchronization
extern std::mutex midi_mutex;
extern std::atomic<bool> running;

#endif // THREAD_MUSIC_MUSIC_GENERATION_H