#include "../../include/MusicGeneration.h"
#include "../../include/Utils.h"
#include <random>
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <MidiFile.h>

using namespace smf;

// Global variables
std::mutex midi_mutex;
std::atomic<bool> running(true);
extern MidiFile midifile;  // Defined in main.cpp

// Create a note in a specific scale
int createNoteInScale(const std::vector<int>& scale, int octave, int scaleIndex, int rootNote) {
    return (octave * 12) + rootNote + scale[scaleIndex % scale.size()];
}

// Generate a snippet for a thread based on range and scale
Snippet generateSnippet(int lowNote, int highNote, const std::vector<int>& scale, int rootNote, bool isBass) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // More consistent snippet lengths
    std::uniform_int_distribution<> lenDist(4, 8);
    int length = lenDist(gen);
    
    Snippet snippet;
    int lowOctave = lowNote / 12;
    int highOctave = highNote / 12;
    
    // For melody: create a musical phrase with some direction
    if (!isBass) {
        // Start in the middle of the range
        int octave = lowOctave + (highOctave - lowOctave) / 2;
        int scaleIndex = gen() % scale.size();
        
        for (int i = 0; i < length; i++) {
            Note note;
            note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            
            // Make sure note is within range
            while (note.pitch < lowNote) {
                octave++;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            while (note.pitch > highNote) {
                octave--;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            
            // Step up or down in the scale
            int direction = (i < length/2) ? 1 : -1; // Ascending first half, descending second half
            scaleIndex += direction;
            
            // Occasionally jump by a third (2 scale steps)
            if (gen() % 4 == 0) {
                scaleIndex += direction * 2;
            }
            
            // Handle octave changes
            if (scaleIndex < 0) {
                scaleIndex += scale.size();
                if (octave > lowOctave) octave--;
            }
            if (scaleIndex >= (int)scale.size()) {
                scaleIndex -= scale.size();
                if (octave < highOctave) octave++;
            }
            
            // Set duration - quarter, half, or whole notes
            std::uniform_int_distribution<> durDist(0, 2);
            int durationType = durDist(gen);
            if (durationType == 0) note.duration = TPQ; // Quarter note
            else if (durationType == 1) note.duration = TPQ * 2; // Half note
            else note.duration = TPQ * 4; // Whole note
            
            // Set velocity (volume)
            std::uniform_int_distribution<> velDist(80, 110);
            note.velocity = velDist(gen);
            
            snippet.notes.push_back(note);
        }
    } 
    // For bass: create a simpler, more rhythmic pattern
    else {
        // Start at the root note
        int octave = lowOctave;

        for (int i = 0; i < length; i++) {
            Note note;
            
            // Bass often emphasizes root, fifth, and octave
            int scaleIndex;
            if (i % 4 == 0) scaleIndex = 0; // Root note
            else if (i % 4 == 2) scaleIndex = 4 % scale.size(); // Fifth if available
            else scaleIndex = gen() % scale.size();
            
            note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            
            // Make sure note is within range
            while (note.pitch < lowNote) {
                octave++;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            while (note.pitch > highNote) {
                octave--;
                note.pitch = createNoteInScale(scale, octave, scaleIndex, rootNote);
            }
            
            // Bass often has consistent quarter notes or half notes
            if (i % 2 == 0) note.duration = TPQ; // Quarter note
            else note.duration = TPQ * 2; // Half note
            
            // Bass usually has consistent velocity
            note.velocity = 100;
            
            snippet.notes.push_back(note);
        }
    }
    
    return snippet;
}

// Generate a drum pattern for a specific phase
DrumPattern generateDrumPattern(int phaseNumber) {
    DrumPattern pattern;
    
    // Basic 4/4 pattern with variations based on phase
    switch (phaseNumber % 3) {
        case 0: // Basic rock pattern
            // Kick on 1 and 3
            pattern.kick[0] = true;
            pattern.kick[8] = true;
            
            // Snare on 2 and 4
            pattern.snare[4] = true;
            pattern.snare[12] = true;
            
            // Hi-hat on eighth notes
            for (int i = 0; i < 16; i += 2) {
                pattern.hihat[i] = true;
            }
            break;
            
        case 1: // Syncopated pattern
            // Kick on 1, 2-and, and 4
            pattern.kick[0] = true;
            pattern.kick[6] = true;
            pattern.kick[12] = true;
            
            // Snare on 2 and 3-and
            pattern.snare[4] = true;
            pattern.snare[10] = true;
            
            // Hi-hat on quarter notes
            pattern.hihat[0] = true;
            pattern.hihat[4] = true;
            pattern.hihat[8] = true;
            pattern.hihat[12] = true;
            break;
            
        case 2: // Half-time feel
            // Kick on 1 and 3
            pattern.kick[0] = true;
            pattern.kick[8] = true;
            
            // Snare on 3
            pattern.snare[8] = true;
            
            // Hi-hat on every beat
            for (int i = 0; i < 16; i += 4) {
                pattern.hihat[i] = true;
            }
            break;
    }
    
    // Set velocities
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) {
            pattern.velocities[i] = 110; // Accents on downbeats
        } else if (i % 2 == 0) {
            pattern.velocities[i] = 90; // Medium on upbeats
        } else {
            pattern.velocities[i] = 70; // Soft on off-beats
        }
    }
    
    return pattern;
}

