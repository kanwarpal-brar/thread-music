#include "../../include/MusicGeneration.h"
#include "../../include/Utils.h"
#include "../../include/Constants.h"
#include <random>
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <vector>
#include <MidiFile.h>

using namespace smf;

// Global variables for thread synchronization
std::mutex midi_mutex;
std::atomic<bool> running(true);
extern MidiFile midifile;  // Defined in main.cpp

/**
 * Creates a note within a musical scale at a specific octave and position
 * 
 * @param scale Vector of scale intervals (semitones from root)
 * @param octave Base octave number
 * @param scaleIndex Position within the scale
 * @param rootNote Root note pitch class (0-11, where 0 is C)
 * @return MIDI note number
 */
int createNoteInScale(const std::vector<int>& scale, int octave, int scaleIndex, int rootNote) {
    return (octave * 12) + rootNote + scale[scaleIndex % scale.size()];
}

/**
 * Helper function to select a note duration based on weighted probabilities
 * 
 * @param gen Random number generator
 * @param weights Map of duration values to their probability weights
 * @return Selected duration value in MIDI ticks
 */
int selectDuration(std::mt19937& gen, const std::map<int, double>& weights) {
    std::vector<double> weightValues;
    std::vector<int> durationValues;
    for (const auto& pair : weights) {
        if (pair.second > 0) { // Only include durations with positive weight
            durationValues.push_back(pair.first);
            weightValues.push_back(pair.second);
        }
    }

    if (durationValues.empty()) {
        return TPQ; // Default to quarter note if no valid weights
    }

    std::discrete_distribution<> dist(weightValues.begin(), weightValues.end());
    return durationValues[dist(gen)];
}

/**
 * Generates a musical snippet (phrase) for a thread based on its range and role
 * 
 * @param lowNote Lower bound of note range
 * @param highNote Upper bound of note range
 * @param scale Musical scale to use
 * @param rootNote Root note of the scale
 * @param isBass Whether this snippet is for a bass instrument
 * @return Snippet containing a sequence of musical notes
 */
