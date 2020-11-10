/*


   --------------------------- pin layout used for rfid reader-------------------------------------------
   ------------------------------------
               MFRC522      Arduino    
               Reader/PCD   Uno/101    
   Signal      Pin          Pin        
   ------------------------------------
   RST/Reset   RST          9          
   SPI SS      SDA(SS)      10         
   SPI MOSI    MOSI         11 / ICSP-4
   SPI MISO    MISO         12 / ICSP-1
   SPI SCK     SCK          13 / ICSP-3


  ---------------------------pin layout display module-------------------------------------------

  MODULE.....UNO/NANO
  VCC........+5V.....
  GND........GND.....
  DIN........11......
  CS (LOAD)..2.......
  CLK........13......
*/


/* Include the HCMAX7219, SPI library and rfid library */
#include <HCMAX7219.h>
#include <SPI.h>
#include <MFRC522.h>
/* Set the LOAD (CS) digital pin number*/
#define LOAD 2

/* Create an instance of the library */
HCMAX7219 HCMAX7219(LOAD);

/*set the ss en reset pin number of the RFID Reader*/
#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN); // Create instance of the class
MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information

int countDown = 9; // intial countdown in seconds 
long interval = 10000;//10 secondes window for the owner at the startup of the program where he can scan any rfid tag to activate the reset method
long intervalCountDown = 1000;//1 second interval for intial countdown. 
unsigned long previousMillis = 0;//store the previous measured current time.
unsigned long previousMillisCountDown = 0; // store previous measured current time for the countdown. 
bool reset = true; //keeps track if reset method is actived 
bool countDownDone = false;
int chipCounter = 0; //counter for how many rfid chips have been scanned for the reset method
const int buttonPin = 3; // set pushbutton pin
int LED_1 = 7; // Set green led pin
int LED_2 = 4; // Set red led pin
int Speaker = 8; //set buzzer pin

int scanCount = 0; // store the amount of times a chip is scanned by the RFID scanner
int buttonPushCounter = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
String finalCode = "01234"; //final bomb code
String code; // store the current scanned code
String prevCode; //store prev state of code;
String currSequel; //stores the the sequel number of 
char currCode[8];//stores the current code that has been scanned by the players.
char prevCurrCode[8];//stores the prevCurrCode. Is used to see if user has scanned a new tag



void setup() {
  Serial.begin(9600); //Init Serial
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  pinMode(LED_2, OUTPUT);//set LED_2 pin to ouput(we do this so that this pin has a 5v output instead of 3.3v)
  pinMode(LED_1, OUTPUT);//set LED_1 pint to output
  pinMode(Speaker, OUTPUT);//set Speaker pin to output
  pinMode(buttonPin, INPUT);//set buttonPin to input
  
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library, also set to default key from factory.
  }

}


int block = 6; //this is the block number we will write into and then read. Do not write into 'sector trailer' block, since this can make the block unusable.

byte blockcontent[16];//an array with 16 bytes to be written into one of the 64 card blocks is defined
//byte blockcontent[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//all zeros. This can be used to delete a block.
byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.


void loop() {
  
  buttonState = digitalRead(buttonPin);//store the state of the button.
  unsigned long currentMillis = millis();//store the current time since startup.

  /*if intial countdown is not done keep counting down
    else update display to current scanned code if a new tag is scanned*/
  if (currentMillis < interval &&  currentMillis - previousMillisCountDown >= intervalCountDown) {
    HCMAX7219.Clear();//clear the display
    //Write the countdown number to the output buffer 
    HCMAX7219.print7Seg(countDown, 8);
    //Send the output buffer to the display
    HCMAX7219.Refresh();
    countDown--; //decrement countdown
    //Serial.print(countDown); //used for debugging to see the countdown
    previousMillisCountDown = currentMillis; //set prev to current millis
  } 
  else if (currentMillis >= interval && countDownDone == false || prevCode != code ) {
    countDownDone = true; //countdown is done 
    prevCode = code; //set prev code to current stored code
    HCMAX7219.Clear();
    //Write some current scanned code to the output buffer
    HCMAX7219.print7Seg(currCode, 8);
    //Send the output buffer to the display
    HCMAX7219.Refresh();
  }
  
  // Look for new tags
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return;
  }
  // Verify if the tag has been readed
  if ( ! rfid.PICC_ReadCardSerial()) {
    return;
  }

  readBlock(block, readbackblock);//get the numbers stored on the tag
 
  /*when the chip has been scanned.
    Stop scanning and wait for a new chip to appear
  */
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  CheckRfid(readbackblock);//call the CheckRfid method and send the numbers read from the tag along.

  /*If statement to check if the user scanned a tag while the initial countdown is happening.
   * if so SetupNewchips method will be start*/
  if (reset == true && currentMillis - previousMillis < interval) {
    //Serial.println("resetting chips!"); //text for debugging
    reset = false; //set reset false because resetting is not done yet
    HCMAX7219.Clear();
    //Write 0 to the output buffer */
    HCMAX7219.print7Seg("0", 8);
    //Send the output buffer to the display
    HCMAX7219.Refresh();
    SetupNewChips();

  }
}


