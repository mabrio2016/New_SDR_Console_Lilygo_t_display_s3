// Need to use the board = esp32 / lilygo-t-display-s3 (https://github.com/Xinyuan-LilyGO/T-Display-S3)

#include <TFT_eSPI.h>
#include "font.h"
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "OneButton.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "WiFi.h"

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
#define Gray 0x18E3
#define Black TFT_BLACK
#define Dark_Blue TFT_NAVY
#define Dark_Green TFT_DARKGREEN
#define LED_Blue TFT_BLUE
#define LED_Red TFT_RED
#define LED_Yellow TFT_YELLOW

#define Squelch_ADC_Pin GPIO_NUM_1              // Used for the squelch ADC poteniometer

#define PIN_CLK GPIO_NUM_2              // Used for the Step Rotary Encoder
#define PIN_DT GPIO_NUM_3               // Used for the Step Rotary Encoder

#define Encoder_Switch GPIO_NUM_10                      // Used for the Step Rotary Encoder Switch
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
int connected = back;
int Step = 1;
//int ValuePos = 0;
bool invert = false;
int bright = 6;
int brightnesses[10] = {35, 70, 105, 140, 175, 210, 250};
int pot = 0;        // ???? Needs to be reviewd
int newPos = 1;     // Used by the Step rotary Encoder
static int pos = 1; // Used by the Step rotary Encoder
int modulationNumber = 0;   // Holds the Modulation number code
String Modulation = ""; //Holds the 3 leters modulation mode

RotaryEncoder *encoder = nullptr; // A pointer to the dynamic created Step Rotary Encoder instance.
String Frequency = "";
String Squelch_ = " "; // To disply "SQLCH " 
int squelch = 0;
String Volume_ = " ";  //To display "VOLM "
String Step_ = "Hz";  // Hz or KHz
int dispStep_ = 1;
bool ToSentFrequncyFlag = true;
//------------------------- It might be removed ------
ICACHE_RAM_ATTR void ai0(); // Encoder Pin A //interupt rotine
// ICACHE_RAM_ATTR void ai1(); //Encoder Pin B //interupt rotine
ICACHE_RAM_ATTR void Ask_Switch_Check(); // Ask frequence switch interupt rotine

volatile int Analog_Reading, Filtered_Analog_Reading, Temp_squelch= 0; // Used for the annalog input Squelch potentiometer
volatile float average_Analog_Reading = 0; // Used for the annalog input Squelch potentiometer
volatile long counter = 0;
volatile long Memo1_counter = 0;
volatile long Memo2_counter = 0;
volatile long Memo3_counter = 0;
String Memo1_SQLCH = "100";
String Memo2_SQLCH = "100";
String Memo3_SQLCH = "100";
String Memo1_Modulation = "";
String Memo2_Modulation = "";
String Memo3_Modulation = "";
String Memo1_modulationNumber = "";
String Memo2_modulationNumber = "";
String Memo3_modulationNumber = "";

//uint64_t last_counter = 0;
bool LockEncoder = false;
bool Asked = false;

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
void Show_frequency();
void IRAM_ATTR checkTicks_Encoder();
void singleClick_Encoder();
void doubleClick_Encoder();
void pressStart_Encoder();
void pressStop_Encoder();
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
void Send_Frequency();
String askSquelch();
void Read_Squelch();
void Send_Squelch(String squelch_Value);
String askForModulation();
void Serial_Flush_TX(String command);

