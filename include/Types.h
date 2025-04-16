#ifndef THREAD_MUSIC_TYPES_H
#define THREAD_MUSIC_TYPES_H

#include <vector>

// Musical note structure
struct Note {
    int pitch;     // MIDI pitch (0-127)
    int velocity;  // MIDI velocity (0-127)
    int duration;  // Duration in ticks
};

// Musical snippet (a sequence of notes played when a thread is scheduled)
struct Snippet {
    std::vector<Note> notes;
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
    std::vector<Snippet> snippets; // One snippet per phase
    bool isDrumThread;    // True for thread 0 (drum thread)
    std::vector<DrumPattern> drumPatterns; // One pattern per phase (for drum thread)
};

#endif // THREAD_MUSIC_TYPES_H