// Need to use the board = esp32 / lilygo-t-display-s3 (https://github.com/Xinyuan-LilyGO/T-Display-S3)

#include <TFT_eSPI.h>
#include "font.h"
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "OneButton.h"
#include "driver/gpio.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define back 0x9D91
// #define bigFont DSEG7_Modern_Bold_64
// #define Freqfont DSEG7_Modern_Bold_Italic_45
#define Freqfont DSEG14_Classic_Bold_37 //DSEG14_Classic_Bold_36 //DSEG14_Classic_Regular_35 //DSEG7_Modern_Italic_40 //DSEG7_Modern_Bold_45
#define small DejaVu_Sans_Mono_Bold_12
#define middle Monospaced_bold_18
#define middle1 Monospaced_bold_24 //Monospaced_bold_21
#define middle2 DSEG7_Classic_Bold_30
#define clockFont DSEG7_Modern_Bold_20
#define dc Cousine_Regular_38

#define back 0x9D91
#define color2 0x18E3
#define Black TFT_BLACK
#define Dark_Blue TFT_NAVY
#define LED_Blue TFT_BLUE
#define LED_Red TFT_RED

#define PIN_CLK GPIO_NUM_2              // Used for the Step Rotary Encoder
#define PIN_DT GPIO_NUM_3               // Used for the Step Rotary Encoder

#define Encoder_Switch GPIO_NUM_10        // Used for the Step Rotary Encoder Switch
OneButton Encoder_Switch_button(Encoder_Switch, true);  // Used for the Step Rotary Encoder Switch

#define Memo_button1 GPIO_NUM_11        // Used for the Memo Button1
OneButton button1(Memo_button1, true);  // Used for the Memo Button1
#define Memo_button2 GPIO_NUM_12        // Used for the Memo Button2
OneButton button2(Memo_button2, true);  // Used for the Memo Button2
#define Memo_button3 GPIO_NUM_13        // Used for the Memo Button3
OneButton button3(Memo_button3, true);  // Used for the Memo Button3

int memo1 = back;
int memo2 = back;
int memo3 = back;
int squelch = 0;
int Step = 1;
bool invert = false;
int bright = 6;
int brightnesses[10] = {35, 70, 105, 140, 175, 210, 250};
int pot = 0;        // Needs to be reviewd
int newPos = 0;     // Used by the Step rotary Encoder
static int pos = 1; // Used by the Step rotary Encoder

RotaryEncoder *encoder = nullptr; // A pointer to the dynamic created Step Rotary Encoder instance.
String Frequency = "";
String Squelch_ = " "; //"SQLCH "
String Volume_ = " ";  //"VOLM "

//------------------------- It might be removed ------
ICACHE_RAM_ATTR void ai0(); // Encoder Pin A //interupt rotine
// ICACHE_RAM_ATTR void ai1(); //Encoder Pin B //interupt rotine
ICACHE_RAM_ATTR void Ask_Switch_Check(); // Ask frequence switch interupt rotine

volatile int Analog_Reading, Filtered_Analog_Reading, TempAnalog_Value= 0; // Used for the annalog input Squelch potentiometer
volatile float average_Analog_Reading = 0; // Used for the annalog input Squelch potentiometer
uint64_t counter = 0;
volatile bool LockEncoder = false;
volatile bool Asked = true;

const int P1 = GPIO_NUM_16; //  GPIO16 -> Frequncy Rotary Encoder A
const int P2 = GPIO_NUM_21; //  GPIO21 -> Frequncy Rotary Encoder B
const int BAUDRATE = 57600;
const int SERIAL_TIMEOUT = 2000;

char Received_Freq[16];
char Sent_Freq[16];
char Freq_Hz[15];
// char *Hz = " Hz";

/*Functions*/
void draw();
void initComm();
void initGpio();
void ai0();
char *ultoa(uint64_t val, char *s);
IRAM_ATTR void checkPosition();
void step();
void frequency();
void IRAM_ATTR checkTicks();
void singleClick();
void doubleClick();
void pressStart();
void pressStop();
void IRAM_ATTR checkTicks1();
void singleClick1();
void doubleClick1();
void pressStart1();
void pressStop1();
void IRAM_ATTR checkTicks2();
void singleClick2();
void doubleClick2();
void pressStart2();
void pressStop2();
void IRAM_ATTR checkTicks3();
void singleClick3();
void doubleClick3();
void pressStart3();
void pressStop3();
void askForFrequency();
void Serial_Flush_TX(String command);