void draw()
{
  sprite.setTextColor(Dark_Blue, back);
  sprite.fillSprite(back);
  sprite.setFreeFont(&Freqfont);
  sprite.setTextDatum(0);
  sprite.fillRoundRect(5, 89, 311, 50, 6, TFT_MAROON); // Frequency window.
  sprite.drawRoundRect(5, 89, 310, 51, 6, Black);
  sprite.drawRoundRect(4, 90, 312, 51, 6, Black);
  sprite.setTextDatum(2);
  sprite.setTextColor(back, TFT_MAROON);
  sprite.drawString(Frequency, 309, 95);
  sprite.setFreeFont(&middle1);
  sprite.drawString("Hz ", 316, 57);
  sprite.drawRect(285, 56, 31, 25, Black);
  sprite.drawRect(284, 57, 33, 25, Black);
  sprite.setTextColor(Dark_Blue, back);
  sprite.setFreeFont(&middle2);
  sprite.drawNumber(squelch, 314, 10);
  sprite.drawNumber(dispStep_, 160, 10);
  sprite.setTextDatum(0);
  sprite.setFreeFont(&middle);
  sprite.setTextColor(back, Black);
  sprite.drawString(Squelch_, 170, 6);
  sprite.drawString(" VOL  ", 170, 26);  // " VOL  "   Volume_
  sprite.drawString("STEP ", 35, 6);
  sprite.setFreeFont(&middle1);
  sprite.setTextColor(Black, back);
  sprite.drawString(Modulation, 6, 62);

  // Memo LEDs
  sprite.setTextColor(back, Dark_Green);
  sprite.fillRoundRect(5, 54, 156, 30, 6, Dark_Green);
  sprite.drawRoundRect(5, 54, 155, 31, 6, Black);
  sprite.drawRoundRect(4, 55, 157, 31, 6, Black);
  sprite.drawString("Memories", 25, 58);
  sprite.fillCircle(184, 68, 8, Black);
  sprite.fillCircle(184, 68, 5, memo1);
  sprite.fillCircle(220, 68, 8, Black);
  sprite.fillCircle(220, 68, 5, memo2);
  sprite.fillCircle(256, 68, 8, Black);
  sprite.fillCircle(256, 68, 5, memo3);

  //Connected LED
  sprite.fillCircle(15, 13, 8, Black);
  sprite.fillCircle(15, 13, 5, connected);

  sprite.setTextColor(Black, back);  
  sprite.setFreeFont(&middle);
  sprite.drawString(Step_, 35, 26); // Step

  sprite.fillRect(30, 46, 134, 4, TFT_MAROON); // Step Box Botton Line
  sprite.fillRect(30, 1, 4, 46, TFT_MAROON);   // Step Box Left Line
  sprite.fillRect(30, 1, 130, 4, TFT_MAROON);  // Step Box Top Line
  sprite.fillRect(160, 1, 4, 46, TFT_MAROON);  // Step Box Right Line

  sprite.fillRect(165, 46, 154, 4, TFT_MAROON); // squelch Box Botton Line
  sprite.fillRect(165, 1, 4, 46, TFT_MAROON);   // squelch Box Left Line
  sprite.fillRect(165, 1, 154, 4, TFT_MAROON);  // squelch Box Top Line
  sprite.fillRect(315, 1, 4, 46, TFT_MAROON);   // squelch Box Right Line

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
  //sprite.fillTriangle(160, 138, 156, 130, 164, 130, Black);
  sprite.pushSprite(0, 0);
}

void initGpio()
{
  WiFi.mode(WIFI_OFF);  // Disable the WiFi
  btStop();             // Disable the Bluetooth
  
  //Used for the Squelch potenciometer.
  analogSetPinAttenuation(Squelch_ADC_Pin, ADC_11db);

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
  Encoder_Switch_button.attachClick(singleClick_Encoder);
  Encoder_Switch_button.attachDoubleClick(doubleClick_Encoder);
  Encoder_Switch_button.setPressMs(1000); // that is the time when LongPressStart is called
  Encoder_Switch_button.attachLongPressStart(pressStart_Encoder);
  Encoder_Switch_button.attachLongPressStop(pressStop_Encoder);

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
        counter = counter + (Step);
      }
    }
    else
    {
      if (counter > 0)
      {
        if (counter < 1000)
        {
          counter--;
        }
        else
        {
          counter = counter - (Step);
          if (counter < 0 ){
            //Serial.println(counter);
            counter = 0;
          }          
        }
      }
    }
    ToSentFrequncyFlag = true;
  }
}

IRAM_ATTR void checkPosition() // Interrupt used for the Step Rotary Encoder
{
  encoder->tick(); // just call tick() to check the state.
  step();
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
    pos = newPos;
    Volume_ = " VOL  ";
    Squelch_ = "";
    if (newPos <= 0)
    {
      newPos = 1;
      encoder->setPosition(1);
    }
    if (newPos >= 1001)
    {
      newPos = 1001;
      encoder->setPosition(1001);
    }
    if (newPos > 1 && newPos < 1000){  //if Step = 1,  do not multiply the step value
      int PosX10 = newPos;
      PosX10 = (PosX10 - 1) * 10;
      Step = PosX10;
      dispStep_ = Step;
    }
    else 
    {
      Step = newPos;
      dispStep_ = Step;  
    }
    if (pos > 100){
      Step = (pos - 100) * 1000;
      dispStep_ = Step / 1000;
      Step_ = "KHz";
    }
    else{
      Step_ = "Hz";
    } 
  }
  //dispStep_ = Step; 
}

void Show_frequency() // Might be able to combine this with the askForFrequency()
{
  strcpy(Freq_Hz, ultoa(counter, Received_Freq));
  Frequency = Freq_Hz;
}

