#include <iostream>
/*
The goal of this project is to assemble the fundamental building blocks of a song by programmatically generating MIDI notes, for multiple instruments.
These midi files will be imported and assigned to existing instruments in Ableton live.
Midi files generates should aim to produce a cohesive song.
Where possibe, the use of automations like changes of pan, volume, etc over time is recommended.

More specifics:
- input argument accepted is "-n" a number of threads, and a time "-t" in seconds
- n threads are created, and a midifile ready for n tracks
- for "t" seconds, the threads, using a clock to track their own descheduling, will produce midi notes when they are scheduled
- the goal is to represent the scheduling/descheduling of threads over time, with music
- at the end, the main thread should save and write the midi file
*/

#include "MidiFile.h"

int main(int argc, char* argv[]) {
    // Your code here
    
    std::cout << "Hello, World!" << std::endl;
    
    return 0;
}