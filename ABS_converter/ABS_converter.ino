#define debug1 0 //serial output of pulse count data from input 1
#define debug2 0 //input 2
#define debugout1 0 //output1
#define debugout2 0 //output2
int debugincount1 = 0;
int debugoutcount1 = 0;
int debugincount2 = 0;
int debugoutcount2 = 0;

#define VSSoutput 0 //set to one for output to run as VSS. Set to 0 for use as GPIO. Default is temporarily showing an average of both WSS sensors with an LED.
#define VSSlightoff 750 //how long to display VSS on output when VSS is disabled

//Defined as const variables to ensure type, to prevent speed problems with type conversions
const unsigned long inputwheelteeth = 96; //Encoder has 48 N and 48 S poles. The max9921 sees each as 1 or 0, and "no magnet" is an indeterminate state.
const unsigned long outputwheelteeth = 44;
const unsigned long VSSwheelteeth = 1; //this number is made up. Set to correct wheel count from transmission

#define WSS1PIN1  3
#define WSS1PIN2  4
#define WSS2PIN1  5
#define WSS2PIN2  6
#define VSSPIN    7
#define OE9921    15
#define DIAG9921  16
#define ERR9921   17
#define OUT19921  18
#define OUT29921  19
#define ONBOARDLED 13

unsigned long halfwave = 300; //used to determined the shape of the output pulse
#define longhalfwave  300 //seems like ideally the waveform should be about 600uS long, but that needs to scale down at higher speeds.

#define minimumspeed 1000 //originally used to set a floor to the WSS outputs. Doesn't seem to be required, left for posterity. 

const unsigned long complement1 = 70; // Used as a running average of two input periods. Must add to exactly 100. int to avoid float math
const unsigned long complement2 = 30;

unsigned long period1 = 100;
unsigned long periodflag1;
unsigned long delayflag1;
unsigned long signal1start;
unsigned long signal1send;
unsigned long signal1end;
unsigned long periodtimer1;
unsigned long microsISRbuffer1;
unsigned long period1buffer;
unsigned long oldperiod1buffer;

unsigned long period2 = 100;
unsigned long periodflag2;
unsigned long delayflag2;
unsigned long signal2start;
unsigned long signal2send;
unsigned long signal2end;
unsigned long periodtimer2;
unsigned long microsISRbuffer2;
unsigned long period2buffer;
unsigned long oldperiod2buffer;

unsigned long VSSfactor1;
unsigned long VSSfactor2;
unsigned long VSSperiodbuffer;
unsigned long VSSperiod = 10000000;
unsigned long VSSperiodtimer;
unsigned long VSSdelayflag;
unsigned long VSSlightcount = 0;

bool signal1started = false; //these are used to find the start, middle, and end of the output waveform.
bool signal1sent = false;
bool signal1ended = false;

bool signal2started = false;
bool signal2sent = false;
bool signal2ended = false;

bool input1found = false; //ISR flags
bool input2found = false;

bool VSStoggle = false; //VSS output toggles high/low every half period

bool enabled = false; //output is not enabled until the first input pulse is found.
bool error = false; //

bool errreport;
bool ERRstatus;
bool OUT1status;
bool OUT2status;

