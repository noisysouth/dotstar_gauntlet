#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_DotStarMatrix.h>
#include <Adafruit_DotStar.h>
#include <Fonts/TomThumb.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;

//#define DEBUG_SCREEN
//#define DEBUG_TRACK
//#define DEBUG_CONTROLS
//#define DEBUG_XY_COLOR
//#define DEBUG_DEATH
//#define DEBUG_FASTER

// Joy Featherwing registers, with joystick on left (upside-down)
#define BUTTON_UP    7
#define BUTTON_DOWN  10
#define BUTTON_LEFT  9
#define BUTTON_RIGHT 6
#define BUTTON_SEL   14
uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | 
                (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);

// Joy Featherwing (optional/not currently connected)
#if defined(ESP8266)
  #define IRQ_PIN   2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define IRQ_PIN   14
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define IRQ_PIN   27
#elif defined(TEENSYDUINO)
  #define IRQ_PIN   8
#elif defined(ARDUINO_ARCH_WICED)
  #define IRQ_PIN   PC5
#else
  #define IRQ_PIN   5
#endif

// DotStar LED matrix comm
#if defined(ESP8266)
  #define DATAPIN    13
  #define CLOCKPIN   14
#elif defined(__AVR_ATmega328P__)
  #define DATAPIN    2
  #define CLOCKPIN   4
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define DATAPIN    7
  #define CLOCKPIN   16
#elif defined(TEENSYDUINO)
  #define DATAPIN    9
  #define CLOCKPIN   5
#elif defined(ARDUINO_ARCH_WICED)
  #define DATAPIN    PA4
  #define CLOCKPIN   PB5
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define DATAPIN    27
  #define CLOCKPIN   13
#else // // 32u4, M0, M4, esp32-s2, nrf52840 and 328p
  #define DATAPIN    11
  #define CLOCKPIN   13
#endif

// scrolling on DotStar LED matrix
#define SHIFTDELAY 100

// sound module: Adafruit Music Maker FeatherWing
// (3357 headphone version, or 1788 3W stereo amp version)
// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather ESP8266
#if defined(ESP8266)
  #define VS1053_CS      16     // VS1053 chip select pin (output)
  #define VS1053_DCS     15     // VS1053 Data/command select pin (output)
  #define CARDCS          2     // Card chip select pin
  #define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP32
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define VS1053_CS      32     // VS1053 chip select pin (output)
  #define VS1053_DCS     33     // VS1053 Data/command select pin (output)
  #define CARDCS         14     // Card chip select pin
  #define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

// Feather Teensy3
#elif defined(TEENSYDUINO)
  #define VS1053_CS       3     // VS1053 chip select pin (output)
  #define VS1053_DCS     10     // VS1053 Data/command select pin (output)
  #define CARDCS          8     // Card chip select pin
  #define VS1053_DREQ     4     // VS1053 Data request, ideally an Interrupt pin

// WICED feather
#elif defined(ARDUINO_STM32_FEATHER)
  #define VS1053_CS       PC7     // VS1053 chip select pin (output)
  #define VS1053_DCS      PB4     // VS1053 Data/command select pin (output)
  #define CARDCS          PC5     // Card chip select pin
  #define VS1053_DREQ     PA15    // VS1053 Data request, ideally an Interrupt pin

#elif defined(ARDUINO_NRF52832_FEATHER )
  #define VS1053_CS       30     // VS1053 chip select pin (output)
  #define VS1053_DCS      11     // VS1053 Data/command select pin (output)
  #define CARDCS          27     // Card chip select pin
  #define VS1053_DREQ     31     // VS1053 Data request, ideally an Interrupt pin

// Feather RP2040
#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  #define VS1053_CS       8     // VS1053 chip select pin (output)
  #define VS1053_DCS     10     // VS1053 Data/command select pin (output)
  #define CARDCS          7     // Card chip select pin
  #define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

// Feather M4, M0, 328, ESP32S2, nRF52840 or 32u4
#else
  #define VS1053_CS       6     // VS1053 chip select pin (output)
  #define VS1053_DCS     10     // VS1053 Data/command select pin (output)
  #define CARDCS          5     // Card chip select pin
  // DREQ should be an Int pin *if possible* (not possible on 32u4)
  #define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