void IRAM_ATTR checkTicks_Encoder()
{
  Encoder_Switch_button.tick(); // just call tick() to check the state.
}

void singleClick_Encoder()
{
  
}

void doubleClick_Encoder()
{
  askForFrequency();
  Modulation = askForModulation();
  if (memo1 == LED_Red){
    memo1 = LED_Blue;
  }
  if (memo2 == LED_Red){
    memo2 = LED_Blue;
  }
  if (memo3 ==LED_Red){
    memo3 = LED_Blue;
  }
}

void pressStart_Encoder()
{ 
  
}

void pressStop_Encoder()
{

}

void IRAM_ATTR checkTicks1()
{
  button1.tick(); // just call tick() to check the state.
}

void singleClick1()
{
  if (Memo1_counter != 0){
    counter = Memo1_counter;
    Modulation = Memo1_Modulation;
    memo1 = LED_Red;
    ToSentFrequncyFlag = true;
    Serial_Flush_TX("SQ0" + Memo1_SQLCH + ";");
    Serial_Flush_TX("MD" + Memo1_modulationNumber + ";");
  }  
  if (memo2 == LED_Red){
    memo2 = LED_Blue;  
  }
  if (memo3 == LED_Red){
    memo3 = LED_Blue;  
  }
}

void doubleClick1()
{
  memo1 = back;
  Memo1_counter = 0;
}

void pressStart1()
{ 
  Memo1_counter = counter;
  memo1 = LED_Blue;
  Memo1_SQLCH = askSquelch();
  Memo1_Modulation = Modulation;
  Memo1_modulationNumber = String(modulationNumber);
}

void pressStop1()
{

}

void IRAM_ATTR checkTicks2()
{
  button2.tick(); // just call tick() to check the state.
}

void singleClick2()
{
  if (Memo2_counter != 0){
    counter = Memo2_counter;
    Modulation = Memo2_Modulation;
    memo2 = LED_Red;
    ToSentFrequncyFlag = true;
    Serial_Flush_TX("SQ0" + Memo2_SQLCH + ";");
    Serial_Flush_TX("MD" + Memo2_modulationNumber + ";");
  }
  if (memo1 == LED_Red){
    memo1 = LED_Blue;  
  }
  if (memo3 == LED_Red){
    memo3 = LED_Blue;  
  }
} 

void doubleClick2()
{
  memo2 = back;
  Memo2_counter = 0;
}

void pressStart2()
{ // this function will be called when the button1 was held down for 1 second or more.
  Memo2_counter = counter;
  memo2 = LED_Blue;
  Memo2_SQLCH = askSquelch();
  Memo2_Modulation = Modulation;
  Memo2_modulationNumber = String(modulationNumber);
}

void pressStop2()
{ // this function will be called when the button1 was released after a long hold.

}

void IRAM_ATTR checkTicks3()
{
  button3.tick(); // just call tick() to check the state.
}

void singleClick3()
{
  if (Memo3_counter != 0){
    counter = Memo3_counter;
    Modulation = Memo3_Modulation;
    memo3 = LED_Red;
    ToSentFrequncyFlag = true;
    Serial_Flush_TX("SQ0" + Memo3_SQLCH + ";");
    Serial_Flush_TX("MD" + Memo3_modulationNumber + ";");
  }
  if (memo1 == LED_Red){
    memo1 = LED_Blue;  
  }
  if (memo2 == LED_Red){
    memo2 = LED_Blue;  
  }
}  // Serial.println("Singlelick1() detected.")

void doubleClick3()
{
  memo3 = back;
  Memo3_counter = 0;
}

void pressStart3()
{ // this function will be called when the button1 was held down for 1 second or more.
  Memo3_counter = counter;
  memo3 = LED_Blue;
  Memo3_SQLCH = askSquelch();
  Memo3_Modulation = Modulation;
  Memo3_modulationNumber = String(modulationNumber);
}

void pressStop3()
{ // this function will be called when the button1 was released after a long hold.
}

void askForFrequency(){
  //connected = LED_Yellow;
  LockEncoder = true;
  Asked = true;
  counter = 0; //To disply 0 if no responce
  //connected = back;
  //Serial_Flush_TX("FA;");
  Serial.flush(); // wait until TX buffer is empty.  I will hold rthe execution if COM port is not connected
  delay(20);
  Serial.println("FA;");
  delay(20);
  if(Serial.available()){
    delay(20);
    String rxresponse = Serial.readStringUntil(';');
    if (rxresponse.startsWith("FA")) {
      counter = rxresponse.substring(2, 13).toInt();
      LockEncoder = false;
      Asked = false;
      Show_frequency();
    }      
  }
  else{
    //connected = back;
  }
}