void setup() {
  // put your setup code here, to run once:
  //  if (debug1 || debug2 || debugout1 || debugout2) {
  Serial.begin(115200);
  delay(500);
  Serial.println("ABS Converter");
  Serial.print("Input 1 debug: ");
  Serial.println(debug1);
  Serial.print("Input 2 debug: ");
  Serial.println(debug2);
  Serial.print("Output 1 debug: ");
  Serial.println(debugout1);
  Serial.print("Output 2 debug: ");
  Serial.println(debugout2);

  //  }
  //wheelspeed outputs use two output pins through a voltage divider for 0, 1.75, and 3v (00, 01, and 11)
  pinMode(WSS1PIN1, OUTPUT); //wheelspeed 1 output 1
  pinMode(WSS1PIN2, OUTPUT); //wheelspeed 1 output 2
  pinMode(WSS2PIN1, OUTPUT);//wheelspeed 2 output 1
  pinMode(WSS2PIN2, OUTPUT);//wheelspeed 2 output 2

  pinMode(VSSPIN, OUTPUT);//VSS output
  pinMode(OE9921, OUTPUT);//Output enable to MAX9921
  pinMode(DIAG9921, OUTPUT);//Diag to MAX9921
  pinMode(ERR9921, INPUT); //ERR from MAX9921
  pinMode(OUT19921, INPUT); //Hall 1 signal from MAX9921
  pinMode(OUT29921, INPUT); //Hall 2 signal from MAX9921

  pinMode(ONBOARDLED, OUTPUT); //onboard LED to display power
  digitalWriteFast(ONBOARDLED, HIGH); //display power state

  digitalWriteFast(DIAG9921, LOW); //enforce DIAG port state

  delay(10); //give the max some time to get ready.
  digitalWriteFast(OE9921, HIGH); //enable max9921
  delay(10);

  Serial.println("Max9921 enabled");

  attachInterrupt(digitalPinToInterrupt(OUT19921), input1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(OUT29921), input2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ERR9921), ERRISR, FALLING);

  delayflag1 = micros(); //sets up initial conditions for WSS output waveform
  periodflag1 = delayflag1; //avoid second call to micros(), trickrepeated throughout
  signal1send = period1 - halfwave;
  signal1end = period1;
  signal1start = signal1send - halfwave;

  delayflag2 = delayflag1;
  periodflag2 = delayflag1;
  signal2send = period2 - halfwave;
  signal2end = period2;
  signal2start = signal2send - halfwave;

  VSSdelayflag = delayflag1;

  digitalWriteFast(WSS1PIN1, HIGH);
  digitalWriteFast(WSS1PIN2, LOW);
  digitalWriteFast(WSS2PIN1, HIGH);
  digitalWriteFast(WSS2PIN2, LOW);
  digitalWriteFast(VSSPIN, HIGH);

  updateVSS();

}

