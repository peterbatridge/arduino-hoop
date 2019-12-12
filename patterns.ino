#include "LPD8806.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
//#ifdef __AVR_ATtiny85__
//#include <avr/power.h>
//#include <avr/sleep.h>
//#endif

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
int nLEDs = 96;

// Chose 2 pins for output; can be any valid output pins:
//int dataPin  = 2;
//int clockPin = 3;

//int powerPin = //4;
uint8_t upModePin = 3;//A0;
uint8_t upColorPin = 2;
uint8_t upSpeedPin = A2;

uint8_t upModeButtonState = HIGH;
uint8_t upModeButtonCycles = 0;
uint8_t upColorButtonState = HIGH;
uint8_t upColorButtonCycles = 0;
uint8_t upSpeedButtonState = HIGH;
uint8_t upSpeedButtonCycles = 0;
uint8_t upPaletteButtonCycles = 0;
//int sleepCycles = 0;

uint8_t pattern_length = 160;
uint8_t pattern_start = 0;
uint8_t pattern_width = 24;
// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs);//, dataPin, clockPin);

uint8_t mode = 31;
uint8_t color = 1;
uint8_t color_count = 0;
uint8_t freq = 21;
int tickOffset = 0;
bool RandomMode = false;
uint8_t frequencies[12] = {0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144};//, 233, 377};
uint8_t segments[4] = {1, 2, 4, 6};
uint8_t segments_divisible[8] = {2, 3, 4, 6, 8, 12, 12, 12};
uint8_t freq_count = 6;
uint8_t CYCLES_DEBOUNCE = 2; //check the button for X ticks to see if it is bouncing
bool save_freq = false;
uint8_t saved_freq = 21;

uint8_t PatternMode = 0;
uint8_t ColorMode = 1;
uint8_t SpeedMode = 16;
uint8_t PaletteOffset = 0;

unsigned long tick = 0;
unsigned long stripCount = 96;
unsigned long stripCount2 = 96;

void (*fp_zero[16])();
void (*fp_one[16])(uint8_t segments);
void (*fp_two[2])(uint8_t one, uint8_t two);
void (*fp_thr[10])(uint8_t one, uint8_t two, uint8_t three);

uint16_t i, j, x, y ;
uint32_t c, d;
uint8_t one,two,three;
int8_t huff[63];
uint32_t bitmap[150];

void setup() {
//#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 8 (16) MHz on Trinket
//#endif
  one = 0;
  two = 0;
  three = 0;

  strip.updatePins();
  // Start up the LED strip
  strip.begin();

  pinMode(upSpeedPin, INPUT_PULLUP);    // declare pushbutton as input
  pinMode(upModePin, INPUT_PULLUP);    // declare pushbutton as input
  pinMode(upColorPin, INPUT_PULLUP);
  // Update the strip, to start they are all 'off'
  strip.show();
  //Serial.begin(9600);

  randomSeed(analogRead(13));

  fp_zero[0] = rainbowWhole;
  fp_zero[1] = rainbowWholeSparkle;
  fp_zero[2] = rainbowDistributed;
  fp_zero[3] = rainbowDistributedSparkle;
  fp_zero[4] = QuadMorph;
  fp_zero[5] = dither;
  fp_zero[6] = solidColor;
  fp_zero[7] = scanner;
  fp_zero[8] = colorWipe;
  fp_zero[9] = theaterChase;
  fp_zero[10] = theaterChaseRainbow;
  //fp_zero[11] = GoldBlueSparkle;

  fp_one[0] = Deltas; // <--- 11
  fp_one[1] = Deltas;
  fp_one[2] = Deltas;
  fp_one[3] = Deltas;
  fp_one[4] = Strobe;
  fp_one[5] = Strobe; // <--- 16

  fp_two[0] = RainbowWave; // <--- 17
  
  fp_thr[0] = colorChaseSegmentDual; // <--18
  fp_thr[1] = colorChaseSegmentDual;
  fp_thr[2] = colorChaseSegmentDual;
  fp_thr[3] = colorChaseSegmentDual;
  fp_thr[4] = colorChaseSegmentDual;
  fp_thr[5] = colorChaseSegmentDual;
  fp_two[1] = BiColorRainbow; // <---24
  
  fp_thr[6] = TriRGBStrobe; // <---25
  fp_thr[7] = TriRGBStrobe;
  fp_thr[8] = TriRGBStrobe;
  fp_thr[9] = TriRGBStrobe; // <-- 28
  //triggerSleep();
}
int smooth_random = 0;
void NextRandom() {
  /*if(tester_num==0) {
    mode = 6;
    color = 28;
    PaletteOffset = 0;
    tester_num++;
  }
  else if(tester_num==1) {
    mode = 6;
    color = 19;
    PaletteOffset = 0;
    tester_num=0;
  }
  else {
    tester_num = 0;
  }*/
  mode = random(45); // 0-28 normal modes
  if (mode>28) { mode = 6; }
  if (smooth_random>0 && (mode==15 || mode==16 || (mode>24 && mode<29))) {
    mode = (mode + 6) % 30;
  }
  color = random(36) + 1;
  if (smooth_random) {
    color = random(26) + 1;
  }
  if (mode==6 && color<16) {
      color = random(21) + 16;
      if (smooth_random) {
        color = random(11) + 16;
      }
  }
  stripCount = 96;
  uint8_t off = random(2);
  if (off==0) {
    PaletteOffset = 0;
  }
  else{
    PaletteOffset = random(13)+1;
  }
  if (color>15) {
    PatternDecode(color-16);
  }
    /*  Serial.print("Mode: ");
    Serial.print(mode);
    Serial.print(" Color: ");
    Serial.print(color);
    Serial.print(" PaletteOffset: ");
    Serial.println(PaletteOffset);*/
}
void loop() {
  color_count = ((color_count + (4-(24/pattern_width))) % pattern_length);// + pattern_start;
  handleButtons();
  if (RandomMode) {

    /*if (mode>=27 && mode <=29) {
      mode = 6;
    }*/
    if ((mode >= 0 && mode<= 4) || mode == 17 || mode == 24) {
      if ((mode == 18 || mode == 19) && save_freq == false) {
        save_freq = true;
        saved_freq = freq;
        freq = 0;
      }
      if (tick % (384*2) == 0) { 
        freq = saved_freq;
        save_freq = false;
        NextRandom();
      }
    }
    //Only run Dither once
    else if (mode == 5) {
NextRandom();
    }    
    //For solid colors with patterns do something special
    else if (mode == 6) {
      if (tick % (384*2) == 0) {
        NextRandom();
      }
    }
    else if (mode == 7) {
      if (tick % 2 == 0) {  
        colorWipe();
        NextRandom();
      }
    }
    //Color wipe
    else if (mode == 8) {
      if (stripCount == 95) {  
        colorWipe();
        NextRandom();
      }
    }
    //Theather/Rainbow Chase and RGBStrobe/TriStrobe  
    else if (mode == 9 || mode == 10 ||mode == 15 || mode >= 16 || (mode >= 25 && mode <= 28)) {
      if(tick %80==0) {
        NextRandom();
      }
    }
    //Color Chase Segments
    else if (mode >= 18 && mode <= 23) {
      if (tick % 20 == 0) {
        NextRandom();
      }
    }
    //For all other modes give it 4 ticks before changing
    else if (tick % 4 == 0) {
      NextRandom();
    }
    else if(mode==29) {
      NextRandom();
    }
  }
  //Serial.println(mode);
  if (mode >= 0 && mode <= 10) {
    fp_zero[mode]();
  }
  else if (mode >= 11 && mode <= 14) {
    fp_one[mode - 11](mode - 10);
  }
  else if (mode>=15 && mode <=16){
    fp_one[mode -11](mode-15);
  }
  else if (mode == 17) {
    fp_two[mode - 17](5, 2);
  }
  else if (mode >= 18 && mode <= 23) {
    if(mode%2==0) {
      fp_thr[mode - 18](mode - 16, 1,0);  
    }
    else {
      fp_thr[mode - 18](mode - 16, 1,1);
    }
  }
  else if (mode==24) {
    fp_two[mode-23](5,2);
  }
  else if (mode==25) {
    fp_thr[mode-19](2,6,10);
  }
  else if (mode==26) {
    fp_thr[mode-19](9,11,15);
  }
  else if (mode==27) {
    fp_thr[mode-19](4,8,13);
  }
  else if (mode==28) {
    fp_thr[mode-19](7,12,14);
  }

  else if (mode == 29) {
    RandomMode = true;
    smooth_random=0;
  }
  else {
    colorWipe();
  }
  ticker();
}
void shift_presses(uint8_t next) {
  three = two;
  two = one;
  one = next;
  if (one == 3 && two == 2 && three == 1) {
    one = 0;
    two = 0;
    three = 0;
    triggerSleep();
  }
}
void ticker() {
  tick++;
  stripCount++;
  if (stripCount >= strip.numPixels()) {
    stripCount = 0;
  }
  stripCount2++;
  if (stripCount2 >= strip.numPixels() * 2) {
    stripCount2 = 0;
  }
}
bool handleButtons() {
  if (digitalRead(upModePin) != upModeButtonState) {
    upModeButtonCycles++;
    if (upModeButtonCycles > CYCLES_DEBOUNCE) {
      upModeButtonCycles = 0;
      upModeButtonState = digitalRead(upModePin);
      if (upModeButtonState == LOW) {
        triggerModeUp();
        return true;
      }
    }
  }
  // software debounce
  if (digitalRead(upSpeedPin) == LOW && digitalRead(upColorPin) != upColorButtonState) {
    
    upPaletteButtonCycles++;
    if (upPaletteButtonCycles > CYCLES_DEBOUNCE) {
      upPaletteButtonCycles = 0;
      upColorButtonState = digitalRead(upColorPin);
      if (upColorButtonState == LOW) {
          triggerPaletteUp();   
          upSpeedButtonCycles=0;
          upColorButtonCycles=0;
          return true;
      }
    }

  }
  else if (digitalRead(upSpeedPin) != upSpeedButtonState) {
    upSpeedButtonCycles++;
    if (upSpeedButtonCycles > CYCLES_DEBOUNCE) {
      upSpeedButtonCycles = 0;
      upSpeedButtonState = digitalRead(upSpeedPin);
      if (upSpeedButtonState == HIGH) {
        triggerSpeedUp();
        return true;
      }
    }
  }
  else if (digitalRead(upColorPin) != upColorButtonState && digitalRead(upSpeedPin)==HIGH) {
    upColorButtonCycles++;
    if (upColorButtonCycles > CYCLES_DEBOUNCE) {
      upColorButtonCycles = 0;
      upColorButtonState = digitalRead(upColorPin);
      if (upColorButtonState == LOW) {
        triggerColorUp();
      }
    }
  }

  return false;
}
void blackout() {
  for (uint8_t i = 0; i < strip.numPixels() + 1; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}
void ISR_Wake() {}
void triggerSleep() {

  mode = 27;
  color = 1;
  color_count = 0;
  freq = 21;
  tickOffset = 0;
  
  blackout();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0, ISR_Wake, LOW); //pin 2
  attachInterrupt(1, ISR_Wake, LOW); //pin 3
  sleep_mode();
  //sleeping, until rudely interrupted
  sleep_disable();
  detachInterrupt(0);
  detachInterrupt(1);
}
void triggerPaletteUp() {
  PaletteOffset= (PaletteOffset+1)%14;
}
void triggerModeUp() {
  mode = (mode + 1) % 31;
  shift_presses(2);
  if (RandomMode && smooth_random==0) {
    smooth_random = 1;
  }
  else if (RandomMode && smooth_random) {
    RandomMode = false;
    smooth_random = 0;
    mode = 0;
  }
  stripCount = 96;
  blackout();
}
void triggerColorUp() {
  shift_presses(1);
  color = (color + 1) % 37;
  if (color >= 16) {
    PatternDecode(color-16);
  }
  color_count = 0;
  blackout();
}
void triggerSpeedUp() {
  shift_presses(3);
  freq_count = (freq_count + 1) % 12;
  freq = frequencies[freq_count];
}