Snippet generateSnippet(int lowNote, int highNote, const std::vector<int>& scale, int rootNote, bool isBass) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Generate snippet with consistent length for musical coherence
    std::uniform_int_distribution<> lenDist(4, 8);
    int length = lenDist(gen);
    
    Snippet snippet;
    int lowOctave = lowNote / 12;
    int highOctave = highNote / 12;
    
    // For melodic instruments: create directional musical phrases
    if (!isBass) {
        // Start in the middle of the range for melodic interest
        int octave = lowOctave + (highOctave - lowOctave) / 2;
        int scaleIndex = gen() % scale.size();
        
        for (int i = 0; i < length; i++) {
            Note note;
            note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            
            // Ensure note stays within the specified range
            while (note.pitch < lowNote) {
                octave++;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            while (note.pitch > highNote) {
                octave--;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            
            // Create melodic contour: ascending first half, descending second half
            int direction = (i < length/2) ? 1 : -1;
            scaleIndex += direction;
            
            // Occasionally add larger intervals for melodic interest
            if (gen() % 4 == 0) {
                scaleIndex += direction * 2;
            }
            
            // Handle scale boundaries with octave changes when needed
            if (scaleIndex < 0) {
                scaleIndex += scale.size();
                if (octave > lowOctave) octave--;
            }
            if (scaleIndex >= (int)scale.size()) {
                scaleIndex -= scale.size();
                if (octave < highOctave) octave++;
            }
            
            // Select duration using weighted distribution
            note.duration = selectDuration(gen, MELODY_DURATION_WEIGHTS);
            
            // Varied velocity for expressive dynamics
            std::uniform_int_distribution<> velDist(80, 110);
            note.velocity = velDist(gen);
            
            snippet.notes.push_back(note);
        }
    } 
    // For bass instruments: create simpler, harmonically focused patterns
    else {
        // Start at the root note for harmonic stability
        int octave = lowOctave;

        for (int i = 0; i < length; i++) {
            Note note;
            
            // Bass emphasizes root, fifth, and other chord tones
            int scaleIndex;
            if (i % 4 == 0) scaleIndex = 0;                  // Root note
            else if (i % 4 == 2) scaleIndex = 4 % scale.size(); // Fifth if available
            else scaleIndex = gen() % scale.size();              // Other scale tones
            
            note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            
            // Ensure note stays within the specified range
            while (note.pitch < lowNote) {
                octave++;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            while (note.pitch > highNote) {
                octave--;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            
            // Longer note durations for bass parts
            note.duration = selectDuration(gen, BASS_DURATION_WEIGHTS);
            
            // Consistent velocity for bass stability
            note.velocity = 100;
            
            snippet.notes.push_back(note);
        }
    }
    
    return snippet;
}

/**
 * Generates a drum pattern appropriate for a specific musical phase
 * 
 * @param phaseNumber Current musical phase (affects pattern complexity)
 * @return DrumPattern with kick, snare, and hi-hat patterns
 */
DrumPattern generateDrumPattern(int phaseNumber) {
    DrumPattern pattern;
    
    // Vary drum pattern based on the musical phase
    switch (phaseNumber % 3) {
        case 0: // Basic rock pattern
            // Kick on beats 1 and 3
            pattern.kick[0] = true;
            pattern.kick[8] = true;
            
            // Snare on beats 2 and 4
            pattern.snare[4] = true;
            pattern.snare[12] = true;
            
            // Hi-hat on eighth notes for steady pulse
            for (int i = 0; i < 16; i += 2) {
                pattern.hihat[i] = true;
            }
            break;
            
        case 1: // Syncopated pattern for rhythmic interest
            // Kick on beat 1, 2-and, and 4
            pattern.kick[0] = true;
            pattern.kick[6] = true;
            pattern.kick[12] = true;
            
            // Snare on beat 2 and 3-and
            pattern.snare[4] = true;
            pattern.snare[10] = true;
            
            // Hi-hat on quarter notes for open feel
            pattern.hihat[0] = true;
            pattern.hihat[4] = true;
            pattern.hihat[8] = true;
            pattern.hihat[12] = true;
            break;
            
        case 2: // Half-time feel for contrast
            // Kick on beats 1 and 3
            pattern.kick[0] = true;
            pattern.kick[8] = true;
            
            // Snare only on beat 3 for half-time feel
            pattern.snare[8] = true;
            
            // Hi-hat on downbeats only
            for (int i = 0; i < 16; i += 4) {
                pattern.hihat[i] = true;
            }
            break;
    }
    
    // Set dynamic accents for rhythmic expression
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) {
            pattern.velocities[i] = 110; // Strong accent on downbeats
        } else if (i % 2 == 0) {
            pattern.velocities[i] = 90;  // Medium accent on upbeats
        } else {
            pattern.velocities[i] = 70;  // Soft velocity on off-beats
        }
    }
    
    return pattern;
}

/**
 * Thread function for the dedicated drum/rhythm thread
 * 
 * Provides steady rhythmic foundation and establishes phase transitions
 * 
 * @param data Thread configuration data
 * @param durationSec Total duration in seconds
 * @param numPhases Number of musical phases
 */
void drumThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // Phase length calculations - drum phases aren't aligned to bar boundaries
    int ticksPerPhase = (numPhases > 0) ? static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0)) / numPhases) : static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0)));
    if (durationSec > 0 && ticksPerPhase <= 0) ticksPerPhase = 1;
    int totalTicks = static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0)));

    // Calculate musical grid divisions
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    int ticksPerStep = ticksPerBar / 4; // 16 steps per bar (16th notes)

    // Initialize timing
    auto startWallTime = std::chrono::high_resolution_clock::now();
    int currentPhase = -1;

    // Add track metadata
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Drum Track");
    }

    // Loop state variables
    int currentTick = 0;
    int currentStep = -1; // Start at -1 so first comparison triggers

    // Random number generation for thread activity simulation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);

    // Main timing loop
    while (running) {
        // Update timing
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;

        // Check if finished
        if (currentWallTime >= durationSec) {
            break;
        }

        // Calculate current musical position
        currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));

        // Handle phase transitions
        int newPhase = (ticksPerPhase > 0) ? (currentTick / ticksPerPhase) : 0;
        if (newPhase >= numPhases) newPhase = numPhases - 1;

        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Mark phase transition in MIDI file
            int phaseEventTick = newPhase * ticksPerPhase;
            midifile.addMarker(data.track, phaseEventTick, "Phase " + std::to_string(newPhase + 1));

            // Add crash cymbal at phase transitions for musical emphasis
            midifile.addNoteOn(data.track, phaseEventTick, 9, CRASH, 110);
            midifile.addNoteOff(data.track, phaseEventTick + std::max(1, ticksPerStep), 9, CRASH);

            currentPhase = newPhase;
        }

        // Calculate rhythmic grid position
        int stepPosition = (ticksPerStep > 0) ? (currentTick / ticksPerStep) % 16 : 0;

        // Add drum notes when moving to a new step
        if (stepPosition != currentStep) {
            currentStep = stepPosition;

            // Ensure valid phase
            if (currentPhase < 0) currentPhase = 0;
            const DrumPattern& pattern = data.drumPatterns[currentPhase % data.drumPatterns.size()];

            // Quantize timing to grid
            int stepTick = (ticksPerStep > 0) ? (currentTick / ticksPerStep) * ticksPerStep : currentTick;

            // Add drum hits based on pattern
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Add kick drum if pattern indicates
            if (pattern.kick[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, KICK, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, KICK);
            }

            // Add snare drum if pattern indicates
            if (pattern.snare[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, SNARE, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, SNARE);
            }

            // Add hi-hat if pattern indicates
            if (pattern.hihat[stepPosition]) {
                // Open hi-hat on strong beats, closed on others
                int hihat = (stepPosition % 8 == 0) ? OPEN_HAT : CLOSED_HAT;
                midifile.addNoteOn(data.track, stepTick, 9, hihat, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, hihat);
            }
        }

        // Simulate CPU work to trigger scheduling events
        int busyWorkAmount = busyWorkDist(gen);
        volatile double sum = 0;
        for (int i = 0; i < busyWorkAmount; i++) {
            sum += sin(i) * cos(i);
        }

        // Sleep to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }

    // Add final marker
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        int originalEndTick = totalTicks;
        int finalMarkerTick = std::max(currentTick, originalEndTick);
        midifile.addMarker(data.track, finalMarkerTick, "Original End");
    }
}

