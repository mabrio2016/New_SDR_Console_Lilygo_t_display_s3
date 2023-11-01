#include <TFT_eSPI.h>
#include "font.h"
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "OneButton.h"
#include "driver/gpio.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define back 0x9D91
//#define bigFont DSEG7_Modern_Bold_64
//#define Freqfont DSEG7_Modern_Bold_Italic_45
#define Freqfont DSEG7_Modern_Bold_45
#define small DejaVu_Sans_Mono_Bold_12
#define middle Monospaced_bold_18
#define middle2 DSEG7_Classic_Bold_30
#define clockFont DSEG7_Modern_Bold_20
#define dc Cousine_Regular_38

#define back 0x9D91
#define color2 0x18E3
#define Black TFT_BLACK
#define LED_Blue TFT_BLUE
#define LED_Red TFT_RED

#define PIN_CLK GPIO_NUM_2 // Used for the Step Rotary Encoder
#define PIN_DT GPIO_NUM_3  // Used for the Step Rotary Encoder
#define PIN_INPUT GPIO_NUM_10 // Used for the Step Rotary Encoder Switch
OneButton button(PIN_INPUT, true); // Used for the Step Rotary Encoder Switch
unsigned long pressStartTime; // Used for the Step Rotary Encoder Switch - Save the millis when a press has started.

int memo1 = back;
int memo2 = LED_Blue;
int memo3 = LED_Red;
int squelch = 0;
int Step = 0;
bool invert=false;
int bright=6;
int brightnesses[10]={35,70,105,140,175,210,250};
int pot=0;  // Needs to be reviewd
int newPos = 0;     // Used by the Step rotary Encoder
static int pos = 0; // Used by the Step rotary Encoder

RotaryEncoder *encoder = nullptr; // A pointer to the dynamic created Step Rotary Encoder instance.
String Frequency= "";
String Squelch_ = " "; //"SQLCH "
String Volume_ = " ";  //"VOLM "

//------------------------- It might be removed ------
ICACHE_RAM_ATTR void ai0(); //Encoder Pin A //interupt rotine
//ICACHE_RAM_ATTR void ai1(); //Encoder Pin B //interupt rotine
ICACHE_RAM_ATTR void Ask_Switch_Check(); // Ask frequence switch interupt rotine

volatile int TempAnalog_Value, Analog_Value = 0;  //These variable will hold the Analog Squelch values;
volatile int Analog_Reading, Filtered_Analog_Reading = 0; volatile float average_Analog_Reading = 0;  // Used for the annalog input Squelch potentiometer
volatile long TempCounter, readFrequency, toSendFrequency, memoFrequncy = 0;
uint64_t counter = 0;
volatile bool LockEncoder = false;

const int P1 = GPIO_NUM_16; //  GPIO16 -> Frequncy Rotary Encoder A
const int P2 = GPIO_NUM_21; //  GPIO21 -> Frequncy Rotary Encoder B
const int BAUDRATE = 57600;
const int SERIAL_TIMEOUT = 2000;

char Received_Freq[16];
char Sent_Freq[16];
char Freq_Hz[15];
//char *Hz = " Hz";

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

void setup() 
{  
  analogReadResolution(12);
  tft.init();
  tft.setRotation(1);
  sprite.createSprite(320,170);
  sprite.setTextColor(Black,back);
  sprite.setFreeFont(&Freqfont);
  ledcSetup(0, 10000, 2);
  ledcAttachPin(38, 0);
  ledcWrite(0, brightnesses[bright]);  //brightnes of screen
  initGpio();
  initComm();
}