void rainbowDistributedSparkle() {
  uint16_t i;
  int place1 = random(strip.numPixels());
  for (i = 0; i < strip.numPixels(); i++) {
    if (i == place1) {// ||i == place2){ // { ||i == place3) {// ||i == place4 ||i == place5) {
      strip.setPixelColor(i,Color(1,0));
    }
    else {
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + tick) % 384) );
    }
    handleButtons();
  }
  strip.show();
  delay(freq);
}
/*void GoldBlueSparkle() {
  int place1 = random(strip.numPixels());
  int place2 = random(strip.numPixels());
  int place3 = random(strip.numPixels());
  int place4 = random(strip.numPixels());
  for (int q = 0; q < 3; q++) {
    for (int i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, Wheel(20)); //turn every third pixel on
      if (i == place1 ||i == place2 || i == place3 ||i == place4){ // { ||i == place3) {// ||i == place4 ||i == place5) {
        strip.setPixelColor(i,Color(10,0));
      }
    }

    strip.show();
  
    handleButtons();
    delay(freq);
  
    for (int i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, 0);      //turn every third pixel off
    }
  }
}*/
void rainbowWholeSparkle() {
  uint8_t i, j;
  int place1 = random(strip.numPixels());
  int place2 = random(strip.numPixels());
  for (i = 0; i < strip.numPixels(); i++) {
    if (i == place1 ||i == place2){ // { ||i == place3) {// ||i == place4 ||i == place5) {
      strip.setPixelColor(i,Color(1,0));
    }
    else { 
      strip.setPixelColor(i, Wheel(tick % 384));
    }
    handleButtons();
  }
  strip.show();
  delay(freq);
}
void rainbowWhole() {
  uint8_t i, j;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(tick % 384));
    handleButtons();
  }
  strip.show();
  delay(freq);
}
// Rainbow wheel equally distributed along the chain
void rainbowDistributed() {
  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + tick) % 384) );
    handleButtons();
  }
  strip.show();
  delay(freq);
}
void QuadMorph() {
  MultiColor(Wheel(tick % 384), Wheel( (tick + 128) % 384), Wheel( (tick + 256) % 384), Wheel( (tick + 128) % 384),/* 0, 0, 0, 0,*/ 3, 2, 1, 2/*, 0, 0, 0, 0*/);
  delay(freq);
}
//Fill the entire strip with one color
void solidColor() {
  for (uint8_t i = 0; i < strip.numPixels(); i++) {
    handleButtons();
    strip.setPixelColor(i, Color(color, i));
  }
  strip.show();
  delay(freq);
}
// Fill the dots progressively along the strip.
void colorWipe() {

  strip.setPixelColor(stripCount, Color(color, stripCount));
  strip.show();
  handleButtons();
  delay(freq);
}
// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither() {

  // Determine highest bit needed to represent pixel index
  uint8_t hiBit = 0;
  uint8_t n = strip.numPixels() - 1;
  for (int bit = 1; bit < 0x8000; bit <<= 1) {
    if (n & bit) hiBit = bit;
  }

  uint8_t bit, reverse;
  for (int i = 0; i < (hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for (bit = 1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if (i & bit) reverse |= 1;
    }
    strip.setPixelColor(reverse, Color(color, reverse));
    strip.show();
    if (handleButtons()) {
      return;
    }
    delay(freq);
  }
  delay(250); // Hold image for 1/4 sec
}