void loop() {
  periodtimer1 = micros();
  periodtimer2 = periodtimer1;
  VSSperiodtimer = periodtimer1;
  if (enabled) {
    if (input1found) {
      if (debug1) {
        debugincount1++;
        Serial.print("in1: ");
        Serial.println(debugincount1);
      }
      period1buffer = microsISRbuffer1 - periodflag1; //calculate elapsed time since last ISR event
      periodflag1 = microsISRbuffer1; //set flag to 'current' time from ISR

      period1buffer = period1buffer * inputwheelteeth; //scale period up by input wheel count

      VSSfactor1 = period1buffer; //store scaled period for VSS

      period1buffer = period1buffer / outputwheelteeth; //scale period down by output wheel count

      period1buffer = period1buffer * complement1; //store period with filter
      oldperiod1buffer = period1;
      oldperiod1buffer = oldperiod1buffer * complement2;
      period1buffer = oldperiod1buffer + period1buffer;
      period1buffer = period1buffer / 100;

      input1found = false;
      updateVSS();

      if (period1buffer < 2 * longhalfwave + 50) { //scale waveform dynamically from longest wavelength as speed increases
        halfwave = period1buffer - 50;
        halfwave = halfwave / 2;
      }
      else {
        halfwave = longhalfwave;
      }

      //period1 = period1buffer; //store scaled period directly without filtering

      if (!signal1started) {
        period1 = period1buffer;
        signal1start = period1 - (2 * halfwave); //backdate period to beginning of waveform
      }
    }

    if (input2found) {
      if (debug2) {
        debugincount2++;
        Serial.print("in2: ");
        Serial.println(debugincount2);
      }
      period2buffer = microsISRbuffer2 - periodflag2;
      periodflag2 = microsISRbuffer2;

      period2buffer = period2buffer * inputwheelteeth;

      VSSfactor2 = period2buffer;

      period2buffer = period2buffer / outputwheelteeth;

      period2buffer = period2buffer * complement1;
      oldperiod2buffer = period2;
      oldperiod2buffer = oldperiod2buffer * complement2;
      period2buffer = oldperiod2buffer + period2buffer;
      period2buffer = period2buffer / 100;

      input2found = false;
      updateVSS();

      if (period2buffer < 2 * longhalfwave + 50) {
        halfwave = period2buffer - 50;
        halfwave = halfwave / 2;
      }
      else {
        halfwave = longhalfwave;
      }

      //period2 = period2buffer; //removed filter

      //end of signal occurs when the output stage is returned to neutral (1 output HIGH, one output LOW). The prior event it the zero crossing (outputs both LOW). The next prior event is the start of the wave (both outputs HIGH)
      if (!signal2started) {
        period2 = period2buffer; //store new value into active variable only between pulses.
        signal2start = period2 - (2 * halfwave); //backdate period to beginning of waveform
      }
    }

    //generate waveform for output 1
    //    if (period1 > minimumspeed) {
    if (periodtimer1 - delayflag1 >= signal1start) {
      if (!signal1started) {
        digitalWriteFast(WSS1PIN1, LOW);
        digitalWriteFast(WSS1PIN2, LOW);
        signal1started = true;
        signal1send = signal1start + halfwave; //re-find zero crossing center in order to tolerate rapidly changing periods. If this is preloaded, signal distortion will result when the period is updated at a bad time.
        signal1end = signal1send + halfwave; //re-find waveform end (should correspond to the period on average)
      }
      if (!signal1sent) {
        if (periodtimer1 - delayflag1 >= signal1send) {
          digitalWriteFast(WSS1PIN1, HIGH);
          digitalWriteFast(WSS1PIN2, HIGH);
          signal1sent = true;
          signal1ended = false;
        }
      }
      if (!signal1ended) {
        if (periodtimer1 - delayflag1 >= signal1end) {
          digitalWriteFast(WSS1PIN1, HIGH);
          digitalWriteFast(WSS1PIN2, LOW);
          delayflag1 = periodtimer1;
          signal1ended = true;
          signal1started = false;
          signal1sent = false;
          if (debugout1) {
            debugoutcount1++;
            Serial.print("out1: ");
            Serial.println(debugoutcount1);
          }
        }
      }
    }
    //    }
    //    else {
    //      digitalWriteFast(WSS1PIN1, HIGH);
    //      digitalWriteFast(WSS1PIN2, LOW);
    //      delayflag1 = periodtimer1;
    //      signal1ended = true;
    //      signal1started = false;
    //      signal1sent = false;
    //    }

    //generate waveform for output 2
    //    if (period2 > minimumspeed) {
    if (periodtimer2 - delayflag2 >= signal2start) {
      if (!signal2started) {
        digitalWriteFast(WSS2PIN1, LOW);
        digitalWriteFast(WSS2PIN2, LOW);
        signal2started = true;
        signal2send = signal2start + halfwave; //re-find zero crossing center in order to tolerate rapidly changing periods. If this is preloaded, signal distortion will result when the period is updated at a bad time.
        signal2end = signal2send + halfwave; //re-find waveform end (should correspond to the period on average)
      }
      if (!signal2sent) {
        if (periodtimer2 - delayflag2 >= signal2send) {
          digitalWriteFast(WSS2PIN1, HIGH);
          digitalWriteFast(WSS2PIN2, HIGH);
          signal2sent = true;
          signal2ended = false;
        }
      }
      if (!signal2ended) {
        if (periodtimer2 - delayflag2 >= signal2end) {
          digitalWriteFast(WSS2PIN1, HIGH);
          digitalWriteFast(WSS2PIN2, LOW);
          delayflag2 = periodtimer2;
          signal2ended = true;
          signal2started = false;
          signal2sent = false;
          if (debugout2) {
            debugoutcount2++;
            Serial.print("out2: ");
            Serial.println(debugoutcount2);
          }
        }
      }
    }
    //    }
    //    else {
    //      digitalWriteFast(WSS2PIN1, HIGH);
    //      digitalWriteFast(WSS2PIN2, LOW);
    //      delayflag2 = periodtimer2;
    //      signal2ended = true;
    //      signal2started = false;
    //      signal2sent = false;
    //    }

    //generate waveform for VSS
    if (VSSlightcount < VSSlightoff) {
      if (VSSperiodtimer - VSSdelayflag >= VSSperiod) {
        if (VSStoggle) {
          digitalWriteFast(VSSPIN, HIGH);
          VSStoggle = !VSStoggle;
          if (!VSSoutput) {
            VSSlightcount++;
          }
        }
        else {
          digitalWriteFast(VSSPIN, LOW);
          VSStoggle = !VSStoggle;
        }
        VSSdelayflag = VSSperiodtimer;
      }
    }
  }
  if (error) {
    Serial.println("Error reported by Max9921");
    enabled = false;
    digitalWrite(DIAG9921, HIGH);
    delay(5); //allow a bit of time for error output. 350ns according to Max9921 documentation.
    ERRstatus = digitalRead(ERR9921);
    OUT1status = digitalRead(OUT19921);
    OUT2status = digitalRead(OUT29921);
    Serial.print("Err pin: ");
    Serial.println(ERRstatus);
    Serial.print("Hall 1: ");
    Serial.println(OUT1status);
    Serial.print("Hall 2: ");
    Serial.println(OUT2status); //refer to diagnostic table in Max9921 documentation.
    error = false; //clear error but leave Max9921 disabled.
    errreport = true;
  }
  if (errreport) {
    for (int i = 0; i <= 10; i++) {
      digitalWriteFast(VSSPIN, HIGH);
      delay(4000);
      if (ERRstatus) {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(100);
        digitalWriteFast(VSSPIN, LOW);
        delay(1000);
        digitalWriteFast(VSSPIN, HIGH);
      }
      else {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(1000);
      }
      if (OUT1status) {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(100);
        digitalWriteFast(VSSPIN, LOW);
        delay(1000);
        digitalWriteFast(VSSPIN, HIGH);
      }
      else {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(1000);
      }
      if (OUT2status) {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(100);
        digitalWriteFast(VSSPIN, LOW);
        delay(1000);
        digitalWriteFast(VSSPIN, HIGH);
      }
      else {
        digitalWriteFast(VSSPIN, LOW);
        delay(100);
        digitalWriteFast(VSSPIN, HIGH);
        delay(1000);
      }
    }
  }
}

void input1ISR() {
  microsISRbuffer1 = micros();
  enabled = true;
  input1found = true; //set a flag to indicate that the ISR has been accessed
}

void input2ISR() {
  microsISRbuffer2 = micros();
  enabled = true;
  input2found = true; //set a flag to indicate that the ISR has been accessed
}

void ERRISR() {
  error = true;
}

void updateVSS() {
  VSSperiodbuffer = VSSfactor1 + VSSfactor2;
  VSSperiodbuffer = VSSperiodbuffer / 2; //take the average of the two wheel speed sensors
  VSSperiodbuffer = VSSperiodbuffer / VSSwheelteeth;

  VSSperiodbuffer = VSSperiodbuffer * complement1; // filter VSS
  VSSperiod = VSSperiod * complement2;
  VSSperiod = VSSperiod + VSSperiodbuffer;
  VSSperiod = VSSperiod / 200; //divide by 200 instead of 100 because we'll be toggling VSS on or off every half period

  // VSSperiod = VSSperiodbuffer / 2; without filter

}