/**
 * Thread function for melodic instrument threads
 * 
 * Plays musical phrases when the thread is scheduled by the OS
 * Each thread detects its own scheduling and plays notes accordingly
 * 
 * @param data Thread configuration data
 * @param durationSec Total duration in seconds
 * @param numPhases Number of musical phases
 */
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // Phase calculations with bar alignment for musical coherence
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    double initialTotalTicks = durationSec * (TPQ * (TEMPO / 60.0));
    double initialTicksPerPhase = (numPhases > 0) ? initialTotalTicks / numPhases : initialTotalTicks;
    
    // Align phase boundaries to complete bars
    int adjustedTicksPerPhase = 0;
    if (ticksPerBar > 0 && initialTicksPerPhase > 0) {
        adjustedTicksPerPhase = static_cast<int>(std::round(initialTicksPerPhase / static_cast<double>(ticksPerBar))) * ticksPerBar;
        // Ensure minimum phase length of one bar
        if (adjustedTicksPerPhase == 0) adjustedTicksPerPhase = ticksPerBar;
    } else {
        adjustedTicksPerPhase = static_cast<int>(initialTicksPerPhase);
    }
    if (adjustedTicksPerPhase < 0) adjustedTicksPerPhase = 0;

    // Calculate adjusted total duration
    int adjustedTotalTicks = adjustedTicksPerPhase * numPhases;
    double adjustedDurationSec = (TPQ > 0 && TEMPO > 0) ? adjustedTotalTicks / (TPQ * (TEMPO / 60.0)) : 0.0;

    // Time tracking variables
    auto startWallTime = std::chrono::high_resolution_clock::now();
    double lastWallTime = 0;
    double lastCpuTime = getCpuTime();
    int currentPhase = -1;

    // Add track metadata
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Thread " + std::to_string(data.id));
    }

    // Random number generation for thread activity simulation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);

    // Thread state variables
    bool wasScheduled = false;
    bool noteIsOn = false;
    int currentNote = -1;
    int noteStartTick = 0;
    int noteDuration = 0;
    int currentTick = 0;

    // Main timing loop
    while (running) {
        // Update timing
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
        double currentCpuTime = getCpuTime();

        // Check if finished
        if (currentWallTime >= adjustedDurationSec) {
            // Clean up any active notes
            if (noteIsOn) {
                std::lock_guard<std::mutex> lock(midi_mutex);
                int endTick = adjustedTotalTicks;
                midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
            }
            break;
        }

        // Calculate time deltas
        double wallTimeDelta = currentWallTime - lastWallTime;
        double cpuTimeDelta = currentCpuTime - lastCpuTime;

        // Calculate thread scheduling ratio
        double schedulingRatio = (wallTimeDelta > 0) ? cpuTimeDelta / wallTimeDelta : 0;

        // Detect if thread is being scheduled by OS
        bool isScheduled = (schedulingRatio > SCHEDULE_THRESHOLD);

        // Calculate current musical position
        currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));

        // Handle phase transitions
        int newPhase = (adjustedTicksPerPhase > 0) ? (currentTick / adjustedTicksPerPhase) : 0;
        if (newPhase >= numPhases) newPhase = numPhases - 1;

        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Mark phase transition in MIDI file
            int phaseEventTick = newPhase * adjustedTicksPerPhase;

            // End any active note at phase boundary
            if (noteIsOn) {
                int endTick = std::max(noteStartTick, phaseEventTick);
                midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
                noteIsOn = false;
                wasScheduled = false;
            }

            // Add phase marker
            midifile.addMarker(data.track, phaseEventTick, "Phase " + std::to_string(newPhase + 1));

            currentPhase = newPhase;

            // Reset snippet to start of phrase at phase change
            if (currentPhase >= 0 && currentPhase < data.snippets.size()) {
                Snippet& snippetToReset = data.snippets[currentPhase % data.snippets.size()];
                snippetToReset.reset();
            }
        }

        // Handle scheduling state changes
        if (isScheduled != wasScheduled) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            if (isScheduled) {
                // Thread just became scheduled - start playing a note
                int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
                if (!noteIsOn && currentPhase >= 0 && currentPhase < data.snippets.size() && 
                   (adjustedTicksPerPhase == 0 || currentTick < nextPhaseTick)) {
                    Snippet& snippet = data.snippets[currentPhase % data.snippets.size()];
                    Note* note = snippet.getNextNote();

                    if (note) {
                        currentNote = note->pitch;
                        noteDuration = note->duration;
                        midifile.addNoteOn(data.track, currentTick, data.channel, currentNote, note->velocity);
                        noteStartTick = currentTick;
                        noteIsOn = true;
                    }
                }
            } else {
                // Thread just became descheduled - stop playing note
                if (noteIsOn) {
                    int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
                    int endTick = (adjustedTicksPerPhase > 0 && currentTick >= nextPhaseTick) ? nextPhaseTick : currentTick;
                    midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
                    noteIsOn = false;
                }
            }

            wasScheduled = isScheduled;
        }
        // Continue to next note if current note has finished its duration
        else if (isScheduled && noteIsOn && currentTick - noteStartTick >= noteDuration) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            int intendedEndTick = noteStartTick + noteDuration;
            int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
            int endTick = (adjustedTicksPerPhase > 0 && intendedEndTick >= nextPhaseTick) ? 
                          nextPhaseTick : intendedEndTick;

            midifile.addNoteOff(data.track, endTick, data.channel, currentNote);

            // Start next note if still within current phase
            if (adjustedTicksPerPhase == 0 || endTick < nextPhaseTick) {
                if (currentPhase >= 0 && currentPhase < data.snippets.size()) {
                    Snippet& snippet = data.snippets[currentPhase % data.snippets.size()];
                    Note* note = snippet.getNextNote();

                    if (note) {
                        currentNote = note->pitch;
                        noteDuration = note->duration;
                        midifile.addNoteOn(data.track, endTick, data.channel, currentNote, note->velocity);
                        noteStartTick = endTick;
                        noteIsOn = true;
                    } else {
                        noteIsOn = false;
                    }
                } else {
                    noteIsOn = false;
                }
            } else {
                noteIsOn = false;
            }
        }

        // Update timing values for next iteration
        lastWallTime = currentWallTime;
        lastCpuTime = currentCpuTime;

        // Simulate CPU work to trigger scheduling events
        int busyWorkAmount = busyWorkDist(gen);
        volatile double sum = 0;
        for (int i = 0; i < busyWorkAmount; i++) {
            sum += sin(i) * cos(i);
        }

        // Sleep to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }

    // Add final marker
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        int alignedEndTick = adjustedTotalTicks;
        int finalMarkerTick = std::max(currentTick, alignedEndTick);
        midifile.addMarker(data.track, finalMarkerTick, "Aligned End");
    }
}