# Saxes
A wavetable-sythesis based saxophon.

Hardware: Teensy 4.1 with 16 MB additional flash memory, PJRC-Audio-Shield, 2 MPR121 (touch-sensor shields), MPXV5010 pressure-sensor, OLED-Display (128 x 64), Adafruit Pro Trinket Li-Ion charger, 3,7 Watt Max98306 stereo class-D amplifier, 2 loudspeakers, saxophone mouthpiece.  
The file "WavetableSAX_T41_AuSh_Saxa.ino" (the main loop) uses ideas and code-fragments from Jeff Hopkins (https://github.com/jeffmhopkins/Open-Woodwind-Project) and Johan Berglund (https://hackaday.io/Yoe). Many thanks! The original soundfont-File, which I decoded with the SoundFont Decoder (from the "Wavetable Synthesis Capstone project at Portland State, Fall 2016 - Winter 2017" many thanks to you!) is a public domain file (unfortunately I don't remember its origins - but many thanks to its creator). One can use any sf2-file, as long as its size is smaller than the memory on the Teensy 4.1 board. Some parts oft the decoded files (".cpp", ".h") I had to adapt manually. Otherweise the would't compile in Arduino.  
There are two kinds of fingering-tables: a simple one with 12 fingerings, repeatedly used for 3 octaves, and one with the original saxophone fingerings. The sax can be played as alto-sax, tenor-sax, C-sax or soprano-saxophone.  
The OLED-display shows the played note's pitch, or the pressure of the applied wind, meassured by the pressure sensor.  
There is a headphone output, a line-out, or one uses the loudspeekers of the sax.  
It sounds quite well, it is cheap to build (about 100 €). The only shortcomming is, that my sax has no legato (to difficult to code).  
Special thanks to Paul Stoffregen (https://www.pjrc.com/) and his team for their fantastic microcontroller-boards and music shields!  
This is a work in progress: I intend to add a menu for the OLED-display.  
  
I also built two other woodwind instruments last year. One uses the Dreamblaster X2 board (https://www.serdashop.com/DreamBlasterX2), an advanced Waveblaster Compatible and USB Compatible MIDI Synthesizer board. But this intrument is more expensive and does't sound as well as my WavetableSAX.  
The third one is based on Jeff Hopkins woodwind-instrument (https://github.com/jeffmhopkins/Open-Woodwind-Project). It features a teensy 4.1, a PJRC-audio shield, a MPXV5010 pressure-sensor...It does not implement MIDI (like Mr. Hopkins instrument), but nevertheless it can be controlled by TouchOSC (https://hexler.net/touchosc). It is a work in progress too. Again many thanks to Mr. Hopkins: not being a professionell programmer myself, I would never have be able to built my three woodwinds without his ideas and his skill.

