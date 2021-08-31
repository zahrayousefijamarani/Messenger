#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <EEPROM.h>
#include <TheThingsNetwork.h>



#define EEPROM_SIZE 500
#define IsWithin(x, a, b) ((x>=a)&&(x<=b))
#define TS_MINX 142
#define TS_MINY 125
#define TS_MAXX 3871
#define TS_MAXY 3727

#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#define Threshold 40 /* Greater the value, more the sensitivity */

// ---------------------------- constants --------------------
int wakeup_time = 60; // seconds
const char textLimit = 500;
// ----------------------------- variables -------------------
TaskHandle_t Task1;
long lastTimeTouched = 0;
char MyBuffer[textLimit];
bool canSend = false;
String sendingString;
bool id_not_set = false;
String ID = "";

// ---------------------------- LoRa ------------------------
#define loraSerial Serial1
#define debugSerial Serial

// ABP method
const char *devAddr = "00000000";
const char *nwkSKey = "00000000000000000000000000000000";
const char *appSKey = "00000000000000000000000000000000";


#define freqPlan TTN_FP_AS923_925

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

// ---------------------------- keyboard character ----------
const char Mobile_KB[3][13] PROGMEM = {
  {0, 13, 10, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {1, 12, 9, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'},
  {3, 10, 7, 'Z', 'X', 'C', 'V', 'B', 'N', 'M'},
};

const char Mobile_NumKeys[3][13] PROGMEM = {
  {0, 13, 10, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
  {0, 13, 10, '-', '/', ':', ';', '(', ')', '$', '&', '@', '"'},
  {5, 8, 5, '.', '\,', '?', '!', '\''}
};

const char Mobile_SymKeys[3][13] PROGMEM = {
  {0, 13, 10, '[', ']', '{', '}', '#', '%', '^', '*', '+', '='},
  {4, 9, 6, '_', '\\', '|', '~', '<', '>'}, //4
  {5, 8, 5, '.', '\,', '?', '!', '\''}
};

// ---------------- functions -----------------

void message(const byte* payload, size_t length, port_t port) {
  debugSerial.println("-- MESSAGE");
  debugSerial.print("Received " + String(length) + " bytes on port " + String(port) + ":");

  String in = "";
  for (int i = 0; i < length; i++) {
    in += " " + String(payload[i]);
  }
  debugSerial.println(in);
  tft.fillScreen(ILI9341_BLUE);
  tft.setCursor(21, 195);
  tft.print(in);

}

void MakeKB_Button(const char type[][13])
{
  tft.setTextSize(2);
  tft.setTextColor(0xffff, 0xf000);
  for (int y = 0; y < 3; y++)
  {
    int ShiftRight = 15 * pgm_read_byte(&(type[y][0]));
    for (int x = 3; x < 13; x++)
    {
      if (x >= pgm_read_byte(&(type[y][1]))) break;

      drawButton(15 + (30 * (x - 3)) + ShiftRight, 100 + (30 * y), 20, 25); // this will draw the button on the screen by so many pixels
      tft.setCursor(20 + (30 * (x - 3)) + ShiftRight, 105 + (30 * y));
      tft.print(char(pgm_read_byte(&(type[y][x]))));
    }
  }
  //ShiftKey
  drawButton(15, 160, 35, 25);
  tft.setCursor(27, 168);
  tft.print('^');

  //Special Characters
  drawButton(15, 190, 35, 25);
  tft.setCursor(21, 195);
  tft.print(F("SP"));

  //BackSpace
  drawButton(270, 160, 35, 25);
  tft.setCursor(276, 165);
  tft.print(F("BS"));

  //Return
  drawButton(270, 190, 35, 25);
  tft.setCursor(276, 195);
  tft.print(F("RT"));

  //Spacebar
  drawButton(60, 190, 200, 25);
  tft.setCursor(105, 195);
  tft.print(F("SPACE BAR"));
}

void drawButton(int x, int y, int w, int h)
{
  // grey
  tft.fillRoundRect(x - 3, y + 3, w, h, 3, 0x8888); //Button Shading

  // white
  tft.fillRoundRect(x, y, w, h, 3, 0xffff);// outter button color

  //red
  tft.fillRoundRect(x + 1, y + 1, w - 1 * 2, h - 1 * 2, 3, 0xf800); //inner button color
}

void GetKeyPress(char * textBuffer)
{
  char key = 0;
  static bool shift = false, special = false, back = false, lastSp = false, lastSh = false;
  static char bufIndex = 0;

  if (!ts.bufferEmpty())
  {
    lastTimeTouched = millis();
    //ShiftKey
    if (TouchButton(15, 160, 35, 25))
    {
      shift = !shift;
      delay(200);
    }

    //Special Characters
    if (TouchButton(15, 190, 35, 25))
    {
      special = !special;
      delay(200);
    }

    if (special != lastSp || shift != lastSh)
    {
      if (special)
      {
        if (shift)
        {
          tft.fillScreen(ILI9341_BLUE);
          MakeKB_Button(Mobile_SymKeys);
        }
        else
        {
          tft.fillScreen(ILI9341_BLUE);
          MakeKB_Button(Mobile_NumKeys);
        }
      }
      else
      {
        tft.fillScreen(ILI9341_BLUE);
        MakeKB_Button(Mobile_KB);
        tft.setTextColor(0xffff, 0xf800);
      }

      if (special)
        tft.setTextColor(0x0FF0, 0xf800);
      else
        tft.setTextColor(0xFFFF, 0xf800);

      tft.setCursor(21, 195);
      tft.print(F("SP"));

      if (shift)
        tft.setTextColor(0x0FF0, 0xf800);
      else
        tft.setTextColor(0xffff, 0xf800);

      tft.setCursor(27, 168);
      tft.print('^');

      lastSh = shift;

      lastSp = special;
      lastSh = shift;
    }

    for (int y = 0; y < 3; y++)
    {
      int ShiftRight;
      if (special)
      {
        if (shift)
          ShiftRight = 15 * pgm_read_byte(&(Mobile_SymKeys[y][0]));
        else
          ShiftRight = 15 * pgm_read_byte(&(Mobile_NumKeys[y][0]));
      }
      else
        ShiftRight = 15 * pgm_read_byte(&(Mobile_KB[y][0]));

      for (int x = 3; x < 13; x++)
      {
        if (x >=  (special ? (shift ? pgm_read_byte(&(Mobile_SymKeys[y][1])) : pgm_read_byte(&(Mobile_NumKeys[y][1]))) : pgm_read_byte(&(Mobile_KB[y][1])) )) break;

        if (TouchButton(15 + (30 * (x - 3)) + ShiftRight, 100 + (30 * y), 20, 25)) // this will draw the button on the screen by so many pixels
        {
          if (bufIndex < (textLimit - 1))
          {
            delay(200);

            if (special)
            {
              if (shift)
                textBuffer[bufIndex] = pgm_read_byte(&(Mobile_SymKeys[y][x]));
              else
                textBuffer[bufIndex] = pgm_read_byte(&(Mobile_NumKeys[y][x]));
            }
            else
              textBuffer[bufIndex] = (pgm_read_byte(&(Mobile_KB[y][x])) + (shift ? 0 : ('a' - 'A')));

            bufIndex++;
          }
          break;
        }
      }
    }

    //Spacebar
    if (TouchButton(60, 190, 200, 25))
    {
      textBuffer[bufIndex++] = ' ';
      delay(200);
    }

    //BackSpace
    if (TouchButton(270, 160, 35, 25))
    {
      if ((bufIndex) > 0)
        bufIndex--;
      textBuffer[bufIndex] = 0;
      tft.setTextColor(0, ILI9341_BLUE);
      tft.setCursor(15, 80);
      tft.print(F("                          "));
      delay(200);
    }

    //Return
    if (TouchButton(270, 190, 35, 25))
    {
      debugSerial.println(textBuffer);
      // ---------------------------------- send via LORA
      for(int index =0; index< bufIndex; index ++)
      {
        sendingString += textBuffer[index];
        textBuffer[index] = 0;
      }
      canSend = true;
      tft.setTextColor(0, ILI9341_BLUE);
      tft.setCursor(15, 80);
      tft.print(F("                         "));
    }
  }
  tft.setTextColor(0xffff, 0xf800);
  tft.setCursor(15, 80);
  tft.print(textBuffer);
}
void callback(){
   //placeholder callback function
}

void setup(void)
{
  loraSerial.begin(57600);
  debugSerial.begin(9600);
 
  tft.begin();
  if (!ts.begin())
    debugSerial.println(F("Unable to start touchscreen."));
  else
    debugSerial.println(F("Touchscreen started."));

  tft.fillScreen(ILI9341_BLUE);
  // origin = left,top landscape (USB left upper)
  tft.setRotation(1);
  MakeKB_Button(Mobile_KB);

  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T3, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  EEPROM.begin(EEPROM_SIZE);
  int string_length = EEPROM.read(0);
  if (isnan(string_length) && string_length !=0) id_not_set = true;
  else{
     ID = EEPROM.readString(1);
   }


  ttn.personalize(devAddr, nwkSKey, appSKey);
  ttn.onMessage(message);
  xTaskCreatePinnedToCore(
      Task1code, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */
}

void Task1code( void * parameter) {
  for(;;) {
    GetKeyPress(MyBuffer);
    if ((millis() - lastTimeTouched) >= wakeup_time * 1000)
    {
      esp_deep_sleep_start();
    }      
  }
}

void loop()
{
  if(canSend && id_not_set) 
  {
    ID = sendingString;
    EEPROM.write(0, (byte)sendingString.length());
//    EEPROM.writeString(1, sendingString);
    for(int i =0;i < sendingString.length(); i ++)
      EEPROM.write(i + 1, sendingString[i]);
    EEPROM.commit();

  }
  else if(canSend) 
  {
    byte a[sendingString.length()];
    sendingString.getBytes(a, sendingString.length());
    ttn.sendBytes(a, sizeof(sendingString));
  }
}


byte TouchButton(int x, int y, int w, int h)
{
  int X, Y;
  // Retrieve a point
  TS_Point p = ts.getPoint();
  Y = map(p.x, TS_MINY, TS_MAXY, tft.height(), 0);
  X = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());

  return (IsWithin(X, x, x + w) & IsWithin(Y, y, y + h));
}