void Send_Frequency(){
  char result[11]; 
  sprintf(result, "%011d", counter);
  Serial_Flush_TX("FA" + String(result) + ";;");
  delay(20);
  //if(Serial.available()){
    //String rxresponse = Serial.readStringUntil(';');
    //connected = LED_Yellow;
  //}
  //else {
    //connected = back;  
  //}
}

String askSquelch() {
  String SQLCH = "100";
  LockEncoder = true;
  Asked = true;
  //squelch = 0;
  //Serial_Flush_TX("SQ0;"); 
  Serial.flush(); // wait until TX buffer is empty.  It will hold rthe execution if COM port is not connected
  Serial.println("SQ0;");
  delay(20);
  if(Serial.available()){
    String rxresponse = Serial.readStringUntil(';');
    if (rxresponse.startsWith("SQ0")) {
      squelch = rxresponse.substring(3,6).toInt();
      SQLCH = rxresponse.substring(3,6);  // Used to store the memory 
      //connected = LED_Yellow;
      LockEncoder = false;
      Asked = false;
    }      
  }
  else{
    connected = back;
  }
  return SQLCH;
}

void Read_Squelch()
{
  if (Asked == false);{
    int average = 0;
    for (int i=0; i < 20; i++) {
      average = average + analogRead(Squelch_ADC_Pin);
    }
    average = average/10;    
    Analog_Reading = (Analog_Reading * (12-1) + average) / 12; // average_Analog_Reading += 0.1 * (Analog_Reading - average_Analog_Reading); // one pole digital filter, about 20Hz cutoff
    Filtered_Analog_Reading = (Analog_Reading - 250) / 10;   // Used to adjust the minimum value = 1
    squelch = map(Filtered_Analog_Reading, -25, 800, 1, 255);
    if (squelch != Temp_squelch)
    {
      Volume_ = "";
      Squelch_ = "SQLCH ";
      Send_Squelch(String(squelch));
      Temp_squelch = squelch;
    }
  }
}

void Send_Squelch(String squelch_Value){
  int IntValue = squelch_Value.toInt();
  // the if conditions are needed to add leading zeros to the Squelch value (3 digits)
  if (IntValue >= 10 && IntValue < 100) {
    squelch_Value = "0" + squelch_Value;
  }
  if (IntValue < 10) {
    squelch_Value = "00" + squelch_Value;
  }
  Serial_Flush_TX("SQ0" + squelch_Value + ";");
}

String askForModulation(){
  LockEncoder = true;
  Asked = true;
  Serial.flush(); // wait until TX buffer is empty.  It will hold rthe execution if COM port is not connected
  delay(20);
  Serial.println("MD;");
  delay(20);
  String Modulation_ = "";
  if(Serial.available()){
    delay(20);
    String rxresponse = Serial.readStringUntil(';');
    if (rxresponse.startsWith("MD"))
    {
      modulationNumber = rxresponse.substring(2, 3).toInt();
      switch (modulationNumber)
      {
      case 0:
      Modulation_ = "DSB";
        break;
      case 1:
      Modulation_ = "LSB";
        break;
      case 2:
      Modulation_ = "USB";
        break;
      case 3:
      Modulation_ = "CW";
        break;
      case 4:
        Modulation_ = "FM";
        break;
      case 5:
        Modulation_ = "AM";
        break;
      case 6:
        Modulation_ = "FSK";
        break;
      case 7:
        Modulation_ = "CWR"; //(CW Reverse)
        break;
      case 8:
        Modulation_ = "DRM";
        break;
      case 9:
        Modulation_ = "FSR"; //(FSK Reverse);
        break;      
      default:
        Modulation_ = "AM";
        break;
      }
      Asked = false;
      LockEncoder = false;
    }
  }
  return Modulation_;
}

void Serial_Flush_TX(String command)
{
  //Serial.flush(); // wait until TX buffer is empty // I believe it is not needed
  delay(20);
  Serial.println(command);
  delay(20);
}

void setup()
{
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
  Read_Squelch();  // Need to be in the loop, because no interrupt is configured fo the ADC reading,
  draw();
  Encoder_Switch_button.tick();
  button1.tick();
  button2.tick();
  button3.tick();

  if (Asked == false) {
    if (ToSentFrequncyFlag == true){
      Send_Frequency();
      Show_frequency();
      ToSentFrequncyFlag = false;
    }
  }
  if (LockEncoder == false) {
    connected = LED_Yellow;
  }
  if (LockEncoder == true) {
    connected = back;
  }
  
  //askForFrequency();
  // encoder->tick(); // Check the Step Encoder.
}