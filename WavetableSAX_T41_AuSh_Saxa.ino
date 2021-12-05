 /*
 * Teensy 4.1 & 8 MB-Flash, PRJC-Audio-Shield
 * WavetableSAX_T41_AuSh_SAX??.ino
 *2021-10-06  10:30
 * Griffweise: Muster für ALLE Saxes
 * mit Display
*/
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>
#include <Adafruit_MPR121.h>
#include <U8g2lib.h>  //LCD-Display 128x64
#include "SAXa_samples.h"

#define STATE_NOTE_OFF 0
#define STATE_NOTE_NEW 1
#define STATE_NOTE_ON  2
#define SETTINGS_BREATH_THRESHOLD    80
#define FMAP_SIZE                    155
#define FMAPNEU_SIZE                 41
#define ON_Delay                     5   // Set Delay after ON threshold before velocity is checked (wait for tounging peak)
#define CC_INTERVAL                  5

// EEPROM: ****************************
#define VERSION_ADDR 0
#define STIMM_ADDR 20
#define DISPWHAT_ADDR 24
#define TONART_ADDR 30
#define GRIFF_ADDR  34

//"factory" values for settings
#define VERSION 7            //!!!!!
#define STIMM_FACTORY 3          // welches Sax (C-Sax, Sopran, Alt, Tenor)
#define DISPWHAT_FACTORY 2                 //0: keine Anzeige, 2: note, 1: breath
#define TONART_FACTORY 0           // 0: default b-Tonarten, 1: #-Tonarten
#define GRIFF_FACTORY 0        //0:default(normale Griffweise), 1: vereinfachte Griffweise

Adafruit_MPR121 cap1 = Adafruit_MPR121();
Adafruit_MPR121 cap2 = Adafruit_MPR121();
U8G2_SH1106_128X64_WINSTAR_F_SW_I2C u8g2(U8G2_R3, 24, 25, U8X8_PIN_NONE);