/**/
void CheckRfid(byte readbackblock[18]) {

  int chipNumber = readbackblock[1] - '0';
  int sequelNumber = readbackblock[0] - '0';
  /*Match the UID with the UID in the 2D array.*/
  if (chipNumber >= -1) {
    //save current code 
    
    
    code += (String)chipNumber;//add the scanned number to the total current code
    currSequel += (String)sequelNumber;//add the scanned sequel number to the sequel code
    
    code.toCharArray(currCode, 6);
    Serial.print("current sequel: ");
    Serial.println(currSequel);
    //Clear the output buffer 
    HCMAX7219.Clear();
    //Write current code to the output buffer
    HCMAX7219.print7Seg(currCode, 8);
    //Send the output buffer to the display
    HCMAX7219.Refresh();
     //blink the green led
    BlinkGreen();
    //make beep sound
    BuzzerBeep();
    //add 1 to the scan counter
    scanCount++;

  } else {
    BlinkRed();
    BuzzerFail();
    //Serial.println("kon niet lezen");//Debugging text
  }

  /*if scanned sequel code is the same as sequel code, print debugging info.
    And start blinking the green led and reset the code and scan count*/
  if (finalCode == currSequel && scanCount == 5) {
    Serial.println("Nicuuuu de code klopt ");
    Serial.print(code);
    code = "";
    scanCount = 0;
    for (int i = 0; i < 5; i++) {
      BlinkGreen();
      HCMAX7219.Clear();
      HCMAX7219.Refresh();
      delay(500);
      HCMAX7219.print7Seg(currCode, 8);
      HCMAX7219.Refresh();

    }

    Serial.println(code);
    currSequel = "";
  }
  /*Else if codes don't match.
    reset scanCount and code and give user feedback*/
  else if (scanCount == 5) {
    scanCount = 0;
    currSequel = "";
    code = "";
    currSequel = "";
    delay(2000);
    HCMAX7219.Clear();
    HCMAX7219.print7Seg("FOUT!", 8);
    HCMAX7219.Refresh();
    BuzzerFail();
    for (int i = 0; i < 5; i++) {
      BlinkRed();
    }
  }
}

/*Method for blinking the red Led once*/
void BlinkRed() {
  digitalWrite(LED_2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_2, LOW);    // turn the LED off by making the voltage LOW

}

/*Method for blinking the Green Led once*/
void BlinkGreen() {
  digitalWrite(LED_1, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_1, LOW);    // turn the LED off by making the voltage LOW
  delay(500);
}

/*Method for buzzer to beep once*/
void BuzzerBeep() {
  int Melody[] = {1000, 1500
                 };
  int duration[] = {100, 200};
  for (int i = 0 ; i < 2; i++) {
    tone(Speaker, Melody[i], duration[i]);
    delay(100);
  }
}

/*Method for buzzer to make a fail sound once*/
void BuzzerFail() {
  int fail[] = {200, 100
               };
  int duration[] = {400, 800};
  for (int i = 0 ; i < 2; i++) {
    tone(Speaker, fail[i], duration[i]);
    delay(100);


  }

}

/*Start of the ne*/
void SetupNewChips() {
  Serial.print("Method Setup started");
  chipCounter = 0;
  bool allChipsScanned = false;
  int count = 0;
  while ( allChipsScanned == false) {
    WriteChip(false);

    if (chipCounter == 5) {
      scanCount = 0;
      allChipsScanned = true;
      chipCounter = 0;
      code = "";
      code.toCharArray(currCode, 6);
      HCMAX7219.Clear();
      HCMAX7219.print7Seg(currCode, 8);
      HCMAX7219.Refresh();
    }
  }

}

/*Method for writing a given number onto the chip*/
void WriteChip(bool chipWrite) {

  // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);
  Serial.print(buttonState);
  HCMAX7219.Clear();
  HCMAX7219.print7Seg(buttonPushCounter, 8);
  HCMAX7219.Refresh();

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      if (buttonPushCounter == 9) {
        buttonPushCounter = 0;
      }
      // if the current state is HIGH then the button went from off to on:
      buttonPushCounter++;
      Serial.println("on");
      Serial.print("number of button pushes: ");
      Serial.println(buttonPushCounter);

      HCMAX7219.Clear();
      //Write current button counter  to the output buffer
      HCMAX7219.print7Seg(buttonPushCounter, 8);
      //Send the output buffer to the display
      HCMAX7219.Refresh();
    } else {
      // if the current state is LOW then the button went from on to off:
      Serial.println("off");
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;


  char sequel[2];
  char number[2];
  String strSequel;
  String strNumber;
  strSequel = String(chipCounter);
  strNumber = String(buttonPushCounter);
  strSequel.toCharArray(sequel, 2);
  strNumber.toCharArray(number, 2);

  readbackblock[0] = sequel[0];
  readbackblock[1] = number[0];

  if (chipWrite == false) {

    // Look for new cards
    if ( ! rfid.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if ( ! rfid.PICC_ReadCardSerial()) {
      return;
    }

    block = 6;
    Serial.println("Scan a MIFARE Classic card");

    Serial.println("card selected");
    writeBlock(block,  readbackblock);//the blockcontent array is written into the card block

    readBlock(block, readbackblock);//read the block back
    for (int j = 0 ; j < 16 ; j++) //print the block contents
    {
      Serial.write (readbackblock[j]);//Serial.write() transmits the ASCII numbers as human readable characters to serial monitor
      chipWrite = true;
    }

    Serial.println("");

    if (chipWrite == true) {
      BlinkGreen();
      BuzzerBeep();
      chipCounter++;
      buttonPushCounter = 0;
      Serial.println(chipCounter);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }

  }
}