#endif

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// MATRIX DECLARATION:
// Parameter 1 = width of DotStar matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   DS_MATRIX_TOP, DS_MATRIX_BOTTOM, DS_MATRIX_LEFT, DS_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     DS_MATRIX_TOP + DS_MATRIX_LEFT for the top-left corner.
//   DS_MATRIX_ROWS, DS_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   DS_MATRIX_PROGRESSIVE, DS_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type:
//   DOTSTAR_BRG  Pixels are wired for BRG bitstream (most DotStar items)
//   DOTSTAR_GBR  Pixels are wired for GBR bitstream (some older DotStars)
//   DOTSTAR_BGR  Pixels are wired for BGR bitstream (APA102-2020 DotStars)

#define SCREEN_WIDTH 12
#define SCREEN_HEIGHT 6

Adafruit_DotStarMatrix matrix = Adafruit_DotStarMatrix(
                                  SCREEN_WIDTH, SCREEN_HEIGHT, DATAPIN, CLOCKPIN,
                                  DS_MATRIX_BOTTOM     + DS_MATRIX_LEFT +
                                  DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
                                  DOTSTAR_BGR);

#define RED_BITS 5
#define RED_MASK ((1<<RED_BITS)-1)
#define GREEN_BITS 6
#define GREEN_MASK ((1<<GREEN_BITS)-1)
#define BLUE_BITS 5
#define BLUE_MASK ((1<<BLUE_BITS)-1)

#define RED_SHIFT (GREEN_BITS+BLUE_BITS)
#define GREEN_SHIFT (BLUE_BITS)
#define BLUE_SHIFT 0

#define COLOR_MAX  255

#define COLOR_BLACK   (matrix.Color(  0,   0,   0))
#define COLOR_RED     (matrix.Color(255,   0,   0))
#define COLOR_GREEN   (matrix.Color(  0, 255,   0))
#define COLOR_BLUE    (matrix.Color(  0,   0, 255))
#define COLOR_YELLOW  (matrix.Color(200, 255,   0))
#define COLOR_CYAN    (matrix.Color(  0, 255, 225))
#define COLOR_PINK    (matrix.Color(255,   0, 220))
#define COLOR_WHITE   (matrix.Color(255, 255, 255))
#define COLOR_WALL    COLOR_RED //(matrix.Color( 32,   8,   0)) // 0x200800 must not have any blue, must have red
#define COLOR_PLAYER  COLOR_GREEN
#define COLOR_PELLET  COLOR_BLUE
#define COLOR_EXPLOSION COLOR_YELLOW
#define COLOR_MULTI   (matrix.Color(  0,   0,   1)) // special value: use value near black
#define COLOR_SPARKLE (matrix.Color(100, 100, 100)) // special value

const uint16_t primaryColors[] = {
  COLOR_RED, COLOR_GREEN, COLOR_BLUE
};

enum bright_e {
  bright_dim = 1,
  bright_mid = 2,
  bright_max = 20, // too bright
};

uint16_t screen_dots[SCREEN_WIDTH][SCREEN_HEIGHT];

void print_int(int int_val) {
  if (int_val < 10000) {
    Serial.print(" ");
  }
  if (int_val < 1000) {
    Serial.print(" ");
  }
  if (int_val < 100) {
    Serial.print(" ");
  }
  if (int_val < 10) {
    Serial.print(" ");
  }
  Serial.print(int_val);
}
void print_hex(int int_val) {
  if (int_val < 0x1000) {
    Serial.print("0");
  }
  if (int_val < 0x100) {
    Serial.print("0");
  }
  if (int_val < 0x10) {
    Serial.print("0");
  }
  Serial.print(int_val, HEX);
}

void print_screen(void) {
  int print_x, print_y;
  uint16_t dot_color;
  
  // column labels
  Serial.print("   ");
  for (print_x = 0; print_x < SCREEN_WIDTH; print_x++) {
    //print_int(print_x);
    print_hex(print_x);
    Serial.print(" ");
  }
  Serial.println(); // end of row: column labels

  //for (print_y = 0; print_y < SCREEN_HEIGHT; print_y++) {
  for (print_y = SCREEN_HEIGHT-1; print_y >= 0; print_y--) {
    // row labels
    Serial.print(print_y);
    Serial.print(": ");
    for (print_x = 0; print_x < SCREEN_WIDTH; print_x++) {
      dot_color = screen_dots[print_x][print_y];
      print_hex(dot_color);
      Serial.print(" ");
    }
    Serial.println(); // end of row
  }
}

