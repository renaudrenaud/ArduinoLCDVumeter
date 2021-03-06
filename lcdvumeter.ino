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
const int xres = 16;
const int yres = 8;
const int xres2 = 32;
const int yres2 = 8;
const int gain = 3;
int decaytest = 1;
char im[64], data[64];
char Rim[64], Rdata[64];
char data_avgs[32];
float peaks[32];
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


LiquidCrystal_I2C lcd(0x3F, 16, 2);

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
  Serial.begin(9600);
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
  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(1, v1);
  lcd.createChar(2, v2);
  lcd.createChar(3, v3);
  lcd.createChar(4, v4);
  lcd.createChar(5, v5);
  lcd.createChar(6, v6);
  lcd.createChar(7, v7);
  lcd.createChar(8, v8);

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
    for (load = load + 2; load < 16; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(1);
    }
    lcd.setCursor(0, 0);
    lcd.print("  AudioFoLight  ");
    delay(2);
    
    ;
  }

    lcd.setCursor(0, 0);
    lcd.print("Sensitivity     ");
    lcd.setCursor(0, 1);
    lcd.print("= 16 (-4 to +24)");
    delay(3000);

  lcd.clear();
  delay(200);
}



void vu() {
  //  delay(10);
  for (i = 0; i < 64; i++) {
    if (sensitivity < 0)
      {val = ((analogRead(LCHAN) / sensitivity ));}
    if (sensitivity == 0)
      {val = ((analogRead(LCHAN) ));}
    if (sensitivity > 0)
      {val = ((analogRead(LCHAN) * sensitivity ));}

    //val = ((analogRead(LCHAN) * 16 ));  // chose how to interpret the data from analog in
    // Serial.print(val);
    // Serial.println();
    data[i] = val;
    im[i] = 0;
    if (channels == 2) {
      if (sensitivity < 0)
        {Rval = ((analogRead(RCHAN) / sensitivity ));}
      if (sensitivity == 0)
        {Rval = ((analogRead(RCHAN) ));}
      if (sensitivity > 0)
        {Rval = ((analogRead(RCHAN) * sensitivity ));}
      
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
  for (i = 0; i < 32 ; i++) {
    data[i] = sqrt(data[i] * data[i] + im[i] * im[i]);
  }
  if (channels == 2) {
    for (i = 16; i < 32 ; i++) {
      data[i] = sqrt(Rdata[i - 16] * Rdata[i - 16] + Rim[i - 16] * Rim[i - 16]);
    }
  }

  // todo: average as many or as little dynamically based on yres
  for (i = 0; i < 32; i++) {
    data_avgs[i] = (data[i]);// + data[i*2+1]);// + data[i*3 + 2]);// + data[i*4 + 3]);  // add 3 samples to be averaged, use 4 when yres < 16
    data_avgs[i] = constrain(data_avgs[i], 0, 9 - gain); //data samples * range (0-9) = 9
    data_avgs[i] = map(data_avgs[i], 0, 9 - gain, 0, yres);      // remap averaged values
  }

} // end loop








void decay(int decayrate) {
  //// reduce the values of the last peaks by 1
  if (decaytest == decayrate) {
    for (x = 0; x < 32; x++) {
      peaks[x] = peaks[x] - 1;  // subtract 1 from each column peaks
      decaytest = 0;

    }
  }
  decaytest++;

}



void Two16_LCD() {
  vu();
  decay(1);
  lcd.setCursor(0, 0);
  lcd.print("L"); // Channel ID replaces bin #0 due to hum & noise
  lcd.setCursor(0, 1);
  lcd.print("R"); // ditto

  for (int x = 1; x < 16; x++) {  // init 0 to show lowest band overloaded with hum
    int y = x + 16; // second display line
    if (data_avgs[x] > peaks[x]) peaks[x] = data_avgs[x];
    if (data_avgs[y] > peaks[y]) peaks[y] = data_avgs[y];

    lcd.setCursor(x, 0); // draw first (top) row Left
    if (peaks[x] == 0) {
      lcd.print("_");  // less LCD artifacts than " "
    }
    else {
      lcd.write(peaks[x]);
    }

    lcd.setCursor(x, 1); // draw second (bottom) row Right
    if (peaks[y] == 0) {
      lcd.print("_");
    }
    else {
      lcd.write(peaks[y]);

    }
  }
}

void mono() {
  vu();
  decay(1);
  lcd.setCursor(0, 0);
  lcd.print(" "); // Channel ID replaces bin #0 due to hum & noise
  lcd.setCursor(0, 1);
  lcd.print("M"); // ditto
  for (x = 1; x < 16; x++) { // repeat for each column of the display horizontal resolution
    y = data_avgs[x];  // get current column value
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];

    if (y <= 8) {
      lcd.setCursor(x, 0); // clear first row
      lcd.print(" ");
      lcd.setCursor(x, 1); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else {
      lcd.setCursor(x, 0); // draw first row
      if (y == 9) {
        lcd.write(32); // RC  was " "
      }
      else {
        lcd.write(y - 8); // same chars 1-8 as 9-16
      }
      lcd.setCursor(x, 1);
      lcd.write(8);
    } // end display
  }  // end xres
}

void stereo8() {
  vu();
  decay(1);
  lcd.setCursor(8, 1);
  lcd.print("R"); // Channel ID replaces bin #0 due to hum & noise
  for (x = 1; x < 8; x++) {
    y = data_avgs[x];
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];

    if (y <= 8) {
      lcd.setCursor(x, 0); // clear first row
      lcd.print(" ");
      lcd.setCursor(x, 1); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else {
      lcd.setCursor(x, 0); // draw first row
      if (y == 9) {
        lcd.write(32);
      }
      else {
        lcd.write(y - 8); // same chars 1-8 as 9-16
      }
      lcd.setCursor(x, 1);
      lcd.write(8);
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("L"); // ditto
  for (x = 17; x < 32; x++) {
    y = data_avgs[x];
    z = peaks[x];
    if (y > z) {
      peaks[x] = y;
    }
    y = peaks[x];

    if (y <= 8) {
      lcd.setCursor(x - 8, 0); // clear first row
      lcd.print(" ");
      lcd.setCursor(x - 8, 1); // draw second row
      if (y == 0) {
        lcd.print(" "); // save a glyph
      }
      else {
        lcd.write(y);
      }
    }
    else {
      lcd.setCursor(x - 8, 0); // draw first row
      if (y == 9) {
        lcd.write(32);
      }
      else {
        lcd.write(y - 8); // same chars 1-8 as 9-16
      }
      lcd.setCursor(x - 8, 1);
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
    sensitivity = sensitivity + 1;
    lcd.setCursor(0, 0);
    lcd.print("Sensitiv.: -4/24");
    lcd.setCursor(0, 1);
    lcd.print(sensitivity);
    delay(500);
  }
  
  if (sensitivity > 23) sensitivity = - 5;
  oldbuttonSensitivity = buttonSensitivity;

  
  switch (lightPattern) {
    case 1:
      bars();
      break;
    case 2:
      mono();
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
