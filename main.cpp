#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <random>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <ctime>
#include "external/midifile/include/MidiFile.h"
#include "external/midifile/include/Options.h"

using namespace std;
using namespace smf;

// ======================================================================
// CORE PARAMETERS
// ======================================================================

// Musical parameters
const int TPQ = 480;          // Ticks per quarter note (high resolution)
const int TEMPO = 120;        // BPM
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

// ======================================================================
// MUSICAL STRUCTURES
// ======================================================================

// Musical scales
const vector<int> MAJOR_SCALE = {0, 2, 4, 5, 7, 9, 11}; // C major
const vector<int> MINOR_SCALE = {0, 2, 3, 5, 7, 8, 10}; // C minor
const vector<int> PENTA_SCALE = {0, 2, 4, 7, 9};        // C pentatonic

// Root notes for different phases (in semitones from C)
const vector<int> PHASE_ROOTS = {0, 7, 5, 2}; // C, G, F, D

// ======================================================================
// DATA STRUCTURES
// ======================================================================

// Musical note structure
struct Note {
    int pitch;     // MIDI pitch (0-127)
    int velocity;  // MIDI velocity (0-127)
    int duration;  // Duration in ticks
};

// Musical snippet (a sequence of notes played when a thread is scheduled)
struct Snippet {
    vector<Note> notes;
    int currentNoteIndex = 0;
    
    void reset() {
        currentNoteIndex = 0;
    }
    
    Note* getNextNote() {
        if (notes.empty()) return nullptr;
        Note* note = &notes[currentNoteIndex];
        currentNoteIndex = (currentNoteIndex + 1) % notes.size();
        return note;
    }
};

// Drum pattern
struct DrumPattern {
    bool kick[16] = {false};   // 16 steps per pattern
    bool snare[16] = {false};
    bool hihat[16] = {false};
    int velocities[16] = {0};
};

// Thread data
struct ThreadData {
    int id;               // Thread ID
    int track;            // MIDI track
    int channel;          // MIDI channel
    int instrument;       // MIDI program change
    vector<Snippet> snippets; // One snippet per phase
    bool isDrumThread;    // True for thread 0 (drum thread)
    vector<DrumPattern> drumPatterns; // One pattern per phase (for drum thread)
};

// ======================================================================
// GLOBAL VARIABLES
// ======================================================================
mutex midi_mutex;
MidiFile midifile;
atomic<bool> running(true);

// ======================================================================
// HELPER FUNCTIONS
// ======================================================================

// Get CPU time in seconds
double getCpuTime() {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}

// Create a note in a specific scale
int createNoteInScale(const vector<int>& scale, int octave, int scaleIndex, int rootNote) {
    return (octave * 12) + rootNote + scale[scaleIndex % scale.size()];
}

