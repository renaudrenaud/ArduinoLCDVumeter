/*
  modified by varind in 2013
  this code is public domain, enjoy!
*/
#include <Wire.h>
#include <FastLED.h>
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include <LiquidCrystal_I2C.h>
#include <fix_fft.h>

#define BUFF_MAX 128
#define LCHAN 0
#define RCHAN 1
int buttonPin = 0;      // momentary push button on pin 0
int buttonPin1 = 1;     // momentary push for sensitivity change
int oldButtonVal = 0;
int oldbuttonSensitivity = 0;
const int channels = 2;
const int xres = 16; //16;
const int yres = 8; // maybe define the high, meaning resolution
const int xres2 = 32; //32;
const int yres2 = 8;
const int gain = 3;
int decaytest = 1;
int Rdecaytest = 1;
char im[64], data[64];
char Rim[64], Rdata[64];
char data_avgs[64];
char Rdata_avgs[64];
float peaks[20];
float Rpeaks[20];
int i = 0, val, Rval;
int x = 0, y = 0, z = 0;
int load;
int nPatterns = 9 ;
int lightPattern = 1;
int sensitivity = 16;
int   lmax[2];                                        // level max memory
int   dly[2];   // delay & speed for peak return
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 1000;


LiquidCrystal_I2C lcd(0x3F, 20, 4);

// VU METER CHARACTERS
byte v1[8] = {
  B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111
};
byte v2[8] = {
  B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111
};
byte v3[8] = {
  B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111
};
byte v4[8] = {
  B00000, B00000, B00000, B00000, B11111, B11111, B11111, B11111
};
byte v5[8] = {
  B00000, B00000, B00000, B11111, B11111, B11111, B11111, B11111
};
byte v6[8] = {
  B00000, B00000, B11111, B11111, B11111, B11111, B11111, B11111
};
byte v7[8] = {
  B00000, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};
byte v8[8] = {
  B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};


void setup() {
  // Serial.begin(9600);
  lcd.backlight();
  Wire.begin();
  // DS3231_init(DS3231_INTCN);
  memset(recv, 0, BUFF_MAX);
  // rtc.begin();
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);  // button pin is HIGH, so it drops to 0 if pressed
  pinMode(buttonPin1, INPUT);
  digitalWrite(buttonPin1, HIGH);  // button pin is HIGH, so it drops to 0 if pressed

  //setTheTime("305017101012018");     // ssmmhhDDMMYYYY set time once in the given format
  lcd.begin(20, 4);
  lcd.clear();
  lcd.createChar(1, v1);
  lcd.createChar(2, v2);
  lcd.createChar(3, v3);
  lcd.createChar(4, v4);
  lcd.createChar(5, v5);
  lcd.createChar(6, v6);
  lcd.createChar(7, v7);
  lcd.createChar(8, v8);

  lcd.setCursor(0, 2);
  lcd.print("Sensitivity hit TXO ");
  lcd.setCursor(0, 3);
  lcd.print("16=def (-4 to +38)  ");

  for (i = 0; i < 80; i++)
  {
    for (load = 0; load < i / 5; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(1);
    }
    if (load < 1)
    {
      lcd.setCursor(0, 1);
      lcd.write(7);
    }

    lcd.setCursor(load + 1, 1);
    lcd.write((i - i / 5 * 5) + 1);
    for (load = load + 2; load < 20; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(1);
    }
    lcd.setCursor(0, 0);
    lcd.print("AudioFoLight VuMeter");


    delay(2);

  }



  lcd.clear();
  delay(200);
}



void vu() {
  //  delay(10);
  for (i = 0; i < 64; i++) {
    if (sensitivity < 0)
    {
      val = ((analogRead(LCHAN) / sensitivity ));
    }
    if (sensitivity == 0)
    {
      val = ((analogRead(LCHAN) ));
    }
    if (sensitivity > 0)
    {
      val = ((analogRead(LCHAN) * sensitivity ));
    }

    data[i] = val;
    im[i] = 0;
    if (channels == 2) {
      if (sensitivity < 0)
      {
        Rval = ((analogRead(RCHAN) / sensitivity ));
      }
      if (sensitivity == 0)
      {
        Rval = ((analogRead(RCHAN) ));
      }
      if (sensitivity > 0)
      {
        Rval = ((analogRead(RCHAN) * sensitivity ));
      }

      //Rval = ((analogRead(RCHAN) * 16 ));  // chose how to interpret the data from analog in
      Rdata[i] = Rval;
      Rim[i] = 0;
    }
  };

  fix_fft(data, im, 6, 0); // Send the data through fft
  if (channels == 2) {
    fix_fft(Rdata, Rim, 6, 0); // Send the data through fft
  }

  // get the absolute value of the values in the array, so we're only dealing with positive numbers
  for (i = 0; i < 64 ; i++) {
    data[i] = sqrt(data[i] * data[i] + im[i] * im[i]);
  }
  if (channels == 2) {
    for (i = 0; i < 64 ; i++) {
      Rdata[i] = sqrt(Rdata[i] * Rdata[i] + Rim[i] * Rim[i]);
    }
  }

  // todo: average as many or as little dynamically based on yres
  for (i = 0; i < 64; i++) {
    data_avgs[i] = (data[i]);// + data[i*2+1]);// + data[i*3 + 2]);// + data[i*4 + 3]);  // add 3 samples to be averaged, use 4 when yres < 16
    data_avgs[i] = constrain(data_avgs[i], 0, 9 - gain); //data samples * range (0-9) = 9
    data_avgs[i] = map(data_avgs[i], 0, 9 - gain, 0, yres);      // remap averaged values

    Rdata_avgs[i] = (Rdata[i]);// + data[i*2+1]);// + data[i*3 + 2]);// + data[i*4 + 3]);  // add 3 samples to be averaged, use 4 when yres < 16
    Rdata_avgs[i] = constrain(Rdata_avgs[i], 0, 9 - gain); //data samples * range (0-9) = 9
    Rdata_avgs[i] = map(Rdata_avgs[i], 0, 9 - gain, 0, yres);      // remap averaged values
  }

} // end loop