void draw()
{
  sprite.setTextColor(Dark_Blue, back);
  sprite.fillSprite(back);
  sprite.setFreeFont(&Freqfont);
  sprite.setTextDatum(2);
  sprite.drawString(Frequency, 316, 85);
  sprite.setFreeFont(&middle2);
  sprite.drawFloat(squelch, 0, 314, 10);
  sprite.drawFloat(Step, 0, 160, 10);
  sprite.setTextDatum(0);
  sprite.setFreeFont(&middle);
  sprite.setTextColor(back, Black);
  sprite.drawString(Squelch_, 170, 6);
  sprite.drawString(Volume_, 170, 26);
  sprite.drawString("STEP ", 35, 6);

  // sprite.drawCircle(13,10,8, Black);  //, Black);
  sprite.fillCircle(68, 61, 8, Black); //, Black);
  // sprite.fillCircle(13,10,5, TFT_RED);  //, Black);
  sprite.fillCircle(68, 61, 5, memo1);  //, Black);
                                        // sprite.drawCircle(13,10,8, Black);  //, Black);
  sprite.fillCircle(160, 61, 8, Black); //, Black);
  // sprite.fillCircle(13,10,5, TFT_RED);  //, Black);
  sprite.fillCircle(160, 61, 5, memo2); //, Black);
  // sprite.drawCircle(13,10,8, Black);  //, Black);
  sprite.fillCircle(251, 61, 8, Black); //, Black);
  // sprite.fillCircle(13,10,5, TFT_RED);  //, Black);
  sprite.fillCircle(251, 61, 5, memo3); //, Black);

  sprite.setTextColor(Black, back);

  sprite.drawString("Mem1", 6, 55);
  sprite.drawString("Mem2", 95, 55);
  sprite.drawString("Mem3", 185, 55);

  sprite.setFreeFont(&middle1);
  sprite.drawString("Hz", 290, 60);
  sprite.setFreeFont(&middle);
  sprite.drawString("KHz", 35, 26);

  sprite.fillRect(30, 46, 134, 4, TFT_MAROON); // Step Box Botton Line
  sprite.fillRect(30, 1, 4, 46, TFT_MAROON);   // Step Box Left Line
  sprite.fillRect(30, 1, 130, 4, TFT_MAROON);  // Step Box Top Line
  sprite.fillRect(160, 1, 4, 46, TFT_MAROON);  // Step Box Right Line

  sprite.fillRect(165, 46, 154, 4, TFT_MAROON); // squelch Box Botton Line
  sprite.fillRect(165, 1, 4, 46, TFT_MAROON);   // squelch Box Left Line
  sprite.fillRect(165, 1, 154, 4, TFT_MAROON);  // squelch Box Top Line
  sprite.fillRect(315, 1, 4, 46, TFT_MAROON);   // squelch Box Right Line
  // for(int i=0;i<0;i++)
  // sprite.fillRect(6+(i*8),26,6,5,color2);
  sprite.setFreeFont(&small);
  sprite.setTextDatum(4);
  int temp = pot;              // Needs to be reviewd
  for (int i = 0; i < 80; i++) // Drawing the Scale
  {
    if ((temp % 10) == 0)
    {
      sprite.drawLine(i * 4, 170, i * 4, 158, Black);
      sprite.drawLine((i * 4) + 1, 170, (i * 4) + 1, 158, Black);
      sprite.drawFloat(temp, 0, i * 4, 148);
    }
    else if ((temp % 5) == 0 && (temp % 10) != 0)
    {
      sprite.drawLine(i * 4, 170, i * 4, 162, Black);
      sprite.drawLine((i * 4) + 1, 170, (i * 4) + 1, 162, Black);
    }
    else
    {
      sprite.drawLine(i * 4, 170, i * 4, 164, Black);
    }
    temp = temp + 1;
  }
  sprite.fillTriangle(160, 138, 156, 130, 164, 130, Black);
  sprite.pushSprite(0, 0);
}