// Cylon Eye
void scanner() {
  int i, j, pos, dir;
  uint8_t r, g, b;

  pos = 0;
  dir = 1;
  blackout();


  for (i = 0; i < strip.numPixels() * 2; i++) {
    c = Color(color, pos);
    g = (c >> 16) & 0x7f;
    r = (c >>  8) & 0x7f;
    b =  c        & 0x7f;
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 3, strip.Color(r / 16, g / 16, b / 16));
    strip.setPixelColor(pos - 2, strip.Color(r / 8, g / 8, b / 8));
    strip.setPixelColor(pos - 1, strip.Color(r / 4, g / 4, b / 4));
    strip.setPixelColor(pos, strip.Color(r, g, b));
    strip.setPixelColor(pos + 1, strip.Color(r / 4, g / 4, b / 4));
    strip.setPixelColor(pos + 2, strip.Color(r / 8, g / 8, b / 8));
    strip.setPixelColor(pos + 3, strip.Color(r / 16, g / 16, b / 16));
    strip.show();

    if (handleButtons()) {
      break;
    }
    delay(freq);
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for (j = -3; j <= 3; j++)
      strip.setPixelColor(pos + j, strip.Color(0, 0, 0));
    // Bounce off ends of strip
    pos += dir;
    if (pos < 0) {
      pos = 1;
      dir = -dir;
    } else if (pos >= strip.numPixels()) {
      pos = strip.numPixels() - 3;
      dir = -dir;
    }
  }
}

/*void colorChase() {
  int i;

  // Start by turning all pixels off:
  //for (i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  //for (i = 0; i < strip.numPixels(); i++) {
  strip.setPixelColor(stripCount, Color(color, stripCount)); // Set new pixel 'on'
  strip.show();              // Refresh LED states
  strip.setPixelColor(stripCount, 0); // Erase pixel, but don't refresh!
  handleButtons();
  delay(freq);
  //}

  strip.show(); // Refresh to turn off last pixel
}
void colorChase() {
  int i;

  // Start by turning all pixels off:
  //for (i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  //for (i = 0; i < strip.numPixels(); i++) {
  strip.setPixelColor(stripCount, Color(color, stripCount)); // Set new pixel 'on'
  strip.show();              // Refresh LED states
  strip.setPixelColor(stripCount, 0); // Erase pixel, but don't refresh!
  handleButtons();
  delay(freq);
  //}

  strip.show(); // Refresh to turn off last pixel
}*/
//Theatre-style crawling lights.
void theaterChase() {
  for (uint8_t q = 0; q < 3; q++) {
    for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, Color(color, i + q)); //turn every third pixel on
    }
    strip.show();

    handleButtons();
    delay(freq);

    for (int i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, 0);      //turn every third pixel off
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow() {
  for (uint8_t q = 0; q < 3; q++) {
    for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, Wheel( tick % 384)); //(i+j) % 384));    //turn every third pixel on
    }
    strip.show();
    handleButtons();
    delay(freq);

    for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
      strip.setPixelColor(i + q, 0);      //turn every third pixel off
    }
  }
}
void Strobe(uint8_t RGBorRandom) {
  uint8_t c1 = 2;
  uint8_t c2 = 6;
  uint8_t c3 = 10;
  if (RGBorRandom) {
    c1 = random(15)+1;
    c2 = random(15)+1;
    c3 = random(15)+1;
  }
  for (uint8_t i = 0; i < 3; i++) {
    if (i == 0) {
      TriColor(0,c1);
      TriColor(3,c1);
      TriColor(6,c1);
    }
    if (i == 1) {
      TriColor(0,c2);
      TriColor(3,c2);
      TriColor(6,c2);
    }
    if (i == 2) {      
      TriColor(0,c3);
      TriColor(3,c3);
      TriColor(6,c3);
    }
    if (handleButtons()) {
      return;
    }
    strip.show();
    delay(freq);
  }
}
void TriColor(uint8_t start,uint8_t color) {
  for (int q = start; q < strip.numPixels(); q+=9) {
    strip.setPixelColor(q, Color(color, 0));
    strip.setPixelColor(q+1, Color(color, 0));
    strip.setPixelColor(q+2, Color(color, 0));
  }
}
void TriRGBStrobe(uint8_t c0,uint8_t c1,uint8_t c2) {
  for (uint8_t i = 0; i < 3; i++) {
    if (i == 0) {
      TriColor(0,c0);
      TriColor(3,c1);
      TriColor(6,c2);
    }
    if (i == 1) {
      TriColor(0,c1);
      TriColor(3,c2);
      TriColor(6,c0);
    }
    if (i == 2) {
      TriColor(0,c2);
      TriColor(3,c0);
      TriColor(6,c1);
    }
    if (handleButtons()) {
      return;
    }
    strip.show();
    delay(freq);
  }
}
void MultiColor(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, /*uint32_t c5, uint32_t c6, uint32_t c7, uint32_t c8,*/
                uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4/*, uint8_t l5, uint8_t l6, uint8_t l7, uint8_t l8*/) {
  uint8_t i, n, m;
  for (i = 0; i < strip.numPixels(); i += l1 + l2 + l3 + l4/* + l5 + l6 + l7 + l8*/) {
    for (n = i; n < i + l1; n++) {
      strip.setPixelColor(n, c1);
    }
    for (m = i + l1; m < i + l1 + l2; m++) {
      strip.setPixelColor(m, c2);
    }
    for (m = i + l1 + l2; m < i + l1 + l2 + l3; m++) {
      strip.setPixelColor(m, c3);
    }
    for (m = i + l1 + l2 + l3; m < i + l1 + l2 + l3 + l4; m++) {
      strip.setPixelColor(m, c4);
    }
    /*for (m = i + l1 + l2 + l3 + l4; m < i + l1 + l2 + l3 + l4 + l5; m++) {
      strip.setPixelColor(m, c5);
    }
    for (m = i + l1 + l2 + l3 + l4 + l5; m < i + l1 + l2 + l3 + l4 + l5 + l6; m++) {
      strip.setPixelColor(m, c6);
    }
    for (m = i + l1 + l2 + l3 + l4 + l5 + l6; m < i + l1 + l2 + l3 + l4 + l5 + l6 + l7; m++) {
      strip.setPixelColor(m, c7);
    }
    for (m = i + l1 + l2 + l3 + l4 + l5 + l6 + l7; m < i + l1 + l2 + l3 + l4 + l5 + l6 + l7 + l8; m++) {
      strip.setPixelColor(m, c8);
    }
*/
  }
  strip.show();
}
/*
*
* ONE ARGUMENT FUNCTIONS
*
*/
void Deltas(uint8_t segments) {
  int i, j, pos1, pos2, dir1, dir2;
  int q, seglen;
  pos1 = 0;
  pos2 = strip.numPixels() / segments;
  dir1 = 1;
  dir2 = -1;
  seglen = strip.numPixels() / segments;
  for (i = 0; i <= (strip.numPixels() / segments) * 2; i++) {
    for (q = 0; q < segments; q++) {
      strip.setPixelColor(pos1 + (seglen * q), Color(color, pos1 + (seglen * q)));
      strip.setPixelColor(pos2 + (seglen * q), Color(color, pos2 + (seglen * q)));
    }

    strip.show();
    delay(freq);
    if (handleButtons()) {
      return;
    }
    for (q = 0; q < segments; q++) {
      strip.setPixelColor(pos1 + (seglen * q), strip.Color(0, 0, 0));
      strip.setPixelColor(pos2 + (seglen * q), strip.Color(0, 0, 0));
    }
    // Bounce off ends of strip
    pos1 += dir1;
    pos2 += dir2;
    if (pos1 < 0) {
      pos1 = 0;
      dir1 = -dir1;
    } else if (pos1 >= strip.numPixels() / segments / 2) {
      pos1 = strip.numPixels() / segments / 2;
      dir1 = -dir1;
    }
    if (pos2 < strip.numPixels() / segments / 2) {
      pos2 = strip.numPixels() / segments / 2;
      dir2 = -dir2;
    } else if (pos2 >= strip.numPixels() / segments) {
      pos2 = strip.numPixels() / segments;
      dir2 = -dir2;
    }
  }
}
// Sine wave effect
#define PI 3.14
void Wave(uint8_t cycles) {
  /*float y;
  byte  r, g, b, r2, g2, b2;
  uint32_t q;
  //for (int x = 0; x < (strip.numPixels() * 4); x++)
  //{
    for (int i = 0; i < strip.numPixels(); i++) {
      int t = color % 16;
      if (t==0) { t = 15;}
      q = Color(t, i);
      g = q >> 16 & 0x7f;
      r = q >> 8  & 0x7f;
      b = q       & 0x7f;
      y = sin(PI * (float)cycles * (float)(tick + i) / (float)strip.numPixels());
      if (y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      strip.setPixelColor(i, r2, g2, b2);
    }
    if (handleButtons()) {
      return;
    }
    strip.show();
    delay(freq);
  //}*/
}