void decay(int decayrate) {
  //// reduce the values of the last peaks by 1
  if (decaytest == decayrate) {
    for (x = 0; x < 20; x++) {
      peaks[x] = peaks[x] - 1;  // subtract 1 from each column peaks
      decaytest = 0;

    }
  }
  decaytest++;

}

void Rdecay(int decayrate) {
  //// reduce the values of the last peaks by 1
  if (Rdecaytest == decayrate) {
    for (x = 0; x < 20; x++) {
      Rpeaks[x] = Rpeaks[x] - 1;  // subtract 1 from each column peaks
      Rdecaytest = 0;

    }
  }
  Rdecaytest++;

}

void Two16_LCD() {
  vu();
  decay(1);
  Rdecay(1);
  lcd.setCursor(0, 0);
  lcd.print("L"); // Channel ID replaces bin #0 due to hum & noise
  lcd.setCursor(0, 1);
  lcd.print("R"); // ditto

  for (int x = 1; x < 20; x++) {  // init 0 to show lowest band overloaded with hum
    //int y = x + 16; // second display line
    if (data_avgs[x] > peaks[x]) peaks[x] = data_avgs[x];
    if (Rdata_avgs[x] > Rpeaks[x]) Rpeaks[x] = Rdata_avgs[x];

    lcd.setCursor(x, 0); // draw first (top) row Left
    if (peaks[x] == 0) {
      lcd.print("_");  // less LCD artifacts than " "
    }
    else {
      lcd.write(peaks[x]);
    }

    lcd.setCursor(x, 1); // draw second (bottom) row Right
    if (Rpeaks[x] == 0) {
      lcd.print("_");
    }
    else {
      lcd.write(Rpeaks[x]);

    }
  }
}

void mono(int pos) {
  int linePos = 0;
  if (pos == 0) {
    linePos = 2;
  }
  vu();
  decay(1);
  lcd.setCursor(0, linePos);
  lcd.print(" "); // Channel ID replaces bin #0 due to hum & noise
  lcd.setCursor(0, linePos + 1);
  lcd.print("M"); // ditto
  lcd.setCursor(16, linePos + 1);
  lcd.print("Mono"); // ditto
  for (x = 1; x < 16; x++) { // repeat for each column of the display horizontal resolution
    y = data_avgs[x];  // get current column value
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];

    if (y <= 8) {
      lcd.setCursor(x, linePos); // clear first row
      lcd.print(" ");
      lcd.setCursor(x, linePos + 1); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else {
      lcd.setCursor(x, linePos); // draw first row
      if (y == 9) {
        lcd.write(32); // RC  was " "
      }
      else {
        lcd.write(y - 8); // same chars 1-8 as 9-16
      }
      lcd.setCursor(x, linePos + 1);
      lcd.write(8);
    } // end display
  }  // end xres
}