// GUItool: begin automatically generated code
AudioSynthWavetable      wavetable;      //xy=84,198
AudioMixer4              mixer;          //xy=316,364
AudioOutputI2S           i2s1;           //xy=538,368
AudioConnection          patchCord1(wavetable, 0, mixer, 0);
AudioConnection          patchCord2(mixer, 0, i2s1, 0);
AudioConnection          patchCord3(mixer, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=90,390
// GUItool: end automatically generated code
int li_Pin = 30;
int re_Pin = 32;
int mi_Pin = 31;
int menu_Pin = 33;

int  CC_Number = 0;
int  state = STATE_NOTE_OFF;
unsigned long ccSendTime = 0L;     // The last time we sent CC values
unsigned long breath_on_time = 0L; // Time when breath sensor value went over the ON threshold
int  note_playing             = 0;
int  note_fingered            = 0;
int note                      = 0;
int note_merk                 = 0;
char vorzeichen[2]              = " ";
int breath_vol                = 0;
int   breath_measured  = 0;                             //Current measured breath
int   breath_threshold = SETTINGS_BREATH_THRESHOLD;     //The threshold for breath to trigger a note
int  breath_ori_measured = 0;
byte breath_changed = 0;                       //bei 1: Anblasdruck hat sich verändert!!!
uint16_t currtouched1 = 0;
uint16_t currtouched2 = 0;
byte Tonart = 0;            //aus EEProm auslesen
byte posi = 0;              //Display: auf welche Zeile kommt Note
int breath_force = 0;                             //Blasstärke
char note_name[6];                       //Notenkürzel 

char str[11];                                      //für OLED
byte display_what = 2;                   //0: Display OFF, 1:breath, 2:note : welches soll angezeigt werden
byte stimmung = 0;                        //4: C-Sax, 3: Sopransax, 1: Altsax; 2: Tenorsax
byte tune = 1;
char Instr[10];
char m_eintrag[10];
int griffweise = 0;
int vorz = 0;

struct fmap_entry {
  uint16_t keys1;
  uint16_t keys2;
  uint8_t midi_note;
  char nn0[6];           //[6]
  char nn1[6]; 
  int vz;  // vorzeichen: 0=keines, 1=vorzeichen vorhanden
  byte b_linie;  // Notenlinie
  float freq;    // Tonfrequenz
};

struct fmap_entry fmap[FMAP_SIZE] = {
  {0b000100011010, 0b000001011100, 46, "B", "B",       1, 126, 116.5409},//nur b, 
  {0b000001011010, 0b000001011100, 47, "H", "H",       0, 126, 123.4708} ,
  {0b000000011010, 0b000001011100, 48, "c0", "c0",     0, 122, 130.8128},  //c 0, in MIDI: C3
  {0b000010011010, 0b000001011100, 49, "cis0", "des0", 1, 122, 138.5913},
  {0b000100011010, 0b000000111100, 49, "cis0", "des0", 1, 122, 138.5913},  //ff
  {0b000000011010, 0b000000011100, 50, "d0", "d0",     0, 118, 146.8324},  //D
  {0b000100011010, 0b000000011100, 50, "d0", "d0",     0, 118, 146.8324},  //D, ff
  {0b000001011010, 0b000000011100, 50, "d0", "d0",     0, 118, 146.8324},  //D, ff
  {0b000000011010, 0b000000111100, 51, "dis0", "es0",  1, 118, 155.5635},
  {0b000000011010, 0b000000001100, 52, "e0", "e0",     0, 114, 164.8138},  //E
  {0b000001011010, 0b000001001100, 52, "e0", "e0",     0, 114, 164.8138},  //E, ff
  {0b000000011010, 0b000000000100, 53, "f0", "f0",     0, 110, 174.6141},  //F
  {0b000000011010, 0b000001010100, 53, "f0", "f0",     0, 110, 174.6141},  //F, ff
  {0b000000011010, 0b000000001000, 54, "fis0", "ges0", 1, 110, 184.9972},
  {0b000000011010, 0b100000000100, 54, "fis0", "ges0", 1, 110, 184.9972},
  {0b000000011010, 0b000000011000, 54, "fis0", "ges0", 1, 110, 184.9972}, //ff
  {0b000000011010, 0b000000000000, 55, "g0", "g0",     0, 106, 195.9977},   //G
  {0b000000011010, 0b100000000000, 55, "g0", "g0",     0, 106, 195.9977},   //G, ff
  {0b000000011010, 0b100000100000, 55, "g0", "g0",     0, 106, 195.9977},   //G, ff
  {0b000000111010, 0b000000000000, 56, "gis0", "as0",  1, 106, 207.6523},
  {0b000000111010, 0b100000000000, 56, "gis0", "as0",  1, 106, 207.6523}, //ff
  {0b000000111010, 0b100000100000, 56, "gis0", "as0",  1, 106, 207.6523}, //ff
  {0b000000001010, 0b000000000000, 57, "a0", "a0",     0, 102, 220.0000},   //A
  {0b000000001010, 0b000000011100, 57, "a0", "a0",     0, 102, 220.0000},   //A, ff
  {0b000000001010, 0b001000000000, 58, "ais0", "b0",   1, 102, 233.0819},
  {0b000000000010, 0b000000000100, 58, "ais0", "b0",   1, 102, 233.0819},
  {0b000000000110, 0b000000000000, 58, "ais0", "b0",   1, 102, 233.0819},
  {0b000100001010, 0b000001011101, 58, "ais0", "b0",   1, 102, 233.0819}, // ff
  {0b000000010110, 0b000000000100, 58, "ais0", "b0",   1, 102, 233.0819}, // ff
  {0b000000011010, 0b001000000000, 58, "ais0", "b0",   1, 102, 233.0819}, // ff
  {0b000000000010, 0b000000000000, 59, "h0", "h0",     0, 98, 246.9417},    //H
  {0b000001011010, 0b000001011101, 59, "h0", "h0",     0, 98, 246.9417},    //H, ff
  {0b000000011010, 0b000100000100, 59, "h0", "h0",     0, 98, 246.9417},    //H, ff
  {0b000000001000, 0b000000000000, 60, "c1", "c1",     0, 94, 261.6256},    //c', in MIDI: C4
  {0b000000000010, 0b000100000000, 60, "c1", "c1",     0, 94, 261.6256},    //c1, MIDI: C4
  {0b000000011010, 0b000001011101, 60, "c1", "c1",     0, 94, 261.6256},    //C4, ff
  {0b000000011000, 0b000000000000, 60, "c1", "c1",     0, 94, 261.6256},    //C4, ff
  {0b000000000000, 0b000000000000, 61, "cis1", "des1", 1, 94, 277.1826},   //
  {0b000010011010, 0b000001011100, 61, "cis1", "des1", 1, 94, 277.1826},   //ff
  {0b010000001010, 0b000000000000, 61, "cis1", "des1", 1, 94, 277.1826},   //ff
  {0b000000011010, 0b000000011101, 62, "d1", "d1",     0, 90, 293.6648}, //D
  {0b000100011010, 0b000000011101, 62, "d1", "d1",     0, 90, 293.6648}, //D, ff
  {0b010000000000, 0b000000000000, 62, "d1", "d1",     0, 90, 293.6648}, //D, ff
  {0b001000000000, 0b000000000000, 62, "d1", "d1",     0, 90, 293.6648}, //D, ff
  {0b000000011010, 0b000000111101, 63, "dis1", "es1",  1, 90, 311.1270},
  {0b000100011010, 0b000000111101, 63, "dis1", "es1",  1, 90, 311.1270}, //ff
  {0b011000000000, 0b000000000000, 63, "dis1", "es1",  1, 90, 311.1270}, //ff
  {0b000000011010, 0b000000001101, 63, "dis1", "es1",  1, 90, 311.1270}, //E
  {0b000100011010, 0b000001001101, 64, "e1", "e1",     0, 86, 329.6276}, //E, ff
  {0b111000000000, 0b000010000000, 64, "e1", "e1",     0, 86, 329.6276}, //E, ff
  {0b000000011010, 0b000000000101, 65, "f1", "e1",     0, 82, 349.2282}, //F
  {0b000000011010, 0b000000010101, 65, "f1", "e1",     0, 82, 349.2282}, //F, ff
  {0b000100011010, 0b000001011101, 65, "f1", "e1",     0, 82, 349.2282}, //F, ff
  {0b000000011010, 0b000000001001, 66, "fis1", "ges1", 1, 82, 369.9944},
  {0b000000011010, 0b100000000100, 66, "fis1", "ges1", 1, 82, 369.9944},
  {0b000000011010, 0b000000011001, 66, "fis1", "ges1", 1, 82, 369.9944}, //ff
  {0b000100011010, 0b000001011101, 66, "fis1", "ges1", 1, 82, 369.9944}, //ff
  {0b000000011010, 0b000000000001, 67, "g1", "g1",     0, 78, 391.9954},   //G
  {0b000000011010, 0b000001011001, 67, "g1", "g1",     0, 78, 391.9954},   //G, ff
  {0b000000011010, 0b000001011101, 67, "g1", "g1",     0, 78, 391.9954},   //G, ff
  {0b000000011010, 0b000000100001, 68, "gis1", "as1",  1, 78, 415.3047},
  {0b000010011010, 0b000001011101, 68, "gis1", "as1",  1, 78, 415.3047},  //ff
  {0b000000001010, 0b000000000001, 69, "a1", "a1",     0, 74, 440.0000},    //A
  {0b000000001010, 0b000000011101, 69, "a1", "a1",     0, 74, 440.0000},  //a'
  {0b000000001010, 0b001000000001, 70, "ais1", "b1",   1, 74, 466.1638},
  {0b000000000010, 0b000000000101, 70, "ais1", "b1",   1, 74, 466.1638},
  {0b000000000110, 0b000000000001, 70, "ais1", "b1",   1, 74, 466.1638},
  {0b000000010110, 0b000000011101, 70, "ais1", "b1",   1, 74, 466.1638}, //ff
  {0b000000010110, 0b001000000101, 70, "ais1", "b1",   1, 74, 466.1638}, //ff
  {0b000100011010, 0b000001011101, 70, "ais1", "b1",   1, 74, 466.1638}, //ff
  {0b000000000010, 0b000000000001, 71, "h1", "h1",     0, 70, 493.8833},   //B
  {0b000000011010, 0b000100000001, 71, "h1", "h1",     0, 70, 493.8833},   //B, ff
  {0b000001011010, 0b000001011101, 71, "h1", "h1",     0, 70, 493.8833},   //B, ff
  {0b000000001000, 0b000000000001, 72, "c2", "c2",     1, 66, 523.2511},    //c'', MIDI:C5
  {0b000000000010, 0b000100000001, 72, "c2", "c2",     1, 66, 523.2511},    //C5
  {0b000000001000, 0b000000011101, 72, "c2", "c2",     1, 66, 523.2511},    //C5, ff
  {0b000000011010, 0b000001011101, 72, "c2", "c2",     1, 66, 523.2511},    //C5, ff
  {0b000000000000, 0b000000000001, 73, "cis2", "des2", 1, 66, 554.3653},
  {0b000000000000, 0b000000011101, 73, "cis2", "des2", 1, 66, 554.3653}, //ff
  {0b000010011010, 0b000001011101, 73, "cis2", "des2", 1, 66, 554.3653}, //ff
  {0b010000000000, 0b000000000001, 74, "d2", "d2",     0, 62, 587.3295}, //D
  {0b010000011000, 0b000000011101, 74, "d2", "d2",     0, 62, 587.3295}, //D, ff
  {0b011000000000, 0b000000000001, 75, "dis2", "es2",  1, 62, 622.2540},
  {0b011000000000, 0b000010000001, 76, "e2", "e2",     0, 58, 659.2551}, //E
  {0b111000000000, 0b000010000001, 77, "f2", "f2",     0, 54, 698.4565}, //F
  {0b000000001001, 0b000000000001, 77, "f2", "f2",     0, 54, 698.4565}, //F
  {0b111000000000, 0b010010000001, 78, "fis2", "ges2", 1, 54, 739.9888}, //F#5
  {0b000000000010, 0b000000101101, 78, "fis2", "ges2", 1, 54, 739.9888}, //F#5
  {0b000000000001, 0b000001000101, 78, "fis2", "ges2", 1, 54, 739.9888}, //F#5
  {0b000000000011, 0b001000000001, 78, "fis2", "ges2", 1, 54, 739.9888}, //F#5
  {0b000000000001, 0b001000000001, 79, "g2", "g2",     0, 50, 783.9909},   //G5
  {0b000000010010, 0b001000000101, 79, "g2", "g2",     0, 50, 783.9909},   //G5
  {0b000000001000, 0b000000101101, 79, "g2", "g2",     0, 50, 783.9909},   //G5
  {0b010000000010, 0b000000101101, 79, "g2", "g2",     0, 50, 783.9909},   //G5
  {0b000000010010, 0b000100100101, 80, "gis2", "as2",  1, 50, 830.6094},
  {0b000000001000, 0b000000101001, 80, "gis2", "as2",  1, 50, 830.6094},
  {0b000000000010, 0b000100101101, 80, "gis2", "as2",  1, 50, 830.6094},
  {0b000000011000, 0b000100111101, 81, "a2", "a2",     0, 46, 880.0000},    //A
  {0b000000011000, 0b000000100001, 81, "a2", "a2",     0, 46, 880.0000},    //A
  {0b000000011000, 0b000001011101, 81, "a2", "a2",     0, 46, 880.0000},    //A
  {0b010000011000, 0b000100111101, 82, "ais2", "b2",   1, 46, 932.3275},
  {0b010000011000, 0b000000100001, 82, "ais2", "b2",   1, 46, 932.3275},
  {0b000000100000, 0b000100000001, 82, "ais2", "b2",   1, 46, 932.3275},
  {0b000000011010, 0b000000111101, 83, "h2", "h2",     0, 42, 987.7666},      //B
  {0b010000001010, 0b000000100101, 83, "h2", "h2",     0, 42, 987.7666},      //B
  {0b010000010000, 0b000100100001, 83, "h2", "h2",     0, 42, 987.7666},      //B
  {0b011000011000, 0b000000010101, 83, "h2", "h2",     0, 42, 987.7666},      //B
  {0b000000001000, 0b000000000010, 84, "c3", "c3",     1, 38, 1046.5023},    //C6 mein
  {0b000000010010, 0b000000100101, 84, "c3", "c3",     1, 38, 1046.5023},    //c''', MIDI: C6
  {0b000000000010, 0b000000101101, 84, "c3", "c3",     1, 38, 1046.5023},    //C6
  {0b011000010000, 0b000000000001, 84, "c3", "c3",     1, 38, 1046.5023},    //C6
  {0b010000001010, 0b000010100101, 84, "c3", "c3",     1, 38, 1046.5023},    //C6
  {0b000000000000, 0b000000000011, 85, "cis3", "des3", 1, 38, 1108.7305},   //mein
  {0b000000000001, 0b000000100101, 85, "cis3", "des3", 1, 38, 1108.7305},
  {0b011000000000, 0b100000000001, 85, "cis3", "des3", 1, 38, 1108.7305},
  {0b010000000000, 0b000000000010, 86, "d3", "d3",     0, 34, 1174.6591}, //D  mein
  {0b000000000001, 0b001000100001, 86, "d3", "d3",     0, 34, 1174.6591}, //D6
  {0b000000011000, 0b000000011101, 86, "d3", "d3",     0, 34, 1174.6591}, //D6
  {0b011000000000, 0b000000000010, 87, "dis3", "es3",  1, 34, 1244.5079},  //mein
  {0b000000001000, 0b000000101001, 87, "dis3", "es3",  1, 34, 1244.5079},
  {0b000000001001, 0b000000000101, 87, "dis3", "es3",  1, 34, 1244.5079},
  {0b000000000001, 0b000100100001, 87, "dis3", "es3",  1, 34, 1244.5079},
  {0b010000011000, 0b000000011101, 87, "dis3", "es3",  1, 34, 1244.5079},
  {0b011000000000, 0b000010000010, 88, "e3", "e3",     0, 30, 1318.5102}, //E  mein
  {0b000000001000, 0b000000101101, 88, "e3", "e3",     0, 30, 1318.5102}, //E6
  {0b111000000000, 0b000010000010, 89, "f3", "f3",     0, 26, 1396.9129}, //F  mein
  {0b000000010010, 0b000000010101, 89, "f3", "f3",     0, 26, 1396.9129}, //F6
  {0b011000001000, 0b000000101101, 89, "f3", "f3",     0, 26, 1396.9129}, //F6
  {0b111000000000, 0b010010000010, 90, "fis3", "ges3", 1, 26, 1479.9777}, //F#5  mein
  {0b000000010010, 0b000000010001, 90, "fis3", "ges3", 1, 26, 1479.9777}, //F#6
  {0b011000011000, 0b000010101101, 90, "fis3", "ges3", 1, 26, 1479.9777}, //F#6
  {0b000000000001, 0b001000000010, 91, "g3", "g3",     0, 26, 1567.9817},   //G5  mein
  {0b000000001000, 0b000000101101, 91, "g3", "g3",     0, 22, 1567.9817},   //G
  {0b000000010010, 0b000100100110, 92, "gis3", "as3",  1, 22, 1661.2188},  //mein
  {0b000000001000, 0b000000101001, 92, "gis3", "as3",  1, 22, 1661.2188},
  {0b000000011000, 0b000100111110, 93, "a3", "a3",     0, 18, 1760.0000},    //A  mein
  {0b010000011000, 0b000000110001, 93, "a3", "a3",     0, 18, 1760.0000},    //A
  {0b010000011000, 0b000100111110, 94, "ais3", "b3",   1, 18, 1864.6550},  //mein
  {0b011000011000, 0b000000111001, 94, "ais3", "b3",   1, 18, 1864.6550},
  {0b000000011010, 0b000000111110, 95, "h3", "h3",     0, 14, 1975.5332},      //B  mein
  {0b011000011000, 0b000010111001, 95, "h3", "h3",     0, 14, 1975.5332},   //B
  {0b000000010010, 0b000000100110, 96, "c4", "c4",     1, 10, 2093.0045},    //C6  mein
  {0b000000000010, 0b000000101101, 96, "c4", "c4",     1, 10, 2093.0045},    //c'''', MIDI: C7
  {0b000000000001, 0b000000100110, 97, "cis4", "des4", 1, 10, 2217.4610},  //mein
  {0b000000000001, 0b001000100010, 98, "d4", "d4",     0, 6, 2349.3182}, //D6  mein
  {0b000000001000, 0b000000101010, 99, "dis4", "es4",  1, 6, 2489.0158},  //mein
  {0b000000001000, 0b000000101110, 100, "e4", "e4",    0, 2, 2637.0204}, //E6  mein
  {0b000000010010, 0b000000010110, 101, "f4", "f4",    0, 0, 2793.8258}, //F6  mein
  {0b000000010010, 0b000000010010, 102, "fis4", "ges4",1, 0, 2959.9554}, //F#6  mein
  {0b000000001000, 0b000000101110, 103, "g4", "g4",    0, 0, 3135.9634},   //G  mein
  {0b000000001000, 0b000000101010, 104, "gis4", "as4", 1, 0, 3322.4376},  //mein
  {0b010000011000, 0b000000110010, 105, "a4", "a4",    0, 0, 3520.0000},    //A  mein
  {0b011000011000, 0b000000111010, 106, "ais4", "b4",  1, 0, 3729.3100},  //mein
  {0b011000011000, 0b000010111010, 107, "h4", "h4",    0, 0, 3951.0664},   //B  mein
  {0b000000000010, 0b000000101110, 108, "c5", "c5",    1, 0, 4186.0090}    //C7  mein
};

//meine vereinfachte Griffweise:
struct fmap_entry fmapNEU[FMAPNEU_SIZE] = {  
  {0b000100011010, 0b000001011100, 46, "B", "B",       1, 126, 116.5409},//nur b, kein # wegen Linie
  {0b000010011010, 0b000001011100, 47, "H", "H",       0, 126, 123.4708} , //H
  {0b000000011010, 0b000001011100, 48, "c0", "c0",     0, 122, 130.8128},  //C3
  {0b000001011010, 0b000001011100, 49, "cis0", "des0", 1, 122, 138.5913}, //cis/des
  {0b000000011010, 0b000000011100, 50, "d0", "d0",     0, 118, 146.8324},  //D
  {0b000000011010, 0b000000111100, 51, "dis0", "es0",  1, 118, 155.5635}, //dis/es
  {0b000000011010, 0b000000001100, 52, "e0", "e0",     0, 114, 164.8138},  //E
  {0b000000011010, 0b000000000100, 53, "f0", "f0",     0, 110, 174.6141},  //F
  {0b000000011010, 0b000000001000, 54, "fis0", "ges0", 1, 110, 184.9972}, //fis/ges
  {0b000000011010, 0b000000000000, 55, "g0", "g0",     0, 106, 195.9977},   //G
  {0b000000111010, 0b000000000000, 56, "gis0", "as0",  1, 106, 207.6523}, //gis/as
  {0b000000001010, 0b000000000000, 57, "a0", "a0",     0, 102, 220.0000},   //A
  {0b000000000110, 0b000000000000, 58, "ais0", "b0",   1, 102, 233.0819}, //ais/b
  {0b000000000010, 0b000000000000, 59, "h0", "h0",     0, 98, 246.9417},    //H
  {0b000000011010, 0b000001011101, 60, "c1", "c1",     0, 94, 261.6256},    //C4/1...1.Oktave
  {0b000000001000, 0b000000000000, 60, "c1", "c1",     0, 94, 261.6256},    //C4/2...1.Oktave
  {0b000001011010, 0b000001011101, 61, "cis1", "des1", 1, 94, 277.1826}, //cis/des
  {0b000000011010, 0b000000011101, 62, "d1", "d1",     0, 90, 293.6648}, //D
  {0b000000011010, 0b000000111101, 63, "dis1", "es1",  1, 90, 311.1270}, //dis/eb
  {0b000000011010, 0b000000001101, 64, "e1", "e1",     0, 86, 329.6276}, //E ..20
  {0b000000011010, 0b000000000101, 65, "f1", "e1",     0, 82, 349.2282}, //F
  {0b000000011010, 0b000000001001, 66, "fis1", "ges1", 1, 82, 369.9944}, //fis/ges
  {0b000000011010, 0b000000000001, 67, "g1", "g1",     0, 78, 391.9954},   //G
  {0b000000111010, 0b000000000001, 68, "gis1", "as1",  1, 78, 415.3047}, //gis/as
  {0b000000001010, 0b000000000001, 69, "a1", "a1",     0, 74, 440.0000},    //A
  {0b000000000110, 0b000000000001, 70, "ais1", "b1",   1, 74, 466.1638}, //ais/b
  {0b000000000010, 0b000000000001, 71, "h1", "h1",     0, 70, 493.8833},      //H
  {0b000000011010, 0b000001011110, 72, "c2", "c2",     1, 66, 523.2511},    //C5...2.Oktave
  {0b000000001000, 0b000000000001, 72, "c2", "c2",     1, 66, 523.2511},    //C5...2.Oktave
  {0b000001011010, 0b000001011110, 73, "cis2", "des2", 1, 66, 554.3653}, //cis/des
  {0b000000011010, 0b000000011110, 74, "d2", "d2",     0, 62, 587.3295}, //D
  {0b000000011010, 0b000000111110, 75, "dis2", "es2",  1, 62, 622.2540}, //dis/es  !!!!!!!!!!!!!!!!
  {0b000000011010, 0b000000001110, 76, "e2", "e2",     0, 58, 659.2551}, //E
  {0b000000011010, 0b000000000110, 77, "f2", "f2",     0, 54, 698.4565}, //F
  {0b000000011010, 0b000000001010, 78, "fis2", "ges2", 1, 54, 739.9888}, //F#5
  {0b000000011010, 0b000000000010, 79, "g2", "g2",     0, 50, 783.9909},   //G
  {0b000000111010, 0b000000000010, 80, "gis2", "as2",  1, 50, 830.6094}, //gis/as
  {0b000000001010, 0b000000000010, 81, "a2", "a2",     0, 46, 880.0000},    //A
  {0b000000000110, 0b000000000010, 82, "ais2", "b2",   1, 46, 932.3275}, //ais/b
  {0b000000000010, 0b000000000010, 83, "h2", "h2",     1, 42, 987.7666}, //H
  {0b000000001000, 0b000000000010, 84, "c3", "c3",     1, 38, 1046.5023} //C6...
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void setup() {
	Serial.begin(115200);
	AudioMemory(240);
	sgtl5000_1.enable();
	sgtl5000_1.volume(1.0);
  wavetable.setInstrument(SAXa);
  mixer.gain(0,0.9);  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  delay(50);
  //für Menü:
  pinMode(li_Pin,INPUT_PULLUP);  //zurück
  pinMode(re_Pin,INPUT_PULLUP);  //vor
  pinMode(mi_Pin,INPUT_PULLUP);  //bestätigen = OK
  pinMode(menu_Pin,INPUT_PULLUP);//wenn EIN: Menüschleife, AUS: Menü verlassen
  
  //settings von EEPROM:
  stimmung = readSetting(STIMM_ADDR);
  display_what = readSetting(DISPWHAT_ADDR);
  Tonart = readSetting(TONART_ADDR);
  griffweise = readSetting(GRIFF_ADDR);
  //int change = 0;
  //int stelle = 1;
  //Serial.print("menu_PIN: ");Serial.print(menu_Pin);
  //********************************************************
  //menu_call();

  if (readSetting(VERSION_ADDR) != VERSION){
    writeSetting(VERSION_ADDR,VERSION);
    writeSetting(STIMM_ADDR,STIMM_FACTORY);
    writeSetting(DISPWHAT_ADDR,DISPWHAT_FACTORY);
    writeSetting(TONART_ADDR,TONART_FACTORY);
    writeSetting(GRIFF_ADDR,GRIFF_FACTORY);
  }
  //settings von EEPROM:
  stimmung = readSetting(STIMM_ADDR);
  display_what = readSetting(DISPWHAT_ADDR);
  Tonart = readSetting(TONART_ADDR);
  griffweise = readSetting(GRIFF_ADDR);
  delay(50);
  Serial.print("Version: ");Serial.println(readSetting(VERSION_ADDR));
  if (stimmung == 0)  {
    strcpy(Instr, "C-SAX");
    tune = 0;
  }
   if (stimmung == 1)  {
    strcpy(Instr, "SopranSAX");
    tune = -2;
  }
   if (stimmung == 2)  {
    strcpy(Instr, "AltSAX");
    tune = -9;
  }
   if (stimmung == 3)  {
    strcpy(Instr, "TenorSAX");
    tune = -14;
  }
  Serial.println(Instr);
  if (griffweise == 0) {
    Serial.println("Griffweise: original Saxophon");
  } else {
    Serial.println("Griffweise: vereinfacht (zwei Oktav-Knöpfe)");
  }
  state = STATE_NOTE_OFF;
  
    cap1.begin(0x5A, &Wire1);
  if (!cap1.begin(0x5A,&Wire1)) {
    Serial.println("MPR121_1 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121_1 found!");
  cap2.begin(0x5B,&Wire1);
  if (!cap2.begin(0x5B,&Wire1)) {
    Serial.println("MPR121_2 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121_2 found!");
  
   if (!u8g2.begin()) Serial.println("Display nicht gefunden");
   if (u8g2.begin())  Serial.println("Display gefunden");
   
  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"SETUP");
  u8g2.drawStr(0,24,Instr);
  u8g2.drawStr(0,38,vorzeichen);
  u8g2.sendBuffer();
  delay(2000);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  Serial.print("display_what: ");Serial.println(display_what);
  testton();
  Serial.print("Ende Setup!");
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void loop() {
  breath();
  wavetable.amplitude(breath_measured / 127.0);
  updateNote();
  
    if (state == STATE_NOTE_OFF) {
      if (breath_ori_measured > breath_threshold) {
        breath_on_time = millis();
        state = STATE_NOTE_NEW;  // Go to next state
      }
      
    } else if (state == STATE_NOTE_NEW) {
      
      if (breath_ori_measured > breath_threshold) {
          breath();
          wavetable.playNote(note_fingered, breath_vol);
          note_playing = note_fingered;
          state = STATE_NOTE_ON;
          //u8g2.clearBuffer();
          //u8g2.sendBuffer();
            //draw_note(note);
        
      } else {
        state = STATE_NOTE_OFF;
        wavetable.stop();
        //u8g2.clearBuffer();
        //u8g2.sendBuffer();
        }
      
    } else if (state == STATE_NOTE_ON) {
      if (breath_ori_measured < breath_threshold) {
        wavetable.stop();
        state = STATE_NOTE_OFF;
        //u8g2.clearBuffer();
        //u8g2.sendBuffer();        
      } else {
        wavetable.amplitude(breath_measured / 127.0); //!!!!!!!!!!!!!!!!!!!!!
        //updateNote();
            if (note_fingered != note_playing) {
            wavetable.stop();
            breath();
            wavetable.playNote(note_fingered, breath_vol);
            note_playing = note_fingered;
            //u8g2.clearBuffer();
            //u8g2.sendBuffer();
            //draw_note(note);
        }
       
      }
    }
   if (millis() - ccSendTime > CC_INTERVAL) {
      breath();
      ccSendTime = millis();
    }
    breath_changed = 0;
  int knob = analogRead(A1);
  float vol = (float)knob / 1023.0;
  sgtl5000_1.volume(vol);
  //Serial.print("  vol: ");Serial.println(vol);
} //loop ende

//********************************************************************************************************
void updateNote() {
  currtouched1 = cap1.touched();
  currtouched2 = cap2.touched();
if (griffweise == 0)  {  //orig. Sax
 for (uint8_t i = 0; i < FMAP_SIZE; i++) {
     if ((currtouched1 == fmap[i].keys1) && (currtouched2 == fmap[i].keys2)) {
      note_fingered = fmap[i].midi_note;
      note = note_fingered;
      vorz = fmap[i].vz;
      if (Tonart == 0)  {  //Kreuztonart
        strcpy(note_name, fmap[i].nn0);
        if (fmap[i].vz == 1) {
          strcpy(vorzeichen,"#");
          }
      posi = fmap[i].b_linie - 8;
      }
      if (Tonart == 1)  {  //b-Tonart
        strcpy(note_name, fmap[i].nn1);
        if (fmap[i].vz == 1) {
          strcpy(vorzeichen,"b");
        }
      posi = fmap[i].b_linie - 4;
      }
      if (stimmung == 1) {
      note_fingered = note_fingered - 2;
      }
      if (stimmung == 2) {
      note_fingered = note_fingered - 9;
      }
      if (stimmung == 3) {
      note_fingered = note_fingered - 14;
      }
      if (note != note_merk)   draw_note(note);
      note_merk = note;
      break;
    }
  }
} 

if (griffweise == 1)  {   //meine vereinfachte Grifweise mit 2 Oktavknöpfen
 for (uint8_t i = 0; i < FMAPNEU_SIZE; i++) {
     if ((currtouched1 == fmapNEU[i].keys1) && (currtouched2 == fmapNEU[i].keys2)) {
      note_fingered = fmapNEU[i].midi_note;
      note = note_fingered;
      vorz = fmap[i].vz;
      if (Tonart == 0)  {
      if (fmap[i].vz == 1) {
        strcpy(note_name, fmap[i].nn0);
          strcpy(vorzeichen,"#");
        }  
      posi = fmapNEU[i].b_linie - 8;
      }
      if (Tonart == 1)  {
        strcpy(note_name, fmap[i].nn1);
      if (fmap[i].vz == 1) {
          strcpy(vorzeichen,"b");
         }  
      posi = fmapNEU[i].b_linie - 4;
      }
      if (stimmung == 1) {
      note_fingered = note_fingered - 2;
      }
      if (stimmung == 2) {
      note_fingered = note_fingered - 9;
      }
      if (stimmung == 3) {
      note_fingered = note_fingered - 14;
      }
      
      if (note != note_merk)   draw_note(note);
      note_merk = note;
      break;
    }
  }
} 
}
//**************************************************************************
void breath()  {
  int value = analogRead(A8);
  if (value > 1023) value = 1023;
  if (value != breath_ori_measured){ // only send midi data if breath has changed from previous value
    breath_changed = 1;
    if (value < SETTINGS_BREATH_THRESHOLD) breath_measured = SETTINGS_BREATH_THRESHOLD;
     breath_measured = map(value, SETTINGS_BREATH_THRESHOLD, 1023, 0, 127);
     breath_vol = breath_measured;
     breath_ori_measured = value;
   }
}

void breath_show_now(int bL)   {
   breath_force = map(bL, 0, 127, 0, 99);
}

void writeSetting(byte address, unsigned short value){
  union {
    byte v[2];
    unsigned short val;
  } data;
  data.val = value;
  EEPROM.write(address, data.v[0]);
  EEPROM.write(address+1, data.v[1]);  
}

unsigned short readSetting(byte address){
  union {
    byte v[2];
    unsigned short val;
  } data;  
  data.v[0] = EEPROM.read(address); 
  data.v[1] = EEPROM.read(address+1); 
  return data.val;
}

void draw_note(int note)    {
 // Serial.println("draw_note");
 u8g2.clearBuffer();
  for (int i=34;i<127;i=i+8)  {  //10
    if (i <70) {
  u8g2.drawLine(24, i, 42, i);  // x,y,x1,y1
  //Serial.print("i <70: ");Serial.println(i);
    }
    if (i>=70 && i<108)  {
      u8g2.drawLine(10, i, 82, i);
      //Serial.print("i>=70 && i<115: ");Serial.println(i);
    }
    if (i>=108)  {
      u8g2.drawLine(24, i, 42, i);
      //Serial.print("i >118: ");Serial.println(i);
    }
    //u8g2.sendBuffer();
  }
      u8g2.drawFilledEllipse(36, posi, 5, 3);
      //if ((vorzeichen == "#")|| (vorzeichen == "b"))  u8g2.drawStr(21, posi-6,vorzeichen);
      //if ((strncmp(vorzeichen, " ", 1)==0)) u8g2.drawStr(21, posi-6,vorzeichen);
      if (vorz == 1)  u8g2.drawStr(21, posi-6,vorzeichen);
      u8g2.drawStr(0, 12, note_name);
    u8g2.sendBuffer();
}


void u8g2_prepare(void) {
  //u8g2.setFont(u8g2_font_lucasfont_alternate_tf);
  //if (displ == 1)  u8g2.setFont(u8g2_font_t0_11b_tf);
  u8g2.setFont(u8g2_font_unifont_tf);
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void print_settings()  {
  /*strcpy(str,"SETTINGS");
  u8g2.drawStr(0,10,str); 
  u8g2.sendBuffer();
  delay(1000);*/
   
    String ss;
    char zeile_eins[10];
    char plus[10];
    char ch_s[20];
    int x;
    Serial.println("Grundeinstellungen");
    Serial.println("");
    Serial.print("Version: ");Serial.println(readSetting(VERSION_ADDR));
    x = readSetting(VERSION_ADDR);
    itoa(x, plus, 10);  //Zahl in String
    strcpy(zeile_eins, "V. ");
    //Serial.print("zwei: ");Serial.print(zwei);Serial.println(x);
    strcat(zeile_eins, plus);
    strcpy(ch_s, zeile_eins);
        
    if (readSetting(STIMM_FACTORY) == 0)   {
    Serial.print("Display-Status: ");Serial.println("OFF");
    }
    
    if (readSetting(DISPWHAT_ADDR) == 2)  {
    Serial.print("Display-Was: ");Serial.println("Breath");
    }
    if (readSetting(TONART_ADDR) == 0)  {
    Serial.print("Tonart: ");Serial.println("#");
    }
    else  {
      Serial.print("Tonart: ");Serial.println("b");
    }
    if (readSetting(GRIFF_ADDR) == 0)  {
    Serial.print("Griffweise: ");Serial.println("normal");
    }
    else  {
    Serial.print("Griffweise: ");Serial.println("vereinfacht");
    }
}
//***************************************************************************
void menu_call() {
  Serial.println("In Menü - Anfang!");
  pinMode(li_Pin,INPUT_PULLUP);  //zurück
  pinMode(re_Pin,INPUT_PULLUP);  //vor
  pinMode(mi_Pin,INPUT_PULLUP);  //bestätigen = OK
  pinMode(menu_Pin,INPUT_PULLUP);//wenn EIN: Menüschleife, AUS: Menü verlassen
  int val = 0;
  val = digitalRead(menu_Pin);
  Serial.print("val: ");Serial.println(val);Serial.println("..............");
  if ((val == 1))  {  //Menü_Schleife !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //  val = 1;
  //wenn sich ein Wert ändert: change = 1
  //strcpy(m_eintrag, "SAX-Wahl:");
  //strcpy(Instr, "C-SAX");
  Serial.print("In Menü!");
  Serial.print(val);
  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"SAX-Wahl:");
  u8g2.drawStr(0,24,"Alt-SAX");
  u8g2.sendBuffer();
  delay(5000);
  u8g2.clearBuffer();
  /*
  if (digitalRead(re_Pin) == HIGH)  { //vor
    stelle = stelle + 1;
    if (stelle > 4) (stelle = 1);
    if (stelle < 1) (stelle = 1);
    
    if (stelle == 1)  (strcpy(Instr, "Alt-SAX"));
    if (stelle == 2)  (strcpy(Instr, "Tenor"));
    if (stelle == 3)  (strcpy(Instr, "Sopran"));
    if (stelle == 4)  (strcpy(Instr, "C-SAX")); 
    u8g2.drawStr(0,24,Instr);
  }
  if (digitalRead(li_Pin) == HIGH)  { //zurück
    if (stelle > 1)  (stelle = stelle - 1);
    if (stelle > 4)  (stelle = 1);
    if (stelle < 1)  (stelle = 1);
    
    if (stelle == 1)  (strcpy(Instr, "Alt-SAX"));
    if (stelle == 2)  (strcpy(Instr, "Tenor"));
    if (stelle == 3)  (strcpy(Instr, "Sopran"));
    if (stelle == 4)  (strcpy(Instr, "C-SAX"));
    u8g2.drawStr(0,24,Instr); 
  }
  u8g2.sendBuffer();
  if (digitalRead(mi_Pin) == HIGH)  { //OK, nächste Ebene
    //stimmung = stelle;  //STIMM_FACTORY
    delay(5000);
  }
  Serial.print("Li: ");Serial.print(li_Pin);Serial.print(" / Re: ");Serial.print(re_Pin);
  Serial.print(" / Mi: ");Serial.print(mi_Pin);Serial.print(" // menu: ");Serial.print(menu_Pin);
  */
 } // if menu_Pin == HIGH
 
if ((val == 0))  {
  //val = 0;
  Serial.print("In Menü - verlasse Menü, keine Aktion");
  Serial.println(val);
  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"LOW");
  u8g2.drawStr(0,24,"val=0");
  u8g2.sendBuffer();
  delay(5000);
  }
}
//***************************************************

void testton()  {
  wavetable.playNote(72,60);
  delay(200);
  wavetable.playNote(60,60);
  delay(400);
  wavetable.playNote(48,60);
  delay(800);
  wavetable.stop();
}

void intro_melody()  {
Serial.println("Intro ANFANG");
wavetable.playNote(48,60);
delay(200);
wavetable.playNote(53,60);
delay(200);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(56,60);
delay(400);
wavetable.playNote(48,60);
delay(400);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(56,60);
delay(100);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(48,60);
delay(100);
wavetable.playNote(46,60);
delay(400);
wavetable.playNote(48,60);
delay(1200);
wavetable.playNote(48,60);
delay(200);
wavetable.playNote(53,60);
delay(200);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(56,60);
delay(400);
wavetable.playNote(48,60);
delay(400);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(48,60);
delay(100);
wavetable.playNote(46,60);
delay(400);
wavetable.playNote(48,60);
delay(1200);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(56,60);
delay(100);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1600);
wavetable.playNote(65,60);
delay(200);
wavetable.playNote(68,60);
delay(400);
wavetable.playNote(65,60);
delay(200);
wavetable.playNote(61,60);
delay(400);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(62,60);
delay(200);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(67,60);
delay(400);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(60,60);
delay(400);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(65,60);
delay(400);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(58,60);
delay(400);
wavetable.playNote(55,60);
delay(200);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(63,60);
delay(400);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(62,60);
delay(200);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(64,60);
delay(200);
wavetable.playNote(65,60);
delay(200);
wavetable.playNote(68,60);
delay(400);
wavetable.playNote(65,60);
delay(200);
wavetable.playNote(61,60);
delay(400);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(62,60);
delay(200);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(67,60);
delay(400);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(60,60);
delay(400);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(65,60);
delay(400);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(58,60);
delay(400);
wavetable.playNote(55,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(63,60);
delay(200);
wavetable.playNote(61,60);
delay(200);
wavetable.playNote(60,60);
delay(1600);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(56,60);
delay(400);
wavetable.playNote(48,60);
delay(400);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(56,60);
delay(100);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(48,60);
delay(100);
wavetable.playNote(46,60);
delay(400);
wavetable.playNote(48,60);
delay(1200);
wavetable.playNote(48,60);
delay(200);
wavetable.playNote(53,60);
delay(200);
wavetable.playNote(56,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(60,60);
delay(200);
wavetable.playNote(59,60);
delay(200);
wavetable.playNote(58,60);
delay(200);
wavetable.playNote(56,60);
delay(400);
wavetable.playNote(48,60);
delay(400);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(56,60);
delay(100);
wavetable.playNote(55,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(400);
wavetable.playNote(53,60);
delay(1200);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(53,60);
delay(100);
wavetable.playNote(51,60);
delay(100);
wavetable.playNote(48,60);
delay(100);
wavetable.playNote(46,60);
delay(400);
wavetable.playNote(48,60);
delay(1600);

wavetable.stop();
Serial.println("Intro ENDE");
}