/*
*
* TWO ARGUMENT FUNCTIONS
*
*/

void colorChaseSegment(uint8_t segments, uint8_t len) {
  uint8_t i, j, q;
  segments = segments -1;
  segments = segments_divisible[segments];
  // Start by turning all pixels off:
  //for (i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, 0);
  int seg_size = strip.numPixels() / segments;
  // Then display one pixel at a time:
  for (i = 0; i < seg_size; i += len) {
    for (q = 0; q < segments; q++) {
      for (j = q * seg_size + i; j < q * seg_size + i + len; j++) {
        strip.setPixelColor(j, Color(color, j)); // Set new pixel 'on'
      }
    }
    strip.show();
    for (q = 0; q < segments; q++) {
      for (j = q * seg_size + i; j < q * seg_size + i + len; j++) {
        strip.setPixelColor(j, 0);
      }
    }
    if (handleButtons()) {
      return;
    }
    delay(freq);
  }
  strip.show(); // Refresh to turn off last pixel
}
void colorChaseSegmentDual(uint8_t segments, uint8_t len, uint8_t stoppers) {
  uint8_t i, j, q;
  segments = segments -1;
  segments = segments_divisible[segments];
  // Start by turning all pixels off:
  //for (i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, 0);
  int seg_size = strip.numPixels() / segments;
  // Then display one pixel at a time:
  for (i = 0; i < seg_size; i += len) {
    for (q = 0; q < segments; q++) {
      for (j = q * seg_size + i; j < q * seg_size + i + len; j++) {
        if (stoppers && i==0) {
          strip.setPixelColor(j, Color((color+7)%15,j));
        }
        else {
          strip.setPixelColor(j, Color(color, j)); // Set new pixel 'on' 
        }
      }
    }
    strip.show();
    for (q = 0; q < segments; q++) {
      for (j = q * seg_size + i; j < q * seg_size + i + len; j++) {
        if (stoppers && i==0) {
          strip.setPixelColor(j, Color((color+7)%15,j));
        }
        else {
          strip.setPixelColor(j, 0);
        }
      }
    }
    if (handleButtons()) {
      return;
    }
    delay(freq);
  }
  strip.show(); // Refresh to turn off last pixel
}