// given in_x and in_y on screen, return index into string of LEDs that makes up the screen.
int xy_to_idx(int in_x, int in_y) {
  return in_x + (in_y * SCREEN_WIDTH);
}
void set_xy_color(int loc_x, int loc_y, uint16_t in_color) {
  int loc_idx;
  int   red_part,   red_scaled;
  int green_part, green_scaled;
  int  blue_part,  blue_scaled;

  loc_idx = xy_to_idx(loc_x, loc_y);
  red_part   = (in_color >>   RED_SHIFT) &   RED_MASK;
  green_part = (in_color >> GREEN_SHIFT) & GREEN_MASK;
  blue_part  = (in_color >>  BLUE_SHIFT) &  BLUE_MASK;
  red_scaled   = COLOR_MAX *   red_part /   RED_MASK;
  green_scaled = COLOR_MAX * green_part / GREEN_MASK;
  blue_scaled  = COLOR_MAX *  blue_part /  BLUE_MASK;
#ifdef DEBUG_XY_COLOR
  Serial.print("color:"); 
  Serial.print(in_color);
  Serial.print(", parts: r:"); 
  Serial.print(red_part);
  Serial.print(", g:"); 
  Serial.print(green_part);
  Serial.print(", b:");
  Serial.print(blue_part);
  Serial.print(", scaled: r:"); 
  Serial.print(red_scaled);
  Serial.print(", g:"); 
  Serial.print(green_scaled);
  Serial.print(", b:");
  Serial.println(blue_scaled);
#endif
  matrix.setPixelColor(loc_idx, red_scaled, green_scaled, blue_scaled);
}

void show_screen(void) {
  int draw_x, draw_y;
  uint16_t draw_color;

  for (draw_x = 0; draw_x < SCREEN_WIDTH; draw_x++) {
    for (draw_y = 0; draw_y < SCREEN_HEIGHT; draw_y++) {
      draw_color = screen_dots[draw_x][draw_y];
      set_xy_color (draw_x, draw_y, draw_color);
    }
  }
  matrix.show();
}

#define TRACK_START 4
int offset_max = 0;
void add_track(int offset) {
  uint16_t track_arr[] = {
    COLOR_WALL, COLOR_WALL, COLOR_WALL, COLOR_WALL,
    COLOR_WALL, COLOR_WALL, COLOR_WALL, COLOR_WALL,
    COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, 
    COLOR_WALL, COLOR_WALL, COLOR_WALL, COLOR_WALL,
    COLOR_WALL, COLOR_WALL, COLOR_WALL, COLOR_WALL,
  };
  int old_y, new_y;
  int dot_x;
  int track_count, track_mid, track_idx;

  // shift world contents toward bottom of screen (y=0)
  for (old_y = 1; old_y < SCREEN_HEIGHT; old_y++) {
      new_y = old_y-1;
      for (dot_x = 0; dot_x < SCREEN_WIDTH; dot_x++) {
        screen_dots[dot_x][new_y] = screen_dots[dot_x][old_y];
      }
  }

  // add new row at top of screen (SCREEN_HEIGHT-1)
  new_y = SCREEN_HEIGHT-1;
  track_count = sizeof(track_arr) / sizeof(track_arr[0]);
  track_mid = track_count / 2;
  if (offset_max == 0) {
    offset_max = track_mid-1;
  }
#ifdef DEBUG_TRACK
  Serial.print("track_mid: ");
  Serial.print(track_mid);
  Serial.print(", track_count: ");
  Serial.print(track_count);
  Serial.print(", track_idx: ");
#endif
  for (dot_x = 0; dot_x < SCREEN_WIDTH; dot_x++) {
    //track_idx = dot_x + track_mid - SCREEN_WIDTH/2 + offset;
    track_idx = dot_x + offset;
    screen_dots[dot_x][new_y] = track_arr[track_idx];
#ifdef DEBUG_TRACK
    Serial.print(track_idx);
    Serial.print(", ");
#endif
  }
#ifdef DEBUG_TRACK
  Serial.println();
#endif
#ifdef DEBUG_SCREEN
  print_screen();
#endif
}

int player_x = SCREEN_WIDTH/2, player_y = SCREEN_HEIGHT/2; // start in center
int track_offset = TRACK_START;
int player_pellets = 0;
bool player_dead = false;

// start with first character and shift them left (-x) to see more
int x_start = matrix.width();
int x_shift = x_start; // start at position for doing -1 (move to the left) direction.
int track_count = 0; // increment every 10ms. when it reaches track_delay, advance the track.
int track_delay = 15; // start slow
int faster_count = 0; // increment every time track_count reaches track_delay. when it reaches faster_delay, speed up.
int faster_delay = 100;
int pellet_count = 0; // increment every 10ms. when it reaches pellet_delay, possibly add pellet.
int pellet_delay = 15;
int death_count = 0; // increment every 100ms. when it reaches death_delay, stop death animation.
int death_delay = 20;

