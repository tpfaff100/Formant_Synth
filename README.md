# Formant_Synth
Research and fun with formant sine wave / cascade resonator - based on my desire to play with a Yamaha Yamaha's FS1R formant wave gen.

What if you wanted a Yamaha FS1R's formant wave generation ability but didn't have $2,000 to spend on one?
Well... as a result of looking at it some both related and unrelated ideas panned out here-


This is a fun all-C++ project that uses formant wave generators.  

The sine-based resonators are very simplistic and difficult to understand when they produce human speech.  
However, they sound musical and fun.
The project isn't about accurately reproducing human speech but more about thinking about speech musically.

Wave files are included in the project in some places so you can sample the output of the different command-line programs before building them.

These command-line programs are designed for macos but all but the last lines that play the wav files should be portable to Linux easily.
Specifically, I built them in Mac OS Monterey on Intel.

Directions for compilation of each file is simple.  
Just look at the top of each C++ file for compilation and execution instructions.

<bold>
The more complex and capable software is in the 'final' directory.  
</bold>

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


Some sample usage:

./fricative2 [--hybrid | --sine | --singing | --alternate | --alternate-singing | --glitch-singing | --alternate-glitch]
<br>
./fricative2 --alternate-singing "hi there how are you?"
<br>
./fricative2 --sine "hi there"
<br>
./fricative2 --hybrid "hi there"
<br>
./fricative2 --singing "hi there how are you?"
<br>
./fricative2 "aaaabbbb"
<br>
<br>

./filter_tts -filter-singing "hi there"
<br>
./filter_tts --filter-glitch "gee my throat sounds kind of glitchy today"
<br>
<br>

./ringmod_tts "fffffffsssssssssttttttt"
<br>
./ringmod_tts [--filter-sine | --filter-singing | --filter-glitch] "<phrase>"
<br>
./ringmod_tts --filter-sine "why do I sound so strange?"
<br>
./ringmod_tts --filter-singing "why do I sound so strange?"
<br>
./ringmod_tts "hhhoooohhhooo"
<br>
./ringmod_tts "yesyesyesyesyesnonononono"
<br>
./ringmod_tts "ooooeeeeooooeeeeooooeeeeooooeeeeooooooooooooooooeeeeeeeeeeeeeeee"
<br>
./ringmod_tts "fffffffttttttt" 
<br>
./ringmod_tts "fffffffssssssssstttttttkkkkkkkkkkkkggggggggg"
<br>
<br>


Some of the programs also have a way to flip between sine and hybrid word generation mode.
<br>
Noteworthy is flex_tts in the hybrid directory:
<br>
./flex_tts --start-hybrid "a trip to the market"
<br>
./flex_tts --start-sine "a trip to the market"
<br>
<br>
<br>
Are you astounded yet?  Or am I just easily amused?

If you are astounded, don't forget to check out the simpler programs in the 'hybrid' and 'initial_research' folders.  

The simpler programs were not dead-ends I just didn't have time to keep going with them.

Enjoy.