void BiColorRainbow(uint8_t l1, uint8_t l2) {
  MultiColor(Wheel(tick%384), Wheel( (tick + 192) % 384), 0, 0,/* 0, 0, 0, 0,*/ l1, l2, 0, 0/*, 0, 0, 0, 0*/);
  delay(freq);
}
void RainbowWave(uint8_t cycles, uint8_t fast) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements

  uint32_t wtick = 0;
  //for (int x = 0; x < (strip.numPixels() * 4); x++)
  //{
    wtick += 3;
    for (int i = 0; i < strip.numPixels(); i++) {
      g = (Wheel(tick % 384) >> 16) & 0x7f;
      r = (Wheel(tick % 384) >>  8) & 0x7f;
      b =  Wheel(tick % 384)        & 0x7f;
      wtick += fast*10;
      y = sin(PI * (float)cycles * (float)(tick + i) / (float)strip.numPixels());
      if (y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      strip.setPixelColor(i, r2, g2, b2);
    }
    if (handleButtons()) {
      return;
    }
    strip.show();
    delay(freq);
  //}
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch (WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break;
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break;
    case 2:
      b = 127 - WheelPos % 128;  //blue down
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break;
  }
  return (strip.Color(r, g, b));
}
uint32_t Gradient(uint8_t colors[32], uint8_t lengths[16], bool backward[16], bool repeat) {

  return 0;
}
void FillHuffman(uint8_t comp_length, uint8_t p_width,uint8_t p_length, int8_t * huff_ptr, int8_t * local_huff, uint8_t huff_size, uint32_t * bmp_ptr, uint32_t * local_ptr) {
   pattern_width = p_width;
   pattern_length = p_length;
   //pattern_start = p_start;
   memmove(bmp_ptr, local_ptr, (comp_length*4));
   memmove(huff_ptr, local_huff, huff_size);
   /*for (uint8_t i = 0; i < comp_length; i++) {
    //Serial.println(local_ptr[i]);
    bmp_ptr[i] = local_ptr[i];
    //Serial.print(bmp_ptr[i]);
   }*/
   /*for (int i=0; i < huff_size; i++) {
    huff_ptr[i] = local_huff[i];
   }*/
}
void PatternDecode(uint8_t pattern_number) {
  //Serial.print("PatternDECODE!");
  uint32_t new_bitmap[45]; /*= {

  
          //Fire & Ice
          0xfffff55f, 0xffff3fff, 0xf55ffffc, 
          0x0ffff55f, 0xfff003ff, 0xf55fffc0,
          0x00fff55f, 0xff00803f, 0xf55ffc02, 
          0xa00ff55f, 0xf00aa803, 0xf55fc02a, 
          0xaa00f55f, 0x00aaaa80, 0x355c02aa, 
          0xaaa00550, 0x0aaaaaa8, 0x01402aaa, 
          0xaaaa0000, 0xaaaaaaaa, 0x8002aaaa, 
          0xaaaaa00a, 0xaaaaaaaa, 0xa00aaaaa, 
          0x6aaaa00a, 0xaaa95aaa, 0xa00aaaa5, 
          0x56aaa00a, 0xaa9555aa, 0xa00aaa55, 
          0xd56aa00a, 0xa957f55a, 0xa00aa55f, 
          0xfd56a00a, 0x957fff55, 0xa00a55ff, 
          0xffd56009, 0x57fffff5, 0x50055fff, 
          0xfffd5415, 0x7fffffff, 0x5555ffff,
          0xffffd557, 0xffffffff, 0xf55fffff,

        // Flag
        0xaaaaaabf, 0xffffaaaa, 0xaabfffff,
        0xaaaaaa80, 0x04924900, 0x0a49257f,
        0xffff2492, 0x5fffffd2, 0x49280049,
        0x249000a4, 0x9257ffff, 0xf24925ff,
        0xfffd2492, 0x80049249, 0x000aaaaa,
        0xabfffffa, 0xaaaaabff, 0xfff00000,
        0x0000000f, 0xffffffff, 0xffffffff,
        0xfffffff0, 0x00000000, 0x000fffff,
        0xffffffff, 0xffffffff, 0xfff00000,
        0x0000000f, 0xffffffff, 0xffffffff,
        0xfffffff0,

                //Heart
        0x55555555, 0x55555555, 0x55555555,
        0x5fd557f5, 0x55fd557f, 0x55ffd7ff,
        0x5c00ebff, 0xae0075ff, 0xd703fffe,
        0xb81ffff5, 0xe1ffff5f, 0xffffd7ff,
        0xfff5ffff, 0xfd7fffff, 0x5fffffd5,
        0x7ffff555, 0xffffd555, 0x7fff5555,
        0x5fffd555, 0x55fff555, 0x5555ff55,
        0x555555ff, 0x55555555, 0x5f555555,
        0x5555f555, 0x55400000,

        //rave Diamonds
        0x78a6f78d, 0xb6f6e6dd, 0xe4dbc4db,
        0x3d6c67d8, 0xe658ee6d, 0x8e66d8ce,
        0x6d9e4dbc, 0x4dbbd6f6, 0xfbc6d000,

                //Interlocking 1
        0x2aaaaaa8, 0x7fc2aaaa, 0xaa87fc2a,
        0xaaaaa87f, 0xc01ffe54, 0x3fe00fff,
        0x2a1ff007, 0xffffffff, 0xffc01fff,
        0xffffffff, 0x007fffff, 0xfffffc00,
        0x0547ff80, 0x00a8fff0, 0xaaaaaaa3,
        0xffc2aaaa, 0xaa8fff0a, 0xaaaaaa3f,
        0xfc2aaaaa, 0xa8fff0a8, 0x001ffe15,
        0x0003ffc2, 0xa3ffffff, 0xffffe151,
        0xffffffff, 0xfff0a8ff, 0xffffffff,
        0xf8547ff8, 0x07fc2a3f, 0xfc03fe00, //};//, 
        
//Interlocking 2
0xc3d55555, 0x57c3d555, 0x5557c3d7,
0xfffc3d7f, 0xffc3d7ff, 0xfc3d7fff,
0xc0014003, 0xff000500, 0x0ffff5f0,
0xffff5f0f, 0xfff5f0ff, 0xff555055, 
0x5ffd5541, 0x557fffc3, 0xd7fffc3d,
0x7fffc3d7, 0xfffc3d7c, 0x0000003d, 
0x7c000000, 0x3d7c3fff, 0xd7c3fffd,
0x7c3fffd7};//,
/*
//Linked
0x39659659, 0x672c00bc, 0xed95565e,
0xcb2cb2cb, 0xd955cec0, 0x0b000000, 


// Shapes
0xe5541e4d, 0x9ed23c97, 0xffffc8f2,
0x59edb47b, 0x155680ce, 0x555b91ec,
0x59ed91e4, 0xbffffe47, 0x96d9ed19,
0xe4b555cc, 0x00000000,


//Geodark
0xd5ffffff, 0xd5eaaaaa, 0xd5eaaaaa,
0xd5ebffff, 0xd5eb2492, 0x49d5eb24,
0x9249d5eb, 0x249249d5, 0xeb27fff5,
0x7ac9eaad, 0x5eb27aab, 0x57ac9ebf,
0xffffffff, 0x55555557, 0x55555557,
0xffffff57, 0xaaaaab57, 0xaaaaab57,
0xffffeb57, 0x249249eb, 0x57249249,
0xeb572492, 

//Modified Bounce
0x6fd26c01, 0xb4bdb6fd, 0x26c01b4b,
0xdbdbf49b, 0x6d2f6fdb, 0xf49b6d2f,
0x6ff6fd24, 0xbdbff6fd, 0x24bdbf0f,
0x0ff0f00f, 0x0ff0f048, 0x3c3c3c12,
0x483c3c3c, 0x126d20f0, 0x0f049b6d,
0x20f00f04, 0x9b6d20f0, 0x0f049b6d,
0x20f00f04, 0x9b483c3c, 0x3c12483c,
0x3c3c120f, 0x0ff0f00f, 0x0ff0f0f6,
0xfd24bdbf, 0xf6fd24bd, 0xbfdbf49b,
0x6d2f6fdb, 0xf49b6d2f, 0x6f6fd26c,
0x01b4bdb6, 0xfd26c01b, 0x4bdb0000,
/*
0xfbfff003, 0xffffebff, 0xf3c3ffeb,
0xabffffff, 0xfffaabbf, 0xfffffeba,
0xaabfffff, 0xfeaaaabe, 0xffffeeaa,
0x96baffff, 0xea9656aa, 0xffffeaa5,
0x566aaffa, 0xe965556a, 0xaffaaa55,
0x5169aeba, 0x9a514566, 0xaaaa9550,
0x0556aaa6, 0x95500516, 0x5aa59410,
0x00555aa5, 0x55014c11, 0x5965450c,
0x30415555, 0x410ff0c5, 0x4555431f,
0xf3c10554, 0x43cfffc0, 0x155403ff,
0xffcc1414, 0x34fffffc, 0x00003fff,
0xfffc3000, 0x3ffffffc, 0xf4033fff,
0xfbfff003, 0xffefebff, 0xf3c3ffeb,
0xabffffff, 0xfffaabbf, 0xfffffeba,
0xaabfffff, 0xfeaaa6be, 0xffffeeaa,
0x96baffff, 0xea9656aa, 0xeffbeaa5,
0x566aaffa, 0xe965556a, 0xaffaa955,
0x516aaeba, 0x99410156, 0xaaaa9550,
0x0116aaaa, 0x94440016, 0x5aa99400,
0x00155aa9, 0x54000c10, 0x5969450c,
0x30005555, 0x400ff0c1, 0x4551400f,
0xfcc10554, 0x43cfffc0, 0x451000ff,
0xffcc0400, 0x30fffffc, 0x40403fff,
0xfffc300c, 0x3ffffffc, 0xf0033fff, 
lm firey


//Glow
0x75d75d17, 0x5d77eeba, 0x9f175d7e,
0xe75d175d, 0x75b5d75d, 0x745d70fd,
0xd753e2eb, 0xffdceba2, 0xebadb6d7,
0x5d75d170, 0x1fbaea7c, 0x5ffffb9d,
0x745d6db6, 0xdb5d75d7, 0x4003f75d,
0x4f8e38e2, 0xe75d175d, 0x75cebaeb,
0xaebaebbf, 0xbaea69a6, 0x9a773ae8,
0x00075d76, 0xdb6db6db, 0x6dbfba9f,
0xfffffff7, 0x3a00003a, 0xedb6db6d,
0xb6db6dbf, 0xa7ffffff, 0xfff70000,
0x01d75d75, 0xd75d77ff, 0xffffffff,
0xfeebaeba, 0xebaeb800, 

//Dosio
0xffe7ffff, 0xc3ffff99, 0xffff3cff,
0xfe667ffc, 0xc73ff987, 0x9ff307cf,
0xe607e7cc, 0x07f39807, 0xf93007fc,
0x3007fc98, 0x07f9cc07, 0xf3e607e7,
0xf307cff9, 0x879ffcc7, 0x3ffe667f,
0xff3cffff, 0x99ffffc3, 0xffffe7ff, 

//Aztec
0xfffd36db, 0x6dc0003e, 0xbfdcfcf5,
0xb6db7720, 0x675dfbb9, 0x3cd7bfdc,
0x80057db6, 0xdb6e4002, 0xbedb6db7,
0x3fafffe4, 0x002bedb6, 0xdb7279af,
0x7fb9033a, 0xefddcfcf, 0x5b6db770,
0x000faff7, 0xfffd36db, 0x6dc0003e,
0xbfdcfcf5, 0xb6db7720, 0x675dfbb9,
0x3cd7bfdc, 0x80057db6, 0xdb6e4002,
0xbedb6db7, 0x3fafffe4, 0x002bedb6,
0xdb7279af, 0x7fb9033a, 0xefddcfcf,
0x5b6db770, 0x000faff7,

//Squares
0x56ab007a, 0xb55802b5, 0x6ab0057f,
0xffeace49, 0x249357ff, 0x26affe4d,
0x59d5593f, 0x3bdf004e, 0xf6a80277,
0xb54013aa, 0xb57cfffa, 0xc926ac9d,
0x6493564f, 0xfeaf8000,

//Square Flower
0xffffbaaa, 0xaadfffff, 0xfffb9279,
0x24db911e, 0x224db910, 0xe024dbff,
0xf6fffdbf, 0xff6e4438, 0x0936e447,
0x88936e49, 0xe4937fff, 0xffffeeaa,
0xaab7fffc, 

//Triskelion (possible decrease size of this)
0xbc33c318, 0x70030070, 0x07807807,
0xc03e03fc, 0x3fc3fe1f, 0xe3ff3ff3,
0xc63c6382, 0x782705f0, 0xdf0ce0ce,
0x1c01c01e, 0x01e01f00, 0xf00ff0ff,
0x8ff87f87, 0xfc7fcf9c, 0xf99f09f0,
0x7e17e100,

//Violets
0xfffffe7f, 0xfdbffdbf, 0xf18ff66f,
0xc423b99d, 0xb99dc423, 0xf66ff18f,
0xfdbffdbf, 0xfe7fffff, 

//Halfie
0xebf33bec, 0xd72f3bcf, 0x3b35cbef,
0x33ebc000, 

//Squiggle
0xaaaaaac9, 0x24924927, 0x3f9fcff9,
0xd6759d79, 0xd6759d7f, 0xafd7ebd2,
0x4924924b, 0xaaaaaaea, 0xaaaab000,
0x000000fc, 0x7e3f1ef1, 0xbc6f1ef1,
0xbc6f1eff, 0x7fbff6db, 0x6db6dbea,
0xaaaab000, 

//Dark Bounce
0xfdbc47db, 0xf8755fea, 0xb87dba51,
0x1a5dbfdb, 0xa511a5db, 0xf66dbad7,
0x6d9b4bfe, 0x23ff4bf7, 0xffdfff99,
0xe01ccfdf, 0xbf27c9ef, 0xddfbf27c,
0x9efdf27f, 0x5afc9efe, 0x1e01e1ef,
0xbf878078, 0x7bf27f5a, 0xfc9dfbf2,
0x7c9efddf, 0xbf27c9ef, 0xdfe67807,
0x33ffdfff, 0x7fd2ff88, 0xffd2d9b6,
0xeb5db66f, 0xdba511a5, 0xdbfdba51,
0x1a5dbe1d, 0x57faae1f, 0xdbc47dbf*/

//};
  //int8_t fireice [7] = {-1, -1, -1, 9, 2, 10, 3};
  //int8_t heart [7] = { -1, -1, 15, 1, 0, -1, -1};//Heart
  //int8_t flag [7] = { -1, 1, -1, -1, -1, 10, 2};//flag 
  //int8_t diamonds[15] = {-1, -1, -1, -1, 8, 0, 12, 12, 13, -1, -1, -1, -1, -1, -1};
  //int8_t interlock1[15]={-1, 0, -1, -1, -1, 5, -1, -1, -1, -1, -1, -1, -1, 12, 14};
  //int8_t interlock2[7] = {-1, -1, 0, 5, 12, -1, -1};
  //int8_t linked[7] = {-1, -1, 0, 12, 14, -1, -1};
  int8_t shapes[31] =  {-1, 0, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8, 9};
  int8_t geodark[15] = {-1, -1, -1, -1, 12, 10, 0, 10, 8, -1, -1, -1, -1, -1, -1};
  int8_t modifiedbounce[15] = {-1, -1, 0, 2, -1, -1, -1, -1, -1, 4, 13, -1, -1, -1, -1};

  //lmFirey : [-1, -1, -1, 2, 3, 1, 0]
  int8_t glow[15] = {-1, 0, -1, -1, -1, -1, 11, -1, -1, -1, -1, 0, 1, -1, -1};
  //int8_t dosio[15] =  {-1, 8, 0};
  //int8_t aztec[15] =  {-1, -1, 0, 3, -1, -1, -1, -1, -1, 15, 5, -1, -1, -1, -1};
  int8_t squares[15] = {-1, -1, 0, -1, 8, -1, -1, 9, 1, -1, -1, -1, -1, -1, -1};
  int8_t squareflower[31] = {-1, -1, 0, -1, 1, -1, -1, -1, 9, -1, -1, -1, -1, -1, -1, 4, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int8_t triskelion[3] = {-1, 10, 0};
  int8_t rainbow[31] = {-1, -1, -1, -1, -1, -1, -1, 14, 13, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 9, 6, 8, 11, 12, 2, 15};
  int8_t halfie[7] = {-1, -1, 0, 4, 10, -1, -1};
  int8_t squiggle[15] = {-1, -1, -1, 8, -1, -1, 1, -1, -1, 14, 0, 12, 5, -1, -1};
  int8_t darkbounce[31] = {-1, -1, 0, -1, -1, -1, -1, 13, -1, 9, -1, -1, -1, -1, -1, -1, -1, 8, 8, -1, -1, 11, 1, -1, -1, -1, -1, -1, -1, -1, -1};
  //int8_t slide[7] = {-1, -1, 0, 10, 11, -1, -1};
  int8_t circles[15] = {-1, -1, -1, -1, -1, -1, 0, 9, 12, 2, 8, 11, 15, -1, -1};
  int8_t chase_bounce[7] = {-1, -1, 0, 2, 10, -1, -1};
  //int8_t half_blue[63] = {-1, -1, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, -1, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int8_t checks[63] = {-1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, 8, 12, 2, 13, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  if (pattern_number==0) {
    uint32_t local[45] = {
      //Rainbow
      0xe4eae633, 0x48ff0772, 0x26e9ae00
    };
    FillHuffman(3,24,3,huff,rainbow,31,new_bitmap,local);
  }
  else if (pattern_number==1) {
      uint32_t local[45] = {
          //Fire & Ice
          0xfffff55f, 0xffff3fff, 0xf55ffffc, 
          0x0ffff55f, 0xfff003ff, 0xf55fffc0,
          0x00fff55f, 0xff00803f, 0xf55ffc02, 
          0xa00ff55f, 0xf00aa803, 0xf55fc02a, 
          0xaa00f55f, 0x00aaaa80, 0x355c02aa, 
          0xaaa00550, 0x0aaaaaa8, 0x01402aaa, 
          0xaaaa0000, 0xaaaaaaaa, 0x8002aaaa, 
          0xaaaaa00a, 0xaaaaaaaa, 0xa00aaaaa, 
          0x6aaaa00a, 0xaaa95aaa, 0xa00aaaa5, 
          0x56aaa00a, 0xaa9555aa, 0xa00aaa55, 
          0xd56aa00a, 0xa957f55a, 0xa00aa55f, 
          0xfd56a00a, 0x957fff55, 0xa00a55ff, 
          0xffd56009, 0x57fffff5, 0x50055fff, 
          0xfffd5415, 0x7fffffff, 0x5555ffff,
          0xffffd557, 0xffffffff, 0xf55fffff
      };
      int8_t local_palette [31] = {-1, -1, -1, 9, 2, 10, 3};
      FillHuffman(45,24,90, huff, local_palette,7, new_bitmap, local);
  }  
  else if (pattern_number==2) {
    uint32_t local[45] = {
      //Modified Bounce
      0x7f4d8003, 0x6beefe9b, 0x006d7df7,
      0xf4d836be, 0xfefe9b6d, 0x7dff7f4d,
      0xafbf3dfd, 0x2fbf03cf, 0xfcf041e7,
      0xf9e0920f, 0x3f3c1269, 0x079e7824,
      0xdb483ccf, 0x049b6da4, 0x1e1e0936,
      0xdb690787, 0x824db6d2, 0x0f33c126,
      0xda03cf3c, 0x1269079f, 0x9e0920f3,
      0xfcf041e7, 0xfe781efe, 0x97df9efe,
      0x9b5f7fdf, 0xd36dafbf, 0xbfa6c1b5,
      0xf7dfd360, 0x0dafbbfa, 0x6c001b5f,
      0x60000000
    };

    int8_t local_palette [31] = {};
    FillHuffman(31,24,72,huff,modifiedbounce,15,new_bitmap,local);
  }
  else if(pattern_number==3) {
      uint32_t local[45] = {
        //Triangles
        0xcafe8caf, 0xe8f2ba3f, 0x2ba37951,
        0xef2a3cf2, 0x3c791e8f, 0x1e478f28,
        0xf7951ef2, 0x51f94a3f, 0x28a32851,
        0x94142814, 0x2828ca14, 0x65147e52,
        0x8fcaa3de, 0x547bca3c, 0x791e3c3c,
        0x8f1e479e, 0x547bca8f, 0xf2ba3f2b, 
        0xa3000000
      };
      int8_t local_palette [31] = {-1, 12, -1, -1, -1, 11, 15};//-1, 0, -1, -1, -1, 5, -1, -1, -1, -1, -1, -1, -1, 12, 14};
      FillHuffman(19,24,48,/*30,24,63,*/ huff, local_palette,7/*15*/, new_bitmap, local);
  }
  else if(pattern_number==4) {
      uint32_t local[45] = {
        //rave Diamonds
        0x78a6f78d, 0xb6f6e6dd, 0xe4dbc4db,
        0x3d6c67d8, 0xe658ee6d, 0x8e66d8ce,
        0x6d9e4dbc, 0x4dbbd6f6, 0xfbc6d000
      };
      int8_t local_palette [31] = {-1, -1, -1, -1, 8, 0, 12, 12, 13, -1, -1, -1, -1, -1, -1};
      FillHuffman(9,8,16,huff,local_palette,15, new_bitmap, local);
  }
  
  else if(pattern_number==5) {
      uint32_t local[45] = {
        //Heart
        0x55555555, 0x55555555, 0x55555555,
        0x5fd557f5, 0x55fd557f, 0x55ffd7ff,
        0x5c00ebff, 0xae0075ff, 0xd703fffe,
        0xb81ffff5, 0xe1ffff5f, 0xffffd7ff,
        0xfff5ffff, 0xfd7fffff, 0x5fffffd5,
        0x7ffff555, 0xffffd555, 0x7fff5555,
        0x5fffd555, 0x55fff555, 0x5555ff55,
        0x555555ff, 0x55555555, 0x5f555555,
        0x5555f555, 0x55400000
      };
      int8_t local_palette [31] = { -1, -1, 15, 1, 0, -1, -1};
      FillHuffman(26,24,72,huff,local_palette,7, new_bitmap, local);
  }

      else if(pattern_number==6) {
     uint32_t local[45] = {
      //Double to single bounce 1
      0xffebffff, 0xf77ffffb, 0xeffffdfd,
      0xfffeffbf, 0xff7ff7ff, 0xbffeffdf,
      0xffdfefff, 0xfbf7ffff, 0x7bffffed,
      0xfffffdff, 0xfffcffff, 0xfcfffffc,
      0xfffffcff, 0xfffcffff, 0xfcfffffc,
      0xfffffcff, 0xfffcffff, 0xfcfffffc,
      0xfffffcff, 0xfffcffff, 0xfcfffffc,
      0xfffffcff, 0xfffcffff, 0xfcfffffc,
      0xfffffcff, 0xfffcffff, 0xfcfffffd,
      0xfffffdbf, 0xfffef7ff, 0xff7effff,
      0xbfdfffdf, 0xfbffefff, 0x7ff7ffef,
      0xfbfffdfd, 0xffffbeff, 0xfff77ff0};

      int8_t local_palette [31] = {-1, -1, 0, 10, 2, -1, -1};
      FillHuffman(36,24,135, huff, local_palette, 7, new_bitmap, local);
  }
  else if (pattern_number==7) {
    uint32_t local[45] = {
    //Double 2 single bounce 2
     0xffe1ffff, 0xf33ffff9, 0xe7fffcfc,
     0xfffe7f9f, 0xff3ff3ff, 0x9ffe7fcf,
     0xffcfe7ff, 0xf9f3ffff, 0x39ffffe4,
     0xfffffcff, 0xfffdffff, 0xfdfffffd,
     0xfffffdff, 0xfffdffff, 0xfdfffffd,
     0xfffffdff, 0xfffdffff, 0xfdfffffd,
     0xfffffd7f, 0xfffeefff, 0xff7dffff,
     0xbfbfffdf, 0xf7ffeffe, 0xfff7ffdf,
     0xfbfffbfd, 0xffff7eff, 0xffef7fff,
     0xfdbfffff, 0xb3fffffc, 0xffffff3f,
     0xffffcfff, 0xfff3ffff, 0xfcffffff,
     0x3fffffcf, 0xfffff3ff, 0xfffcfff8
    };
    FillHuffman(36,24,135,huff,chase_bounce,15,new_bitmap,local);
  }
  else if (pattern_number==8) {
    uint32_t local[45] = {
    // Slide
    0xfeff3fbf, 0xcfeff3fb, 0xfcfeff3f,
    0xbfcfeff3, 0xfbfc3fbf, 0xe7f7fcfe, 
    0xff9fdff3, 0xfbfe7f7f, 0xcfeff9fd
    };
    int8_t local_palette[31] = {-1, -1, 0, 10, 11, -1, -1};
    FillHuffman(9,16,32,huff,local_palette,7,new_bitmap,local);
  }
    else if (pattern_number==9) {
    uint32_t local[45] = {
      //Dark Bounce
      0x3ff3f9e7, 0xcfce7f9f, 0x4ff4fa69,
      0xd3d29fa7, 0x2ff2f965, 0xcbca5f97,
      0x77fbbeed, 0xdddeebbf, 0x777ffbfe,
      0xfdfdfefb, 0xff7f1ff1, 0xf8e3c7c6,
      0x3f8f0ff0, 0xf861c3c2, 0x1f876ff6,
      0xfb6ddbda, 0xdfb75ff5, 0xfaebd7d6,
      0xbfaf0000, 
    };
    FillHuffman(19,8,45,huff,checks,63,new_bitmap,local);
  }  
    else if (pattern_number==10) {
    uint32_t local[45] = {
      //Triskelion
      0xbc33c318, 0x70030070, 0x07807807,
      0xc03e03fc, 0x3fc3fe1f, 0xe3ff3ff3,
      0xc63c6382, 0x782705f0, 0xdf0ce0ce,
      0x1c01c01e, 0x01e01f00, 0xf00ff0ff,
      0x8ff87f87, 0xfc7fcf9c, 0xf99f09f0,
      0x7e17e100
    };
    FillHuffman(16,24,63,huff,triskelion,3,new_bitmap,local);
  }
      else if (pattern_number==11) {
    uint32_t local[45] = {
      //Circles
      0xfd25ff92, 0x7ff007fa, 0xb6afcc90,
      0xfe1b63d5, 0xb6d59924, 0x870db6c6,
      0xadb6acc9, 0x24386db6, 0x356db566,
      0x4921c36d, 0xb1eadabf, 0x3243f86d,
      0x8ff497fe, 0x49ffc01e 
    };
    FillHuffman(14,24,24,huff,circles,15,new_bitmap,local);
  }
    else if (pattern_number==12) {
    uint32_t local[45] = {
      //Dark Bounce
      0xfdbccfdb, 0xf9755fea, 0xb97dbacc,
      0xf5dbfb7d, 0x4cebedf6, 0xedd6bb76,
      0xebfe67ff, 0x55f1ff8f, 0xafbdf917,
      0x7bfc71df, 0xf7e3818f, 0xbfbfc637,
      0xfeb5fdf8, 0xf2f22f2f, 0x18f2f22f,
      0x2f1dffad, 0x7f7c63ef, 0xeff181c7,
      0x7fdf8e3e, 0xf7e45def, 0xf5f1ff8f,
      0xaaff99ff, 0xd76edd6b, 0xb76fb7d4,
      0xcebedfdb, 0xaccf5dbe, 0x5d57faae,
      0x5fdbccfd, 0xbf000000
    };
    FillHuffman(29,24,72,huff,darkbounce,31,new_bitmap,local);
  }  
  else if (pattern_number==13) {
    uint32_t local[45] = {
      //glow
      0x75d75d17, 0x5d77eeba, 0x9f175d7e,
      0xe75d175d, 0x75b5d75d, 0x745d70fd,
      0xd753e2eb, 0xffdceba2, 0xebadb6d7,
      0x5d75d170, 0x1fbaea7c, 0x5ffffb9d,
      0x745d6db6, 0xdb5d75d7, 0x4003f75d,
      0x4f8e38e2, 0xe75d175d, 0x75cebaeb,
      0xaebaebbf, 0xbaea69a6, 0x9a773ae8,
      0x00075d76, 0xdb6db6db, 0x6dbfba9f,
      0xfffffff7, 0x3a00003a, 0xedb6db6d,
      0xb6db6dbf, 0xa7ffffff, 0xfff70000,
      0x01d75d75, 0xd75d77ff, 0xffffffff,
      0xfeebaeba, 0xebaeb800
    };
    int8_t local_palette [31] = {};
    FillHuffman(35,24,72,huff,glow,15,new_bitmap,local);
  }

  else if (pattern_number==14) {
    uint32_t local[45] = {
      //Linked
      0x39659659, 0x672c00bc, 0xed95565e,
      0xcb2cb2cb, 0xd955cec0, 0x0b000000
    };
    int8_t local_palette [31] = {-1, -1, 0, 12, 14, -1, -1};
    FillHuffman(6,8,14,huff,local_palette,7,new_bitmap,local);
  }
    else if (pattern_number==15) {
    uint32_t local[45] = {
      // Shapes
      0xe5541e4d, 0x9ed23c97, 0xffffc8f2,
      0x59edb47b, 0x155680ce, 0x555b91ec,
      0x59ed91e4, 0xbffffe47, 0x96d9ed19,
      0xe4b555cc, 0x00000000
    };
      int8_t local_palette [31] = {};
    FillHuffman(11,8,20,huff,shapes,31,new_bitmap,local);
  }
  else if (pattern_number==16) {
    uint32_t local[45] = {
      //Geodark
      0xd5ffffff, 0xd5eaaaaa, 0xd5eaaaaa,
      0xd5ebffff, 0xd5eb2492, 0x49d5eb24,
      0x9249d5eb, 0x249249d5, 0xeb27fff5,
      0x7ac9eaad, 0x5eb27aab, 0x57ac9ebf,
      0xffffffff, 0x55555557, 0x55555557,
      0xffffff57, 0xaaaaab57, 0xaaaaab57,
      0xffffeb57, 0x249249eb, 0x57249249,
      0xeb572492
    };
      int8_t local_palette [31] = {};
    FillHuffman(22,16,50,huff,geodark,15,new_bitmap,local);
  }

  else if (pattern_number==17) {
    uint32_t local[45] = {
      //Squares
      0x56ab007a, 0xb55802b5, 0x6ab0057f,
      0xffeace49, 0x249357ff, 0x26affe4d,
      0x59d5593f, 0x3bdf004e, 0xf6a80277,
      0xb54013aa, 0xb57cfffa, 0xc926ac9d,
      0x6493564f, 0xfeaf8000
    };
    FillHuffman(14,16,32,huff,squares,15,new_bitmap,local);
  }
  else if (pattern_number==18) {
    uint32_t local[45] = {
      //Squareflower
      0xffffbaaa, 0xaadfffff, 0xfffb9279,
      0x24db911e, 0x224db910, 0xe024dbff,
      0xf6fffdbf, 0xff6e4438, 0x0936e447,
      0x88936e49, 0xe4937fff, 0xffffeeaa,
      0xaab7fffc
    };
    FillHuffman(13,16,34,huff,squareflower,31,new_bitmap,local);
  }


  else if (pattern_number==19) {
    uint32_t local[45] = {
      //Halfie
      0xebf33bec, 0xd72f3bcf, 0x3b35cbef,
      0x33ebc000
    };
    FillHuffman(4,8,11,huff,halfie,7,new_bitmap,local);
  }
 else if(pattern_number==20) {
      uint32_t local[45] = {
        // Flag
        0xaaaaaabf, 0xffffaaaa, 0xaabfffff,
        0xaaaaaa80, 0x04924900, 0x0a49257f,
        0xffff2492, 0x5fffffd2, 0x49280049,
        0x249000a4, 0x9257ffff, 0xf24925ff,
        0xfffd2492, 0x80049249, 0x000aaaaa,
        0xabfffffa, 0xaaaaabff, 0xfff00000,
        0x0000000f, 0xffffffff, 0xffffffff,
        0xfffffff0, 0x00000000, 0x000fffff,
        0xffffffff, 0xffffffff, 0xfff00000,
        0x0000000f, 0xffffffff, 0xffffffff,
        0xfffffff0
      };
      int8_t local_palette [31] = { -1, 1, -1, -1, -1, 10, 2};
      FillHuffman(31,24,78, huff, local_palette,7, new_bitmap, local);
  }
 for (uint16_t i = 0; i < (sizeof(bitmap)/sizeof(uint32_t)); i++) {
    bitmap[i] = 0;
 }
 uint32_t bit_pos = 0;
 uint32_t offset =0;
   
  for (int i = 0; i <8*pattern_length; i++) {
    int16_t c = -1;
    uint32_t bit_block = 0;
    uint8_t index = 0;
    //Serial.print(i);
    //Serial.print(" <-i, ");
    //Serial.print(8*pattern_length);
    //Serial.print(" <-Pattern length * 8 ");
    while (c==-1) {
      c = huff[index];
      if (c!=-1) {
        break;
      }
      bit_block = new_bitmap[bit_pos/32 + pattern_start];
      if ((bit_block << (bit_pos%32)) & 0x80000000) {
        //Serial.println("1");
        index = 2*index + 2;
      }
      else {
        //Serial.println("0");
        index = 2*index+1;
      }
      bit_pos++;
    }
    //Serial.print(huff[index]);
    //Serial.print(" goes: at ");
    //Serial.print(i/8);
    //Serial.print(" pos: ");
    //Serial.println((28- 4*(i%8)));
    //Serial.println(((huff[index]) << (28 - 4*(i%8))));
    bitmap[(i/8)] |=  (((uint32_t)huff[index]) << (28 - 4*(i%8)));
    //Serial.println(bitmap[i/8], HEX);
  }
   //for (int i = 0; i<4; i++) {
   // Serial.println(bitmap[i], HEX);
   //}
  //Serial.println("Made it here");

}
uint32_t Pattern(int c, int pos) {
  //c = c + pattern_start;
  pos = pos % pattern_width;
  //if (pos >= 0 && pos < 8) {
    //Serial.print("Start!");
    //Serial.println(pos);
    //Serial.println(pos/8);
    //Serial.println((28-((pos-((pos/8)*8)) * 4)));
    uint32_t color = 0x0000000F & (bitmap[c + (pos/8)] >> (28-((pos-((pos/8)*8)) * 4)));
    //Serial.println(color);
    if (PaletteOffset>0) {
      if (color!=0x00000000 && color!=0x00000001) { 
        color = ((color+PaletteOffset) % 14)+2;
      }
    }
    return Color(/*0x0000000F & (bitmap[c] >> (28-(pos * 4)))*/color, 0);
  /*}
  else if (pos >= 8 && pos < 16) {
    uint32_t color =0x0000000F & (bitmap[c + 1] >> (28-((pos - 8) * 4)));
    if (PaletteOffset>0) {
      if (color!=0x00000000 && color!=0x00000001) { 
        color = ((color+PaletteOffset) % 13)+2;
      }
    }
    return Color(/*0x0000000F & (bitmap[c + 1] >> (28-((pos - 8) * 4)))*//*color, 0);
  }
  else if (pos >= 16 && pos < 24) {
    uint32_t color =0x0000000F & (bitmap[c + 2] >> (28-((pos - 16) * 4)));
    if (PaletteOffset>0) {
      if (color!=0x00000000 && color!=0x00000001) { 
        color = ((color+PaletteOffset) % 13)+2;
      }
    }
    return Color(/*0x0000000F & (bitmap[c + 2] >> (28-((pos - 16) * 4)))*//*color, 0);
  }*/
}
uint32_t Color(int c, int pos)
{
  switch (c) {
    case 0: // Black
      return strip.Color(0, 0, 0);
    case 1: // White
      return strip.Color(127, 127, 127);
    /*Color Wheel*/
    case 2: // Red
      return strip.Color(127, 0, 0);
    case 3: // Orange
      return strip.Color(127, 32, 0);
    case 4: // Yellow
      return strip.Color(127, 127, 0);
    case 5: // Yellow-Green
      return strip.Color(32 , 127, 0);
    case 6: // Green
      return strip.Color(0, 127, 0);
    case 7: // Cyan-Green
      return strip.Color(0, 127, 32);
    case 8: // Cyan
      return strip.Color(0, 127, 127);
    case 9: // Cyan-Blue
      return strip.Color(0, 32, 127);
    case 10:// Blue
      return strip.Color(0, 0, 127);
    case 11:// Purple
      return strip.Color(32, 0, 100);
    case 12:// Light Purple
      return strip.Color(88, 0, 127);
    case 13:// Light Pink
      return strip.Color(127, 0, 127);
    case 14:// Pink
      return strip.Color(127, 0, 88);
    case 15:// Dark Pink
      return strip.Color(127, 0, 32);
    default:
      if (c > 15 && c<37) {
         return Pattern((color_count), pos);
      }
      return strip.Color(0, 0, 0);
  }
}