void start_game(void) {
  int init_x, init_y;

  for (init_x = 0; init_x < SCREEN_WIDTH; init_x++) {
    for (init_y = 0; init_y < SCREEN_HEIGHT; init_y++) {
      screen_dots[init_x][init_y] = COLOR_BLACK;
    }
  }

  player_pellets = 0;
  player_dead = false;
  player_x = SCREEN_WIDTH/2;
  player_y = SCREEN_HEIGHT/2; // start in center
  track_offset = TRACK_START;
  track_count = 0;
  track_delay = 15; // reset speed to slow
  pellet_count = 10; // start about to maybe do a pellet
  pellet_delay = 15; // reset possible pellet frequency to 150msec
  faster_count = 0; // reset speeding up wait timer
  death_count = 0; // reset death animation
  x_shift = x_start; // reset death scrolling text
  
  for (init_y = 0; init_y < SCREEN_HEIGHT; init_y++) {
    add_track(track_offset); // start with straight track
  }
  screen_dots[player_x][player_y] = COLOR_PLAYER;
#ifdef DEBUG_SCREEN
  print_screen();
#endif
  show_screen();
  musicPlayer.startPlayingFile("/ding.mp3");
}

/// File listing helper
// print files on SD card, which should include sound files for this game
void printDirectory(File dir, int numTabs) {
   while(true) {

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

void init_sound() {
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  Serial.println(F("VS1053 found"));

  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");

  // list files
  printDirectory(SD.open("/"), 0);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(10,10);

#if defined(__AVR_ATmega32U4__)
  // Timer interrupts are not suggested, better to use DREQ interrupt!
  // but we don't have them on the 32u4 feather...
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
#else
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
#endif
}

void setup() {
#if defined DEBUG_SCREEN || defined DEBUG_CONTROLS || defined DEBUG_TRACK
  Serial.begin(115200);

  while(!Serial) {
    delay(10);
  }

  Serial.println("Joy FeatherWing basic");
  Serial.println("with Dotstar Matrix Wing");
#endif
  init_sound();

  if(!ss.begin(0x49)){
#ifdef DEBUG_CONTROLS
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
#endif
  } else {
#ifdef DEBUG_CONTROLS
    Serial.println("seesaw started");
    Serial.print("version: ");
    Serial.println(ss.getVersion(), HEX);
#endif
  }
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  //pinMode(IRQ_PIN, INPUT); // Note: Can't do this, or it stops all sound effects from playing afterwards.
  
  matrix.begin();  
  matrix.setFont(&TomThumb);
  matrix.setTextWrap(false);
  matrix.setBrightness(bright_dim);

  for (byte i = 0; i < 3; i++) {
    matrix.fillScreen(primaryColors[i]);
    matrix.show();
    delay(500);
  }
  matrix.fillScreen(0);
  matrix.show();

  start_game();
}

void
check_player(void) {
  if (screen_dots[player_x][player_y] == COLOR_PELLET) {
    player_pellets++;
    musicPlayer.startPlayingFile("/bubble.mp3");
  }
  if (screen_dots[player_x][player_y] == COLOR_WALL) {
    screen_dots[player_x][player_y] = COLOR_EXPLOSION;
    player_dead = true;
    musicPlayer.startPlayingFile("/birds.mp3");
  } else {
    // show player again
    screen_dots[player_x][player_y] = COLOR_PLAYER;
  }
  show_screen();
}

int last_x = 0, last_y = 0;
bool last_left = false;
bool last_right = false;
bool last_up = false;
bool last_down = false;
bool last_sel = false;

void loop() {
  int joy_x = ss.analogRead(2);
  int joy_y = ss.analogRead(3);
  int draw_x, draw_y;
  uint16_t draw_color;
  
  bool btn_left = false;
  bool btn_right = false;
  bool btn_up = false;
  bool btn_down = false;
  bool btn_sel = false;
  int move_x = 0, move_y = 0;

  if (player_dead) {
    if (death_count == 0) {
      matrix.setTextColor(COLOR_GREEN);
#ifdef DEBUG_DEATH
      Serial.print("player_pellets: ");
      Serial.println(player_pellets);
#endif
    }
    matrix.fillScreen(0);
    matrix.setCursor(x_shift, 5); // move text position each time thru this loop
    matrix.print(player_pellets);
    matrix.show();
    delay(100);
    
    x_shift--; // move text over next time (scroll from off-screen right towards the left, so left-most characters seen first)
    death_count++;
    if (death_count >= death_delay) {
      start_game();
    }
    return;
  } // end if(player_dead)

  track_count++;
  if (track_count >= track_delay) {
    track_offset = track_offset + random(-1,2);
    if (track_offset < 0) {
      track_offset = 0;
    }
    if (track_offset > offset_max) {
      track_offset = offset_max;
    }
    // erase where the player was
    screen_dots[player_x][player_y] = COLOR_BLACK;
    add_track (track_offset); // shift everything up, add 1 row at bottom
    check_player();
    if (player_dead) {
      delay(10);
      return;
    }
    faster_count++;
#ifdef DEBUG_FASTER
    Serial.print("faster_count: ");
    Serial.println(faster_count);
#endif
    if (faster_count >= faster_delay) {
      track_delay--;
#ifdef DEBUG_FASTER
      Serial.print("track_delay: ");
      Serial.println(track_delay);
#endif
      faster_count = 0;
    }
    track_count = 0;
  }

  pellet_count++;
  if (pellet_count >= pellet_delay) {
    int pellet_rand;

    pellet_rand = random(1,20);
    if (pellet_rand == 1) {
      int pellet_x, pellet_y;

      // add new pellet at top of screen (SCREEN_HEIGHT-1)
      pellet_x = random(8,12) - track_offset; // place in black part of track
      pellet_y = SCREEN_HEIGHT-1;
      screen_dots[pellet_x][pellet_y] = COLOR_PELLET;
    }
    pellet_count = 0;
  }

  if ( (abs(joy_x - last_x) > 3)  ||  (abs(joy_y - last_y) > 3)) {
#ifdef DEBUG_CONTROLS
    Serial.print(joy_x); Serial.print(", "); Serial.println(joy_y);
#endif
    if (joy_x > last_x) {
      btn_up = true;
    }
    if (joy_x < last_x) {
      btn_down = true;
    }
    if (joy_y > last_y) {
      btn_right = true;
    }
    if (joy_y < last_y) {
      btn_left = true;
    }

    last_x = joy_x;
    last_y = joy_y;
  }
  
  /* if(!digitalRead(IRQ_PIN)) {  // Uncomment to use IRQ */

    uint32_t buttons = ss.digitalReadBulk(button_mask);

    //Serial.println(buttons, BIN);

    if (! (buttons & (1 << BUTTON_LEFT))) {
      //Serial.println("Left Button pressed");
      btn_left = true;
    }
    if (! (buttons & (1 << BUTTON_RIGHT))) {
      //Serial.println("Right Button pressed");
      btn_right = true;
    }
    if (! (buttons & (1 << BUTTON_UP))) {
      //Serial.println("Up Button pressed");
      btn_up = true;
    }
    if (! (buttons & (1 << BUTTON_DOWN))) {
      //Serial.println("Down Button pressed");
      btn_down = true;
    }
    if (! (buttons & (1 << BUTTON_SEL))) {
      //Serial.println("SEL Button pressed");
      btn_sel = true;
    }

    if (btn_left && !last_left) {
      move_x = -1;
    }
    if (btn_right && !last_right) {
      move_x =  1;
    }
    if (btn_up && !last_up) {
      move_y = -1;
    }
    if (btn_down && !last_down) {
      move_y =  1;
    }

    if (move_x != 0 || move_y != 0) { // player is moving
      // erase where thc player was
      screen_dots[player_x][player_y] = COLOR_BLACK;
      //musicPlayer.startPlayingFile("/track001.mp3");
    }
    player_x += move_x;
    if (player_x < 0) {
      player_x = 0;
    }
    if (player_x >= SCREEN_WIDTH) {
      player_x = SCREEN_WIDTH-1;
    }
    player_y += move_y;
    if (player_y < 0) {
      player_y = 0;
    }
    if (player_y >= SCREEN_HEIGHT) {
      player_y = SCREEN_HEIGHT-1;
    }
    if (move_x != 0 || move_y != 0) { // player is moving
      check_player();
      if (player_dead) {
        delay(10);
        return;
      }
#ifdef DEBUG_SCREEN
      print_screen();
#endif
#ifdef DEBUG_CONTROLS
      Serial.print("player_x: "); // say where the player is now
      Serial.print(player_x);
      Serial.print(", y: ");
      Serial.println(player_y);
#endif
    }

    //last_buttons = buttons;
    last_left = btn_left;
    last_right = btn_right;
    last_up = btn_up;
    last_down = btn_down;
    last_sel = btn_sel;
  /* } // Uncomment to use IRQ */
  delay(10);
}