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
        int rootScale = 0; // Root note in scale
        
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
    // Calculate ticks per phase
    double phaseLength = static_cast<double>(durationSec) / numPhases;
    int ticksPerPhase = phaseLength * (TPQ * TEMPO / 60.0);
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    int ticksPerStep = ticksPerBar / 4; // 16 steps per bar (16th notes)
    
    // Time tracking
    auto startWallTime = std::chrono::high_resolution_clock::now();
    double startCpuTime = getCpuTime();
    double lastCpuTime = startCpuTime;
    double lastWallTime = 0;
    int currentPhase = -1;
    
    // Add track name
    {
        std::lock_guard<std::mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Drum Track");
    }
    
    // Main loop variables
    int currentTick = 0;
    int currentStep = -1; // Start at -1 so first comparison triggers
    int noteCount = 0;
    
    // Random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);
    
    // Main loop
    while (running) {
        // Get current times
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
        double currentCpuTime = getCpuTime();
        
        // Check if duration has elapsed
        if (currentWallTime >= durationSec) {
            break;
        }
        
        // Calculate time deltas
        double wallTimeDelta = currentWallTime - lastWallTime;
        double cpuTimeDelta = currentCpuTime - lastCpuTime;
        
        // Calculate scheduling ratio
        double schedulingRatio = (wallTimeDelta > 0) ? cpuTimeDelta / wallTimeDelta : 0;
        
        // Determine if thread is scheduled
        bool isScheduled = (schedulingRatio > SCHEDULE_THRESHOLD);
        
        // Calculate current MIDI tick position based on wall time
        currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));
        
        // Check for phase change
        int newPhase = static_cast<int>(currentWallTime / phaseLength);
        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);
            
            // Add a marker for the phase change
            int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
            midifile.addMarker(data.track, phaseTick, "Phase " + std::to_string(newPhase + 1));
            
            // Add crash cymbal at phase boundary
            midifile.addNoteOn(data.track, phaseTick, 9, CRASH, 110);
            midifile.addNoteOff(data.track, phaseTick + ticksPerStep, 9, CRASH);
            noteCount++;
            
            currentPhase = newPhase;
        }
        
        // Calculate the step based on musical timing rather than pattern position
        // This ensures consistent playback regardless of thread scheduling
        int stepPosition = (currentTick / ticksPerStep) % 16;
        
        // Only output notes when we move to a new step
        if (stepPosition != currentStep) {
            currentStep = stepPosition;
            
            // Get the current pattern for this phase
            const DrumPattern& pattern = data.drumPatterns[currentPhase % data.drumPatterns.size()];
            
            // Calculate the exact tick for this step (quantized to the grid)
            int stepTick = (currentTick / ticksPerStep) * ticksPerStep;
            
            // Always add drum hits on the quantized grid regardless of scheduling
            std::lock_guard<std::mutex> lock(midi_mutex);
            
            // Add kick drum
            if (pattern.kick[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, KICK, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + ticksPerStep - 1, 9, KICK);
                noteCount++;
            }
            
            // Add snare drum
            if (pattern.snare[stepPosition]) {
                midifile.addNoteOn(data.track, stepTick, 9, SNARE, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + ticksPerStep - 1, 9, SNARE);
                noteCount++;
            }
            
            // Add hi-hat
            if (pattern.hihat[stepPosition]) {
                int hihat = (stepPosition % 8 == 0) ? OPEN_HAT : CLOSED_HAT;
                midifile.addNoteOn(data.track, stepTick, 9, hihat, pattern.velocities[stepPosition]);
                midifile.addNoteOff(data.track, stepTick + ticksPerStep - 1, 9, hihat);
                noteCount++;
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
    
    std::cout << "Drum thread generated " << noteCount << " notes." << std::endl;
}

// Melodic thread function for all threads except the drum thread
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // Calculate phase length
    double phaseLength = static_cast<double>(durationSec) / numPhases;
    int ticksPerPhase = phaseLength * (TPQ * TEMPO / 60.0);
    
    // Time tracking
    auto startWallTime = std::chrono::high_resolution_clock::now();
    double startCpuTime = getCpuTime();
    double lastCpuTime = startCpuTime;
    double lastWallTime = 0;
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
    int noteCount = 0;
    
    // Main loop
    while (running) {
        // Get current times
        auto nowWall = std::chrono::high_resolution_clock::now();
        double currentWallTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
        double currentCpuTime = getCpuTime();
        
        // Check if duration has elapsed
        if (currentWallTime >= durationSec) {
            // Turn off any active notes
            if (noteIsOn) {
                std::lock_guard<std::mutex> lock(midi_mutex);
                int endTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));
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
        int currentTick = static_cast<int>(currentWallTime * (TPQ * (TEMPO / 60.0)));
        
        // Check for phase change
        int newPhase = static_cast<int>(currentWallTime / phaseLength);
        if (newPhase != currentPhase) {
            std::lock_guard<std::mutex> lock(midi_mutex);
            
            // End any active note
            if (noteIsOn) {
                int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
                midifile.addNoteOff(data.track, phaseTick, data.channel, currentNote);
                noteIsOn = false;
            }
            
            // Add a marker for the phase change
            int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
            midifile.addMarker(data.track, phaseTick, "Phase " + std::to_string(newPhase + 1));
            
            // Reset snippet pointer for new phase
            data.snippets[newPhase % data.snippets.size()].reset();
            
            currentPhase = newPhase;
        }
        
        // Handle scheduling transitions
        if (isScheduled != wasScheduled) {
            std::lock_guard<std::mutex> lock(midi_mutex);
            
            if (isScheduled) {
                // Thread was just scheduled - start a note from the snippet
                if (!noteIsOn && currentPhase >= 0) {
                    // Get the current snippet
                    Snippet& snippet = data.snippets[currentPhase % data.snippets.size()];
                    Note* note = snippet.getNextNote();
                    
                    if (note) {
                        // Start the note
                        currentNote = note->pitch;
                        noteDuration = note->duration;
                        midifile.addNoteOn(data.track, currentTick, data.channel, currentNote, note->velocity);
                        noteStartTick = currentTick;
                        noteIsOn = true;
                        noteCount++;
                    }
                }
            } else {
                // Thread was just descheduled - end the note
                if (noteIsOn) {
                    midifile.addNoteOff(data.track, currentTick, data.channel, currentNote);
                    noteIsOn = false;
                }
            }
            
            wasScheduled = isScheduled;
        }
        // Check if we need to move to the next note in the snippet
        else if (isScheduled && noteIsOn && currentTick - noteStartTick >= noteDuration) {
            std::lock_guard<std::mutex> lock(midi_mutex);
            
            // End the current note
            midifile.addNoteOff(data.track, currentTick, data.channel, currentNote);
            
            // Get the next note from the snippet
            Snippet& snippet = data.snippets[currentPhase % data.snippets.size()];
            Note* note = snippet.getNextNote();
            
            if (note) {
                // Start the next note
                currentNote = note->pitch;
                noteDuration = note->duration;
                midifile.addNoteOn(data.track, currentTick, data.channel, currentNote, note->velocity);
                noteStartTick = currentTick;
                noteCount++;
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
    
    std::cout << "Thread " << data.id << " generated " << noteCount << " notes." << std::endl;
}