void Squelch()
{
  Analog_Reading = analogRead(1);
  average_Analog_Reading += 0.08 * (Analog_Reading - average_Analog_Reading); // one pole digital filter, about 20Hz cutoff
  Filtered_Analog_Reading = (average_Analog_Reading - 200) / 10;
  if (Filtered_Analog_Reading != TempAnalog_Value)
  {
    Squelch_ = "SQLCH ";
    Volume_ = "";
    squelch = map(Filtered_Analog_Reading, -20, 360, 1, 255);
    char result[3];
    sprintf(result, "%03d", squelch);
    if (Asked == false) {
      Serial_Flush_TX("SQ0" + String(result) + ";;");
    }    
  }
}

void initGpio()
{
  // Used for the Frequency Rotary Encoder
  pinMode(P1, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);
  attachInterrupt(P1, ai0, RISING);
  // Used for the Step Rotary Encoder
  encoder = new RotaryEncoder(PIN_CLK, PIN_DT, RotaryEncoder::LatchMode::TWO03);

  attachInterrupt(digitalPinToInterrupt(PIN_CLK), checkPosition, CHANGE);  // Used for the Step Rotary Encoder Pins CLK
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkPosition, CHANGE);   // Used for the Step Rotary Encoder Pins DT
  attachInterrupt(digitalPinToInterrupt(Encoder_Switch), checkTicks1, CHANGE); // Used for the Step Rotary Encoder Switch
  attachInterrupt(digitalPinToInterrupt(Memo_button1), checkTicks1, CHANGE); // Used for the Step Rotary Encoder Switch
  attachInterrupt(digitalPinToInterrupt(Memo_button2), checkTicks2, CHANGE); // Used detect the Memo2 button
  attachInterrupt(digitalPinToInterrupt(Memo_button3), checkTicks3, CHANGE); // Used detect the Memo2 button
  
  // Used detect the Step Rotary Switch clicks
  Encoder_Switch_button.attachClick(singleClick);
  Encoder_Switch_button.attachDoubleClick(doubleClick);
  Encoder_Switch_button.setPressMs(1000); // that is the time when LongPressStart is called
  Encoder_Switch_button.attachLongPressStart(pressStart);
  Encoder_Switch_button.attachLongPressStop(pressStop);

  // Used detect the Memo1 button clicks
  button1.attachClick(singleClick1);
  button1.attachDoubleClick(doubleClick1);
  button1.setPressMs(1000); // that is the time when LongPressStart is called
  button1.attachLongPressStart(pressStart1);
  button1.attachLongPressStop(pressStop1);

  // Used detect the Memo2 button clicks
  button2.attachClick(singleClick2);
  button2.attachDoubleClick(doubleClick2);
  button2.setPressMs(1000); // that is the time when LongPressStart is called
  button2.attachLongPressStart(pressStart2);
  button2.attachLongPressStop(pressStop2);

  // Used detect the Memo3 button clicks
  button3.attachClick(singleClick3);
  button3.attachDoubleClick(doubleClick3);
  button3.setPressMs(1000); // that is the time when LongPressStart is called
  button3.attachLongPressStart(pressStart3);
  button3.attachLongPressStop(pressStop3);
}

void initComm()
{
  Serial.setDebugOutput(false);
  Serial.setTimeout(SERIAL_TIMEOUT);
  Serial.clearWriteError();
  Serial.begin(BAUDRATE);
  /* while (!Serial)
  {
    yield();
    delay(10); // wait for serial port to connect. Needed for native USB port only
  } */
}

void ai0()
{ // Interrupt used for the Frequency Rotary Encoder
  if (!LockEncoder)
  {
    if (digitalRead(P2) == LOW)
    {
      if (counter < 1000)
      {
        counter++;
      }
      else
      {
        counter = counter + (pos);
      }
    }
    else
    {
      if (counter != 0)
      {
        if (counter < 1000)
        {
          counter--;
        }
        else
        {
          counter = counter - (pos);
        }
      }
    }
  }
}