void draw()
{
  sprite.setTextColor(Black,back);
  sprite.fillSprite(back);
  sprite.setFreeFont(&Freqfont);
  sprite.setTextDatum(2);
  sprite.drawString(Frequency,316,75);  
  sprite.setFreeFont(&middle2);
  sprite.drawFloat(squelch,0,314,10);
  sprite.drawFloat(Step,0,160,10);
  sprite.setTextDatum(0);
  sprite.setFreeFont(&middle);
  sprite.setTextColor(back,Black);
  sprite.drawString(Squelch_,170,6);
  sprite.drawString(Volume_,170,26);
  sprite.drawString("STEP ",35,6);
    //sprite.drawCircle(13,10,8, Black);  //, Black);
    sprite.fillCircle(13,10,8, Black);  //, Black);
    //sprite.fillCircle(13,10,5, TFT_RED);  //, Black);
    sprite.fillCircle(13,10,5, memo1);  //, Black);
  sprite.setTextColor(Black,back);
  sprite.drawString("KHz",35,26);
  sprite.drawString("KHz",280,53);
  sprite.fillRect(30,46,134,4,TFT_MAROON); //Step Box Botton Line
  sprite.fillRect(30,1,4,46,TFT_MAROON);   //Step Box Left Line
  sprite.fillRect(30,1,130,4,TFT_MAROON);  //Step Box Top Line
  sprite.fillRect(160,1,4,46,TFT_MAROON);  //Step Box Right Line

  sprite.fillRect(165,46,154,4,TFT_MAROON); //squelch Box Botton Line
  sprite.fillRect(165,1,4,46,TFT_MAROON);   //squelch Box Left Line
  sprite.fillRect(165,1,154,4,TFT_MAROON);  //squelch Box Top Line
  sprite.fillRect(315,1,4,46,TFT_MAROON);   //squelch Box Right Line
  //for(int i=0;i<0;i++)
  //sprite.fillRect(6+(i*8),26,6,5,color2);
  sprite.setFreeFont(&small);
  sprite.setTextDatum(4);
  int temp=pot; // Needs to be reviewd
  for(int i=0;i<80;i++) // Drawing the Scale
  {
    if((temp%10)==0)
    {
      sprite.drawLine(i*4,170,i*4,158,Black);
      sprite.drawLine((i*4)+1,170,(i*4)+1,158,Black);
      sprite.drawFloat(temp,0,i*4,148);
    }
    else if((temp%5)==0 && (temp%10)!=0)
    {
      sprite.drawLine(i*4,170,i*4,162,Black);
      sprite.drawLine((i*4)+1,170,(i*4)+1,162,Black);
    }
    else
    {
      sprite.drawLine(i*4,170,i*4,164,Black);
    }
    temp=temp+1;
  }
  sprite.fillTriangle(160,138,156,130,164,130,Black);
  sprite.pushSprite(0,0);
}

void Squelch() {
  Analog_Reading = analogRead(1);
  average_Analog_Reading += 0.08 * (Analog_Reading - average_Analog_Reading); // one pole digital filter, about 20Hz cutoff
  Filtered_Analog_Reading = (average_Analog_Reading - 200) / 10; //* 1.2; 
  if (Filtered_Analog_Reading != TempAnalog_Value) {
      Squelch_ = "SQLCH ";
      Volume_ = "";
      Analog_Value = map(Filtered_Analog_Reading, -20, 360, 1, 255); 
      //Serial.printf("Filtered_Analog_Reading = %d\n",Analog_Reading);
      TempAnalog_Value = Filtered_Analog_Reading;
      squelch = Analog_Value;
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
  // Used for the Step Rotary Encoder Pins CLK, DT and Switch
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT), checkTicks, CHANGE);
  // Used detect the Step Rotary Switch clicks
  button.attachClick(singleClick);
  button.attachDoubleClick(doubleClick);
  button.setPressMs(1000); // that is the time when LongPressStart is called
  button.attachLongPressStart(pressStart);
  button.attachLongPressStop(pressStop);
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

void ai0() {  // Interrupt used for the Frequency Rotary Encoder
  if (!LockEncoder){
      if(digitalRead(P2)==LOW){
        if (counter < 1000) {
          counter++;
        }
        else {
          counter = counter + (pos);
        }
      }
      else{
        if (counter != 0) {
            if (counter < 1000) {
              counter--;
            }
            else {
                counter = counter - (pos);
            }          
        }
      }
  }
}

IRAM_ATTR void checkPosition()  // Interrupt used for the Step Rotary Encoder
{
  encoder->tick(); // just call tick() to check the state.
}

char *ultoa(uint64_t val, char *s)
{
    char *p = s + 13;
    *p = '\0';
    do {
        if ((p - s) % 4 == 2)
            *--p = '.';
        *--p = '0' + val % 10;
        val /= 10;
    } while (val);
    return p;
}

void step(){
  //pos = 0;
  newPos = encoder->getPosition();
  if (pos != newPos) {
    Squelch_ = "";
    Volume_ = " VOL  ";
    if (newPos < 0 ) {
      newPos = 0;
      encoder->setPosition(0);
    }
    if (newPos >= 100 ) {
      newPos = 100;
      encoder->setPosition(100); 
    }
    pos = newPos;
    Step = pos;
  }
}

void frequency(){
  strcpy(Freq_Hz,ultoa(counter, Received_Freq)); 
  Frequency = Freq_Hz;  
}

void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  button.tick(); // just call tick() to check the state.
}

void singleClick() {
  //Serial.println("singleClick() detected.");
  memo1 = LED_Red;
} // singleClick

void doubleClick() {
  Serial.println("doubleClick() detected.");
} // doubleClick

void pressStart() { // this function will be called when the button was held down for 1 second or more.
  //Serial.println("pressStart()");
  pressStartTime = millis() - 1000; // as set in setPressMs()
  memo1 = back;
} // pressStart()

void pressStop() {  // this function will be called when the button was released after a long hold.
  //Serial.print("pressStop(");
  //Serial.print(millis() - pressStartTime);
  //Serial.println(") detected.");
  memo1 = LED_Blue;
} // pressStop()

void loop() 
{
  Squelch();
  step();
  frequency();
  draw();
  button.tick();
  //encoder->tick(); // Check the Step Encoder.
}