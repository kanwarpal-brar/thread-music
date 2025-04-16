#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <ctime>
#include "external/midifile/include/MidiFile.h"
#include "external/midifile/include/Options.h"
#include "include/Constants.h"
#include "include/Types.h"
#include "include/Utils.h"
#include "include/MusicGeneration.h"

using namespace std;
using namespace smf;

// Global MIDI file instance shared across threads
MidiFile midifile;

int main(int argc, char* argv[]) {
    // Parse command-line options
    Options options;
    options.define("n|num-threads=i:4", "Number of threads to create");
    options.define("t|time=i:60", "Duration in seconds");
    options.define("p|phases=i:3", "Number of musical phases");
    options.process(argc, argv);
    
    // Extract and validate settings
    int threadCount = options.getInteger("num-threads");
    int durationSec = options.getInteger("time");
    int numPhases = options.getInteger("phases");
    
    // Apply minimum values for parameters
    if (threadCount <= 0) threadCount = 4;
    if (durationSec <= 0) durationSec = 60;
    if (numPhases <= 0) numPhases = 3;
    
    cout << "Creating " << threadCount << " threads for " << durationSec 
         << " seconds with " << numPhases << " musical phases" << endl;
    
    // Initialize MIDI file structure
    midifile.absoluteTicks();  // Use absolute timing
    midifile.setTPQ(TPQ);      // Set timing resolution
    
    // Create a track for each thread
    for (int i = 0; i < threadCount; i++) {
        midifile.addTrack();
    }
    
    // Add global tempo metadata to first track
    midifile.addTempo(0, 0, TEMPO);
    midifile.addTimeSignature(0, 0, 4, 2, 24, 8); // 4/4 time signature
    
    // Create thread configuration data
    vector<ThreadData> threadConfigs;
    
    // Set up the dedicated drum thread (always thread 0)
    ThreadData drumThread;
    drumThread.id = 0;
    drumThread.track = 0;
    drumThread.channel = 9; // Channel 9 (10 in user interfaces) is reserved for percussion in MIDI
    drumThread.instrument = 0; // Instrument number not used for percussion channel
    drumThread.isDrumThread = true;
    
    // Create drum patterns for each phase
    for (int phase = 0; phase < numPhases; phase++) {
        drumThread.drumPatterns.push_back(generateDrumPattern(phase));
    }
    
    threadConfigs.push_back(drumThread);
    
    // Set up melodic threads with different registers and roles
    for (int i = 1; i < threadCount; i++) {
        ThreadData config;
        config.id = i;
        config.track = i;
        config.channel = (i - 1) % 15 + 1; // Spread across channels 1-16, skipping 10 (percussion)
        if (config.channel == 9) config.channel = 16; // Skip percussion channel
        config.isDrumThread = false;
        
        // Assign instrument role and snippets based on thread ID
        if (i % 3 == 1) {
            // Bass instruments - provide harmonic foundation
            config.instrument = 32 + (i % 8); // Various bass instruments
            // Generate snippets for each phase with bass-specific patterns
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 2 == 0) ? MAJOR_SCALE : MINOR_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(BASS_LOW, BASS_HIGH, scale, rootNote, true));
            }
        } else if (i % 3 == 2) {
            // Mid-range instruments - provide harmonic context
            config.instrument = 16 + (i % 8); // Various organ/guitar instruments
            // Generate snippets for each phase with mid-range patterns
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 3 == 0) ? MAJOR_SCALE : 
                                         (phase % 3 == 1) ? MINOR_SCALE : PENTA_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(MID_LOW, MID_HIGH, scale, rootNote, false));
            }
        } else {
            // High-range instruments - provide melodic interest
            config.instrument = 80 + (i % 8); // Various lead instruments
            // Generate snippets for each phase with lead patterns
            for (int phase = 0; phase < numPhases; phase++) {
                const vector<int>& scale = (phase % 3 == 0) ? PENTA_SCALE : 
                                         (phase % 3 == 1) ? MAJOR_SCALE : MINOR_SCALE;
                int rootNote = PHASE_ROOTS[phase % PHASE_ROOTS.size()];
                config.snippets.push_back(generateSnippet(HIGH_LOW, HIGH_HIGH, scale, rootNote, false));
            }
        }
        
        threadConfigs.push_back(config);
        
        // Set instrument for this track (Program Change message)
        midifile.addPatchChange(config.track, 0, config.channel, config.instrument);
    }
    
    // Create and launch threads
    vector<thread> threads;
    for (const auto& config : threadConfigs) {
        if (config.isDrumThread) {
            threads.emplace_back(drumThreadFunction, config, durationSec, numPhases);
        } else {
            threads.emplace_back(melodicThreadFunction, config, durationSec, numPhases);
        }
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Ensure MIDI events are in chronological order
    midifile.sortTracks();
    
    // Use current timestamp as unique identifier
    time_t timeNow = time(nullptr);
    
    // Generate output filename with parameters and timestamp
    string filename = "thread_music_" + to_string(threadCount) + "threads_" + 
                      to_string(durationSec) + "sec_" + to_string(numPhases) + "phases_" + 
                      to_string(timeNow) + ".mid";
    midifile.write(filename);
    
    cout << "MIDI file " << filename << " has been created." << endl;
    cout << "Tracks: " << midifile.getTrackCount() << endl;
    
    return 0;
}