// Generate a snippet for a thread based on range and scale
Snippet generateSnippet(int lowNote, int highNote, const vector<int>& scale, int rootNote, bool isBass) {
    random_device rd;
    mt19937 gen(rd());
    
    // More consistent snippet lengths
    uniform_int_distribution<> lenDist(4, 8);
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
            uniform_int_distribution<> durDist(0, 2);
            int durationType = durDist(gen);
            if (durationType == 0) note.duration = TPQ; // Quarter note
            else if (durationType == 1) note.duration = TPQ * 2; // Half note
            else note.duration = TPQ * 4; // Whole note
            
            // Set velocity (volume)
            uniform_int_distribution<> velDist(80, 110);
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

// ======================================================================
// THREAD FUNCTIONS
// ======================================================================

// Drum thread function (thread 0)
void drumThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // Calculate ticks per phase
    double phaseLength = static_cast<double>(durationSec) / numPhases;
    int ticksPerPhase = phaseLength * (TPQ * TEMPO / 60.0);
    int ticksPerBar = BEATS_PER_BAR * TPQ;
    int ticksPerStep = ticksPerBar / 4; // 16 steps per bar (16th notes)
    
    // Time tracking
    auto startWallTime = chrono::high_resolution_clock::now();
    double startCpuTime = getCpuTime();
    double lastCpuTime = startCpuTime;
    double lastWallTime = 0;
    int currentPhase = -1;
    
    // Add track name
    {
        lock_guard<mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Drum Track");
    }
    
    // Main loop variables
    int currentTick = 0;
    int currentStep = -1; // Start at -1 so first comparison triggers
    int noteCount = 0;
    
    // Random number generation
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);
    
    // Main loop
    while (running) {
        // Get current times
        auto nowWall = chrono::high_resolution_clock::now();
        double currentWallTime = chrono::duration_cast<chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
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
            lock_guard<mutex> lock(midi_mutex);
            
            // Add a marker for the phase change
            int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
            midifile.addMarker(data.track, phaseTick, "Phase " + to_string(newPhase + 1));
            
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
            lock_guard<mutex> lock(midi_mutex);
            
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
        this_thread::sleep_for(chrono::milliseconds(THREAD_SLEEP_MS));
    }
    
    cout << "Drum thread generated " << noteCount << " notes." << endl;
}

