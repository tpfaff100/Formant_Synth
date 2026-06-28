# Formant_Synth
Research and fun with formant sine wave / cascade resonator - based on my desire to play with a Yamaha Yamaha's FS1R formant wave gen.

What if you wanted a Yamaha FS1R's formant wave generation ability but didn't have $2,000 to spend on one?
Well... as a result of looking at it some both related and unrelated ideas panned out here-


This is a fun all-C++ project that uses formant wave generators.  

The sine-based resonators are very simplistic and difficult to understand when they produce human speech.  
However, they sound musical and fun.
The project isn't about accurately reproducing human speech but more about thinking about speech musically.

Wave files are included in the project in some places so you can sample the output of the different command-line programs before building them.

This program is designed for macos but all but the last lines that play the wav files should be portable to Linux easily.
Specifically, I built them in Monterey/Intel.

Directions for compilation of each file is simpleton.  Just look at the top of each C++ file for compilation and execution instructions.

The more complex and capable software is in the 'final' directory.  

These programs play with may options with voice creation including:
- Ring modulation
- Switching between sine and cascade resonators for every other word
- Introducing trigonometry tangents to scramble voice to make it raspy and charmingly unintelligible!
- Adding a rise and fall to generated voice output to make it sound more appealing.

Some of these modes can be mixed and matched in the later programs which are most interestingly:

ringmod_tts
fricative2
filter_sweep_tts

The hybrid directory is where I first started mixing sine and cascade resonators and playing with options to switch between them.
There is some interesting sw in there but am including it mostly for reference / so I can remember later what I did, etc.

Check out the attached wave files to see if there is anything that excites you.


Why did I do this?  I am interested in voice production + ring modulation + effects + audio quality destruction.
A perfect project for me to play with.

Enjoy.