IRAM_ATTR void checkPosition() // Interrupt used for the Step Rotary Encoder
{
  encoder->tick(); // just call tick() to check the state.
}

char *ultoa(uint64_t val, char *s)
{
  char *p = s + 13;
  *p = '\0';
  do
  {
    if ((p - s) % 4 == 2)
      *--p = '.';
    *--p = '0' + val % 10;
    val /= 10;
  } while (val);
  return p;
}

void step()
{
  newPos = encoder->getPosition();
  if (pos != newPos)
  {
    Squelch_ = "";
    Volume_ = " VOL  ";
    if (newPos <= 0)
    {
      newPos = 1;
      encoder->setPosition(1);
    }
    if (newPos >= 100)
    {
      newPos = 100;
      encoder->setPosition(100);
    }
    pos = newPos;
    Step = pos;
  }
}

void frequency()
{
  strcpy(Freq_Hz, ultoa(counter, Received_Freq));
  Frequency = Freq_Hz;
}

void IRAM_ATTR checkTicks()
{
  button1.tick(); // just call tick() to check the state.
}

void singleClick()
{
  
}

void doubleClick()
{
  askForFrequency();
}

void pressStart()
{ 
  
}

void pressStop()
{

}

void IRAM_ATTR checkTicks1()
{
  button1.tick(); // just call tick() to check the state.
}

void singleClick1()
{
  memo1 = LED_Red;
}

void doubleClick1()
{
  // Serial.println("doubleClick1() detected.");
}

void pressStart1()
{ // this function will be called when the button1 was held down for 1 second or more.
  memo1 = back;
}

void pressStop1()
{ // this function will be called when the button1 was released after a long hold.
  memo1 = LED_Blue;
}

void IRAM_ATTR checkTicks2()
{
  button2.tick(); // just call tick() to check the state.
}

void singleClick2()
{
  memo2 = LED_Red;
  // Serial.println("Singlelick1() detected.");
}

void doubleClick2()
{
  // Serial.println("doubleClick1() detected.");
}

void pressStart2()
{ // this function will be called when the button1 was held down for 1 second or more.
  memo2 = back;
}

void pressStop2()
{ // this function will be called when the button1 was released after a long hold.
  memo2 = LED_Blue;
}

void IRAM_ATTR checkTicks3()
{
  button3.tick(); // just call tick() to check the state.
}

void singleClick3()
{
  memo3 = LED_Red;
  // Serial.println("Singlelick1() detected.");
}

void doubleClick3()
{
  // Serial.println("doubleClick1() detected.");
}

void pressStart3()
{ // this function will be called when the button1 was held down for 1 second or more.
  memo3 = back;
}

void pressStop3()
{ // this function will be called when the button1 was released after a long hold.
  memo3 = LED_Blue;
}

void askForFrequency(){
  counter = 0;

  Serial_Flush_TX("FA;");
  LockEncoder = true;
  Asked = true;
  if(Serial.available()){
    String rxresponse = Serial.readStringUntil(';');
    if (rxresponse.startsWith("FA")) {
      counter = rxresponse.substring(2, 13).toInt();
      LockEncoder = false;
      Asked = false;
    }      
  }
}

void Serial_Flush_TX(String command)
{
  Serial.flush(); // wait until TX buffer is empty
  delay(20);
  Serial.println(command);
  delay(20);
}

void setup()
{
  analogReadResolution(12);
  tft.init();
  tft.setRotation(1);
  sprite.createSprite(320, 170);
  sprite.setTextColor(Black, back);
  sprite.setFreeFont(&Freqfont);
  ledcSetup(0, 10000, 2);
  ledcAttachPin(38, 0);
  ledcWrite(0, brightnesses[bright]); // brightnes of screen
  initGpio();
  initComm();
}

void loop()
{
  Squelch();
  step();
  frequency();
  draw();
  Encoder_Switch_button.tick();
  button1.tick();
  button2.tick();
  button3.tick();
  //askForFrequency();
  // encoder->tick(); // Check the Step Encoder.
}