// Drum thread function (thread 0)
void drumThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // --- Original Phase Length Calculations (No Bar Alignment for Drums) ---
    int ticksPerPhase = (numPhases > 0) ? static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0)) / numPhases) : static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0)));
    // Ensure ticksPerPhase is at least 1 if duration is non-zero
    if (durationSec > 0 && ticksPerPhase <= 0) ticksPerPhase = 1;
    int totalTicks = static_cast<int>(durationSec * (TPQ * (TEMPO / 60.0))); // Use original total ticks

    // Calculate ticks per bar and step for pattern generation
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    int ticksPerStep = ticksPerBar / 4; // Assuming 4 beats, 4 subdivisions each = 16 steps
    // --- End Phase Length Calculations ---

    // Time tracking
    auto startWallTime = std::chrono::high_resolution_clock::now();
    int currentPhase = -1;

    // Add track name
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Drum Track");
    }

    // Main loop variables
    int currentTick = 0;
    int currentStep = -1; // Start at -1 so first comparison triggers

    // Random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);

    // Main loop
    while (running) {
        // Get current times
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;

        // Check if duration has elapsed (use original durationSec)
        if (currentWallTime >= durationSec) {
            break;
        }

        // Calculate current MIDI tick position based on wall time
        currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));

        // Check for phase change based on original ticks per phase
        int newPhase = (ticksPerPhase > 0) ? (currentTick / ticksPerPhase) : 0;
        // Ensure phase doesn't exceed numPhases - 1 due to rounding/timing
        if (newPhase >= numPhases) newPhase = numPhases - 1;

        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Calculate the exact tick for the phase change event using original ticksPerPhase
            int phaseEventTick = newPhase * ticksPerPhase;

            // Add a marker for the phase change
            midifile.addMarker(data.track, phaseEventTick, "Phase " + std::to_string(newPhase + 1));

            // Add crash cymbal at phase boundary
            midifile.addNoteOn(data.track, phaseEventTick, 9, CRASH, 110);
            // Ensure crash note off doesn't happen before note on
            midifile.addNoteOff(data.track, phaseEventTick + std::max(1, ticksPerStep), 9, CRASH);

            currentPhase = newPhase;
        }

        // Calculate the step based on musical timing
        int stepPosition = (ticksPerStep > 0) ? (currentTick / ticksPerStep) % 16 : 0;

        // Only output notes when we move to a new step
        if (stepPosition != currentStep) {
            currentStep = stepPosition;

            // Ensure currentPhase is valid before accessing drumPatterns
            if (currentPhase < 0) currentPhase = 0; // Handle initial state before first phase change
            const DrumPattern& pattern = data.drumPatterns[currentPhase % data.drumPatterns.size()];

            // Calculate the exact tick for this step (quantized to the grid)
            int stepTick = (ticksPerStep > 0) ? (currentTick / ticksPerStep) * ticksPerStep : currentTick;

            // Add drum hits on the quantized grid
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Add kick drum
            if (pattern.kick[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, KICK, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, KICK);
            }

            // Add snare drum
            if (pattern.snare[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, SNARE, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, SNARE);
            }

            // Add hi-hat
            if (pattern.hihat[stepPosition]) {
                int hihat = (stepPosition % 8 == 0) ? OPEN_HAT : CLOSED_HAT;
                midifile.addNoteOn(data.track, stepTick, 9, hihat, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + std::max(1, ticksPerStep - 1), 9, hihat);
            }
        }

        // Do some busy work to consume CPU cycles
        int busyWorkAmount = busyWorkDist(gen);
        volatile double sum = 0;
        for (int i = 0; i < busyWorkAmount; i++) {
            sum += sin(i) * cos(i);
        }

        // Sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }

    // Add a final marker at the originally requested end time
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        int originalEndTick = totalTicks; // Use totalTicks calculated from durationSec
        // Ensure the marker is placed at or after the last calculated tick if loop exited early
        int finalMarkerTick = std::max(currentTick, originalEndTick);
        midifile.addMarker(data.track, finalMarkerTick, "Original End");
    }
}

