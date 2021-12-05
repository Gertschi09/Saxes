# Saxes
A wavetable-sythesis based saxophon.

Hardware: Teensy 4.1 with 16 MB additional flash memory, PJRC-Audio-Shield, 2 MPR121 (touch-sensor shields), MPXV5010 pressure-sensor, OLED-Display (128 x 64), Adafruit Pro Trinket Li-Ion charger, 3,7 Watt Max98306 stereo class-D amplifier, 2 loudspeaker, saxophone mouthpiece.  
The file "WavetableSAX_T41_AuSh_Saxa.ino" (the main loop) uses ideas and code-fragments from Jeff Hopkins (https://github.com/jeffmhopkins/Open-Woodwind-Project) and Johan Berglund (https://hackaday.io/Yoe). Many thanks! The original soundfont-File, which I decoded with the SoundFont Decoder (from the "Wavetable Synthesis Capstone project at Portland State, Fall 2016 - Winter 2017" many thanks to you!) is a public domain file (unfortunately I don't remember its origins - but many thanks to its creator). One can use any sf2-file, as long as its size is smaller than the memory on the Teensy 4.1 board. Some parts oft the decoded files ("SAXa_samples.cpp", "SAXa_samples.h") have to be adapted manually. Otherweise the don't compile in Arduino.  
There ar two kinds of fingering-tables: a simple one with 12 fingerings, repeatedly used for 3 octaves, and one with the original saxophone fingerings. The sax can be playes as alto-sax, tenor-sax, C-sax or soprano-saxophone.  
On the OLED-display the played note's pitch, or the pressure of the applied wind meassured by the pressure sensor.  
There is a headphone output, a line-out, or one uses the loudspekers of the sax.  
It sounds quite well, is cheap to build (about 100 €). The only shortcomming is, that my sax has no legato (to difficult to code).  
Special thanks to Paul Stoffregen and his team for their fantastic microcontroller-boards and music shields!  
This is a work in progress: I intend to add a menu for the OLED-display. 