// Melodic thread function for all threads except the drum thread
void melodicThreadFunction(ThreadData data, int durationSec, int numPhases) {
    // Calculate phase length
    double phaseLength = static_cast<double>(durationSec) / numPhases;
    int ticksPerPhase = phaseLength * (TPQ * TEMPO / 60.0);
    
    // Time tracking
    auto startWallTime = chrono::high_resolution_clock::now();
    double startCpuTime = getCpuTime();
    double lastCpuTime = startCpuTime;
    double lastWallTime = 0;
    int currentPhase = -1;
    
    // Add track name
    {
        lock_guard<mutex> lock(midi_mutex);
        midifile.addTrackName(data.track, 0, "Thread " + to_string(data.id));
    }
    
    // Random number generation
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> busyWorkDist(BUSY_WORK_MIN, BUSY_WORK_MAX);
    
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
        auto nowWall = chrono::high_resolution_clock::now();
        double currentWallTime = chrono::duration_cast<chrono::milliseconds>(nowWall - startWallTime).count() / 1000.0;
        double currentCpuTime = getCpuTime();
        
        // Check if duration has elapsed
        if (currentWallTime >= durationSec) {
            // Turn off any active notes
            if (noteIsOn) {
                lock_guard<mutex> lock(midi_mutex);
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
            lock_guard<mutex> lock(midi_mutex);
            
            // End any active note
            if (noteIsOn) {
                int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
                midifile.addNoteOff(data.track, phaseTick, data.channel, currentNote);
                noteIsOn = false;
            }
            
            // Add a marker for the phase change
            int phaseTick = static_cast<int>(newPhase * phaseLength * (TPQ * (TEMPO / 60.0)));
            midifile.addMarker(data.track, phaseTick, "Phase " + to_string(newPhase + 1));
            
            // Reset snippet pointer for new phase
            data.snippets[newPhase % data.snippets.size()].reset();
            
            currentPhase = newPhase;
        }
        
        // Handle scheduling transitions
        if (isScheduled != wasScheduled) {
            lock_guard<mutex> lock(midi_mutex);
            
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
            lock_guard<mutex> lock(midi_mutex);
            
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
        this_thread::sleep_for(chrono::milliseconds(THREAD_SLEEP_MS));
    }
    
    cout << "Thread " << data.id << " generated " << noteCount << " notes." << endl;
}

int main(int argc, char* argv[]) {
    Options options;
    options.define("n|num-threads=i:4", "Number of threads to create");
    options.define("t|time=i:60", "Duration in seconds");
    options.define("p|phases=i:3", "Number of musical phases");
    options.process(argc, argv);
    
    int threadCount = options.getInteger("num-threads");
    int durationSec = options.getInteger("time");
    int numPhases = options.getInteger("phases");
    
    // Apply minimum values
    if (threadCount <= 0) threadCount = 4;
    if (durationSec <= 0) durationSec = 60;
    if (numPhases <= 0) numPhases = 3;
    
    cout << "Creating " << threadCount << " threads for " << durationSec 
         << " seconds with " << numPhases << " musical phases" << endl;
    
    // Initialize MIDI file
    midifile.absoluteTicks();
    midifile.setTPQ(TPQ);
    
    // Create a track for each thread
    for (int i = 0; i < threadCount; i++) {
        midifile.addTrack();
    }
    
    // Add tempo metadata
    midifile.addTempo(0, 0, TEMPO);
    midifile.addTimeSignature(0, 0, 4, 2, 24, 8); // 4/4 time signature
    
    // Create thread configuration data
    vector<ThreadData> threadConfigs;
    
    // Set up the dedicated drum thread (thread 0)
    ThreadData drumThread;
    drumThread.id = 0;
    drumThread.track = 0;
    drumThread.channel = 9; // Channel 9 is reserved for percussion
    drumThread.instrument = 0; // Not used for percussion
    drumThread.isDrumThread = true;
    
    // Create drum patterns for each phase
    for (int phase = 0; phase < numPhases; phase++) {
        drumThread.drumPatterns.push_back(generateDrumPattern(phase));
    }
    
    threadConfigs.push_back(drumThread);
    
    // Set up melodic threads
    for (int i = 1; i < threadCount; i++) {
        ThreadData config;
        config.id = i;
        config.track = i;
        config.channel = (i - 1) % 15 + 1; // Channels 1-16, skipping 9 (percussion)
        if (config.channel == 9) config.channel = 16; // Skip channel 9
        config.isDrumThread = false;
        
        // Choose instrument based on thread ID
        if (i % 3 == 1) {
            // Bass instruments
            config.instrument = 32 + (i % 8); // Various bass instruments
            // Generate snippets for each phase for bass
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 2 == 0) ? MAJOR_SCALE : MINOR_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(BASS_LOW, BASS_HIGH, scale, rootNote, true));
            }
        } else if (i % 3 == 2) {
            // Mid-range instruments
            config.instrument = 16 + (i % 8); // Various organ/guitar instruments
            // Generate snippets for each phase for mid-range
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 3 == 0) ? MAJOR_SCALE : 
                                         (phase % 3 == 1) ? MINOR_SCALE : PENTA_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(MID_LOW, MID_HIGH, scale, rootNote, false));
            }
        } else {
            // High-range instruments
            config.instrument = 80 + (i % 8); // Various lead instruments
            // Generate snippets for each phase for high-range
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 3 == 0) ? PENTA_SCALE : 
                                         (phase % 3 == 1) ? MAJOR_SCALE : MINOR_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(HIGH_LOW, HIGH_HIGH, scale, rootNote, false));
            }
        }
        
        threadConfigs.push_back(config);
        
        // Set instrument for this track
        midifile.addPatchChange(config.track, 0, config.channel, config.instrument);
    }
    
    // Create threads
    vector<thread> threads;
    for (const auto& config : threadConfigs) {
        if (config.isDrumThread) {
            threads.emplace_back(drumThreadFunction, config, durationSec, numPhases);
        } else {
            threads.emplace_back(melodicThreadFunction, config, durationSec, numPhases);
        }
    }
    
    // Wait for threads to finish
    for (auto& t : threads) {
        t.join();
    }
    
    // Sort MIDI events by time
    midifile.sortTracks();
    
    // Write MIDI file
    string filename = "thread_music_" + to_string(threadCount) + "threads_" + 
                      to_string(durationSec) + "sec_" + to_string(numPhases) + "phases.mid";
    midifile.write(filename);
    
    cout << "MIDI file " << filename << " has been created." << endl;
    cout << "Tracks: " << midifile.getTrackCount() << endl;
    
    return 0;
}