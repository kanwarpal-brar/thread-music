#ifndef THREAD_MUSIC_TYPES_H
#define THREAD_MUSIC_TYPES_H

#include <vector>

// Note: Represents a single musical note in MIDI format
struct Note {
    int pitch;     // MIDI pitch (0-127, where 60 is middle C)
    int velocity;  // Note volume/intensity (0-127)
    int duration;  // Duration in MIDI ticks
};

// Snippet: A musical phrase that threads play when scheduled
struct Snippet {
    std::vector<Note> notes;
    int currentNoteIndex = 0;
    
    // Reset to beginning of snippet for new iteration
    void reset() {
        currentNoteIndex = 0;
    }
    
    // Get the next note in sequence and advance position
    Note* getNextNote() {
        if (notes.empty()) return nullptr;
        Note* note = &notes[currentNoteIndex];
        currentNoteIndex = (currentNoteIndex + 1) % notes.size();
        return note;
    }
};

// DrumPattern: 16-step rhythm pattern for percussion instruments
struct DrumPattern {
    bool kick[16] = {false};   // Bass drum hits
    bool snare[16] = {false};  // Snare drum hits
    bool hihat[16] = {false};  // Hi-hat cymbal hits
    int velocities[16] = {0};  // Velocity/intensity per step
};

// ThreadData: Configuration and state for each musical thread
struct ThreadData {
    int id;               // Thread identifier
    int track;            // MIDI track number
    int channel;          // MIDI channel (0-15, with 9 reserved for drums)
    int instrument;       // MIDI program/instrument number
    std::vector<Snippet> snippets; // Musical phrases for each phase
    bool isDrumThread;    // Identifies the rhythm thread
    std::vector<DrumPattern> drumPatterns; // Rhythm patterns for each phase
};

#endif // THREAD_MUSIC_TYPES_H