void stereo8(){
  vu();
   decay(1); 
   lcd.setCursor(10, 1);
  lcd.print("R"); // Channel ID replaces bin #0 due to hum & noise
  for (x=1; x < 10; x++) {
    y = data_avgs[x];
        z= peaks[x];
    if (y > z){
      peaks[x]=y;
    }
    y= peaks[x]; 

    if (y <= 9){            
      lcd.setCursor(x,0); // clear first row
      lcd.print(" ");
      lcd.setCursor(x,1); // draw second row
      if (y == 0){
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else{
      lcd.setCursor(x,0);  // draw first row
      if (y == 10){
        lcd.write(32);  
      }
      else {
        lcd.write(y-10);  // same chars 1-8 as 9-16
      }
      lcd.setCursor(x,1);
      lcd.write(8);  
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("L"); // ditto
  for (x=17; x < 32; x++) {
    y = data_avgs[x];
       z= peaks[x];
    if (y > z){
      peaks[x]=y;
    }
    y= peaks[x]; 

    if (y <= 8){            
      lcd.setCursor(x-8,0); // clear first row
      lcd.print(" ");
      lcd.setCursor(x-8,1); // draw second row
      if (y == 0){
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else{
      lcd.setCursor(x-8,0);  // draw first row
      if (y == 9){
        lcd.write(32);  
      }
      else {
        lcd.write(y-8);  // same chars 1-8 as 9-16
      }
      lcd.setCursor(x-8,1);
      lcd.write(8);  
    }
  }
}


void  bars  ()
{
  decay(1);
  vu();
  //  lcd.createChar( i,block[i] );
#define T_REFRESH    100            // msec bar refresh rate
#define T_PEAKHOLD   5*T_REFRESH    // msec peak hold time before return
  long  lastT = 0;
  //  if( millis()<lastT )
  //    return;
  //  lastT += T_REFRESH;
  int anL = map( sqrt( analogRead( LCHAN  ) * 16 ), 0, 128, 0, 80 ); // sqrt to have non linear scale (better was log)
  int anR = map( sqrt( analogRead( RCHAN ) * 16 ), 0, 128, 0, 80 );
  bar( 0, anL );
  bar( 1, anR );
}

void  bar  ( int rows, int levs )
{
  lcd.setCursor( 0, rows );
  lcd.write( rows ? 'R' : 'L' );
  for ( int i = 1 ; i < 16 ; i++ )
  {
    int f = constrain( levs      - i * 5, 0, 5 );
    int p = constrain( lmax[rows] - i * 5, 0, 6 );
    if ( f )
      lcd.write(8);
    else
      lcd.write(32);
  }
  if ( levs > lmax[rows] )
  {
    lmax[rows] = levs;
    dly[rows]  = -(T_PEAKHOLD) / T_REFRESH;              // Starting delay value. Negative=peak don't move
  }
  else
  {
    if ( dly[rows] > 0 )
      lmax[rows] -= dly[rows];

    if ( lmax[rows] < 0 )
      lmax[rows] = 0;
    else
      dly[rows]++;
  }
}



void stereo16() {
  vu();
  decay(1);
  lcd.setCursor(0, 0);
  lcd.print(" "); // Channel ID replaces bin #0 due to hum & noise

  for (x = 1; x < 16; x++) {
    y = data_avgs[x];
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];
    if (x < xres) {
      lcd.setCursor(x, 0); // draw first row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
  }
  lcd.setCursor(0, 1);
  lcd.print(" "); // Channel ID replaces bin #0 due to hum & noise
  for (x = 17; x < 32; x++) {
    y = data_avgs[x];
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];
    if (x - 16 < xres) {
      lcd.setCursor(x - 16, 1); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
  }
}


void thirtytwoband() {
  vu();
  decay(1);
  for (x = 0; x < 32; x++) {
    y = data_avgs[x];

    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];
    if (x < 16) {
      lcd.setCursor(x, 0); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else {
      lcd.setCursor(x - 16, 1);
      if (y == 0) {
        lcd.print(" ");
      }
      else {
        lcd.write(y);
      }
    }
  }
}

void simple() {
  lcd.setCursor(0, 0);
  lcd.print("  AudioFoLight  ");
  lcd.setCursor(0, 1);
  lcd.print(" Time: ");
  lcd.print("No clock");

}



// the loop routine runs over and over again forever;
void loop() {
  // read that state of the pushbutton value;
  int buttonVal = digitalRead(buttonPin);
  if (buttonVal == LOW && oldButtonVal == HIGH) {// button has just been pressed
    lightPattern = lightPattern + 1;
  }

  if (lightPattern > nPatterns) lightPattern = 1;
  oldButtonVal = buttonVal;


  int buttonSensitivity = digitalRead(buttonPin1);
  if (buttonSensitivity == LOW && oldbuttonSensitivity == HIGH) {// button has just been pressed
    if (sensitivity < 5){sensitivity = sensitivity + 1;}
    else{
      sensitivity = sensitivity + 4;}
    lcd.setCursor(0, 0);
    lcd.print("Sensitiv.: -4/40 =");
    lcd.setCursor(18, 0);
    lcd.print(sensitivity);
    delay(500);
  }

  if (sensitivity > 36) sensitivity = - 8;
  oldbuttonSensitivity = buttonSensitivity;


  switch (lightPattern) {
    case 1:
      // mono(0);// bars();
      Two16_LCD();
      break;
    case 2:

      break;
    case 3:
      stereo8();
      break;
    case 4:
      stereo16();
      break;
    case 5:
      Two16_LCD();
      break;
    case 6:
      All();
      break;
    case 7:
      stereo16();
      break;
    case 8:
      bars();
      break;
    case 9:
      lcd.clear();
      break;

  }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {bars, mono, stereo8, stereo16, Two16_LCD };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}
void All()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  EVERY_N_SECONDS( 20 ) {
    nextPattern();  // change patterns periodically
  }
}
// second list
