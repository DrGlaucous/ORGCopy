
## ORGCopy
`version 1.0.2` by Dr_Glaucous

[Find the source code here](https://github.com/DrGlaucous/ORGCopy)

This program allows the user to convert MIDI files into ORG files.
For those who are unaware, the organya (ORG) song format is used with the popular indie videogame ["Cave Story" (Doukutsu Monogatari)](https://en.wikipedia.org/wiki/Cave_Story) initially released in 2004. [You can download it here.](https://www.cavestory.org/download/cave-story.php)

It was developed by the game's creator, Daisuke "Pixel" Amaya specifically for said game.

Being as this music format is rather obscure, not many tools exist for altering it. This tool is now in that list.
___

### Program features:
* Drag and drop support for directory names
* Automatic time signature adjustment to account for any differences between songs
* The ability to combine multiple tracks into 1 (TrackMASHing)
* That's about it...

&nbsp;
&nbsp;
&nbsp;

___
### Usage:

I tried my best to make the program as user-friendly as possible (that is, for a terminal application).

Run the application either by clicking on it, or by executing it from a terminal window. Just follow the series of prompts to copy your ORG tracks to other files. 

To combine tracks that are both populated via TrackMASHing, answer "no" when the prompt asks you if you want to overwrite an already populated track (this question will only be asked if the program finds that your destination track already has notes in it). You will then be able to choose if you want to combine the tracks.

You must choose a priority track in order to combine them. The priority track will be the one whose notes are kept whenever track notes come into conflict.

___
#### Recommended Supplementary Tools

[MIDI2ORG](https://github.com/DrGlaucous/MIDI2ORG) - the companion tool to ORGCopy, it allows the user to convert MIDI files into ORG files.

[ORGMaker2](https://www.cavestory.org/download/music-tools.php) - the program used to actually **view and edit** .org files [-and it's also open-source, too](https://github.com/shbow/organya)

___
### Building:
In the same directory as the CMakeLists.txt, enter the following:

Generate Makefiles:
`cmake -B ./build`

(append >`-G"MSYS Makefiles"`< if using MSYS2 to build)


Generate executable:
`cmake --build ./build --config Release`

The final executable can be found in the "bin" directory

___
### Credits:
Organya Music Format: Daisuke "Pixel" Amaya

"File.cpp" (mostly borrowed from CSE2, a reverse-engineering of the cave story engine): Clownacy and whoever else...

[MIDI-Parser](https://github.com/MStefan99/Midi-Parser) library: MStefan99

Everything left over: Dr_Glaucous