// Melodic thread function for all threads except the drum thread
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // --- Phase and Bar Alignment Calculations ---
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    double initialTotalTicks = durationSec * (TPQ * (TEMPO / 60.0));
    double initialTicksPerPhase = (numPhases > 0) ? initialTotalTicks / numPhases : initialTotalTicks;
    int adjustedTicksPerPhase = 0;
     if (ticksPerBar > 0 && initialTicksPerPhase > 0) {
         adjustedTicksPerPhase = static_cast<int>(std::round(initialTicksPerPhase / static_cast<double>(ticksPerBar))) * ticksPerBar;
         // Ensure phase duration is at least one bar if possible
         if (adjustedTicksPerPhase == 0) adjustedTicksPerPhase = ticksPerBar;
    } else {
        adjustedTicksPerPhase = static_cast<int>(initialTicksPerPhase); // Use initial if ticksPerBar is 0 or initial is 0
    }
    // Ensure adjustedTicksPerPhase is not negative
    if (adjustedTicksPerPhase < 0) adjustedTicksPerPhase = 0;

    int adjustedTotalTicks = adjustedTicksPerPhase * numPhases;
    double adjustedDurationSec = (TPQ > 0 && TEMPO > 0) ? adjustedTotalTicks / (TPQ * (TEMPO / 60.0)) : 0.0;

    // Time tracking
    auto startWallTime = std::chrono::high_resolution_clock::now();
    double lastWallTime = 0;
    double lastCpuTime = getCpuTime();
    int currentPhase = -1;

    // Add track name
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Thread " + std::to_string(data.id));
    }

    // Random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);

    // Thread state variables
    bool wasScheduled = false;
    bool noteIsOn = false;
    int currentNote = -1;
    int noteStartTick = 0;
    int noteDuration = 0;
    int currentTick = 0; // Declare currentTick outside the loop

    // Main loop
    while (running) {
        // Get current times
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
        double currentCpuTime = getCpuTime();

        // Check if duration has elapsed (use adjusted duration)
        if (currentWallTime >= adjustedDurationSec) {
            // Turn off any active notes
            if (noteIsOn) {
                std::lock_guard<std::mutex> lock(midi_mutex);
                // Calculate end tick based on adjusted duration
                int endTick = adjustedTotalTicks;
                midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
            }
            break;
        }

        // Calculate time deltas
        double wallTimeDelta = currentWallTime - lastWallTime;
        double cpuTimeDelta = currentCpuTime - lastCpuTime;

        // Calculate scheduling ratio
        double schedulingRatio = (wallTimeDelta > 0) ? cpuTimeDelta / wallTimeDelta : 0;

        // Determine if thread is scheduled
        bool isScheduled = (schedulingRatio > SCHEDULE_THRESHOLD);

        // Convert current wall time to tick for MIDI events
        currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0))); // Assign value inside loop

        // Check for phase change based on adjusted ticks per phase
        int newPhase = (adjustedTicksPerPhase > 0) ? (currentTick / adjustedTicksPerPhase) : 0;
        // Ensure phase doesn't exceed numPhases - 1
        if (newPhase >= numPhases) newPhase = numPhases - 1;

        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            // Calculate the exact tick for the phase change event
            int phaseEventTick = newPhase * adjustedTicksPerPhase;

            // End any active note precisely at the phase boundary
            if (noteIsOn) {
                // Ensure phaseEventTick doesn't precede noteStartTick if phase change is rapid
                int endTick = std::max(noteStartTick, phaseEventTick);
                midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
                noteIsOn = false; // Explicitly set noteIsOn to false
                wasScheduled = false; // Reset wasScheduled to trigger new note logic if still scheduled
            }

            // Add a marker for the phase change
            midifile.addMarker(data.track, phaseEventTick, "Phase " + std::to_string(newPhase + 1));

            currentPhase = newPhase;

            // Reset snippet pointer for new phase
            if (currentPhase >= 0 && currentPhase < data.snippets.size()) {
                Snippet& snippetToReset = data.snippets[currentPhase % data.snippets.size()];
                snippetToReset.reset();
            }
        }

        // Handle scheduling transitions
        if (isScheduled != wasScheduled) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            if (isScheduled) {
                // Thread was just scheduled - start a note from the snippet
                int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
                if (!noteIsOn && currentPhase >= 0 && currentPhase < data.snippets.size() && (adjustedTicksPerPhase == 0 || currentTick < nextPhaseTick)) {
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
                // Thread was just descheduled - end the note
                if (noteIsOn) {
                    int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
                    int endTick = (adjustedTicksPerPhase > 0 && currentTick >= nextPhaseTick) ? nextPhaseTick : currentTick;
                    midifile.addNoteOff(data.track, endTick, data.channel, currentNote);
                    noteIsOn = false;
                }
            }

            wasScheduled = isScheduled;
        }
        // Check if we need to move to the next note in the snippet
        else if (isScheduled && noteIsOn && currentTick - noteStartTick >= noteDuration) {
            std::lock_guard<std::mutex> lock(midi_mutex);

            int intendedEndTick = noteStartTick + noteDuration;
            int nextPhaseTick = (currentPhase + 1) * adjustedTicksPerPhase;
            int endTick = (adjustedTicksPerPhase > 0 && intendedEndTick >= nextPhaseTick) ? nextPhaseTick : intendedEndTick;

            midifile.addNoteOff(data.track, endTick, data.channel, currentNote);

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

        // Update last time values
        lastWallTime = currentWallTime;
        lastCpuTime = currentCpuTime;

        // Do some busy work to consume CPU cycles
        int busyWorkAmount = busyWorkDist(gen);
        volatile double sum = 0;
        for (int i = 0; i < busyWorkAmount; i++) {
            sum += sin(i) * cos(i);
        }

        // Sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }

    // Add a final marker at the originally requested end time
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        int alignedEndTick = adjustedTotalTicks; // Use the calculated aligned end tick
        // Ensure the marker is placed at or after the last calculated tick if loop exited early
        // Use alignedEndTick as the primary reference point for melodic tracks.
        int finalMarkerTick = std::max(currentTick, alignedEndTick); // Now currentTick is in scope
        midifile.addMarker(data.track, finalMarkerTick, "Aligned End"); // Changed marker name for clarity
    }
}