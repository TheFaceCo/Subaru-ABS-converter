const unsigned long halfwave = 200;

unsigned long period1 = 100;
unsigned long periodflag1;
unsigned long delayflag1;
unsigned long signal1start;
unsigned long signal1send;
unsigned long signal1end;
unsigned long periodtimer1;
unsigned long microsISRbuffer1;
unsigned long period1buffer;

unsigned long period2 = 100;
unsigned long periodflag2;
unsigned long delayflag2;
unsigned long signal2start;
unsigned long signal2send;
unsigned long signal2end;
unsigned long periodtimer2;
unsigned long microsISRbuffer2;
unsigned long period2buffer;

unsigned long VSSfactor1;
unsigned long VSSfactor2;
unsigned long VSSperiodbuffer;
unsigned long VSSperiod = 10000000;
unsigned long VSSperiodtimer;
unsigned long VSSdelayflag;
unsigned long VSSlightoff = 0;

int complement1 = 50; // must add to exactly 100. int to avoid float math
int complement2 = 50;

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

#define minimumspeed 1000



const unsigned long inputwheelteeth = 44;
const unsigned long outputwheelteeth = 96;
const unsigned long VSSwheelteeth = 2; //this number is made up. Set to correct wheel count from transmission


bool signal1started = false;
bool signal1sent = false;
bool signal1ended = false;

bool signal2started = false;
bool signal2sent = false;
bool signal2ended = false;

bool input1found = false;
bool input2found = false;

bool VSStoggle = false;

bool serial1toggle = false;
bool serial2toggle = false;

bool enabled = false;

void setup() {
  // put your setup code here, to run once:
  //  Serial.begin(115200);
  //  Serial.println("start");

  //wheelspeed outputs use two output pins through a voltage divider for 0, 1.75, and 3v (00, 01, and 11)
  pinMode(WSS1PIN1, OUTPUT); //wheelspeed 1 output 1
  pinMode(WSS1PIN2, OUTPUT); //wheelspeed 1 output 2
  pinMode(WSS2PIN1, OUTPUT);//wheelspeed 2 output 1
  pinMode(WSS2PIN2, OUTPUT);//wheelspeed 2 output 2

  pinMode(VSSPIN, OUTPUT);//VSS output
  pinMode(OE9921, OUTPUT);//Output enable to MAX9921
  pinMode(16, OUTPUT);//Diag to MAX9921
  pinMode(17, INPUT); //ERR from MAX9921
  pinMode(OUT19921, INPUT); //Hall 1 signal from MAX9921
  pinMode(OUT29921, INPUT); //Hall 2 signal from MAX9921

  pinMode(ONBOARDLED, OUTPUT); //onboard LED to display power
  digitalWriteFast(ONBOARDLED, HIGH); //display power state

  delay(10); //give the max some time to get ready.
  digitalWriteFast(OE9921, HIGH); //enable max9921
  delay(10);

  attachInterrupt(digitalPinToInterrupt(OUT19921), input1ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(OUT29921), input2ISR, RISING);

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
      if (!signal1started) {
        //    if (serial1toggle) {
        //      Serial.println("1ping");
        //      serial1toggle = !serial1toggle;
        //    }
        //    else {
        //      Serial.println("1gnip");
        //      serial1toggle = !serial1toggle;
        //    }
        period1buffer = microsISRbuffer1 - periodflag1; //calculate elapsed time since last ISR event
        periodflag1 = microsISRbuffer1; //set flag to 'current' time from ISR

        period1buffer = period1buffer * inputwheelteeth; //scale period up by input wheel count

        VSSfactor1 = period1buffer; //store scaled period for VSS

        period1buffer = period1buffer / outputwheelteeth; //scale period down by output wheel count

        period1buffer = period1buffer * complement1; //store period with filter
        period1 = period1 * complement2;
        period1 = period1 + period1buffer;
        period1 = period1 / 100;

        //period1 = period1buffer; //store scaled period directly without filtering

        signal1start = period1 - (2 * halfwave); //backdate period to beginning of waveform

        input1found = false;
        updateVSS();
      }
    }

    if (input2found) {
      if (!signal2started) {
        //    if (serial2toggle) {
        //      Serial.println("2ping");
        //      serial2toggle = !serial2toggle;
        //    }
        //    else {
        //      Serial.println("2gnip");
        //      serial2toggle = !serial2toggle;
        //    }
        period2buffer = microsISRbuffer2 - periodflag2;
        periodflag2 = microsISRbuffer2;

        period2buffer = period2buffer * inputwheelteeth;

        VSSfactor2 = period2buffer;

        period2buffer = period2buffer / outputwheelteeth;

        period2buffer = period2buffer * complement1;
        period2 = period2 * complement2;
        period2 = period2 + period2buffer;
        period2 = period2 / 100;

        //period2 = period2buffer; //removed filter

        //end of signal occurs when the output stage is returned to neutral (1 output HIGH, one output LOW). The prior event it the zero crossing (outputs both LOW). The next prior event is the start of the wave (both outputs HIGH)

        signal2start = period2 - (2 * halfwave); //backdate period to beginning of waveform

        input2found = false;
        updateVSS();
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
    if (VSSlightoff < 1000) {
      if (VSSperiodtimer - VSSdelayflag >= VSSperiod) {
        if (VSStoggle) {
          digitalWriteFast(VSSPIN, HIGH);
          VSStoggle = !VSStoggle;
          VSSlightoff++;
        }
        else {
          digitalWriteFast(VSSPIN, LOW);
          VSStoggle = !VSStoggle;
        }
        VSSdelayflag = VSSperiodtimer;
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

void updateVSS() {
  VSSperiodbuffer = VSSfactor1 + VSSfactor2;
  VSSperiodbuffer = VSSperiodbuffer / 2; //take the average of the two wheel speed sensors
  //  VSSperiodbuffer = VSSperiodbuffer * inputwheelteeth;  //removed becuase this math is being done up top, to reduce duplicate effort
  VSSperiodbuffer = VSSperiodbuffer / VSSwheelteeth;

  //  VSSperiodbuffer = VSSperiodbuffer * complement1;
  //  VSSperiod = VSSperiod * complement2;
  //  VSSperiod = VSSperiod + VSSperiodbuffer;
  //  VSSperiod = VSSperiod / 200; //divide by 200 instead of 100 because we'll be toggling VSS on or off every half period

  VSSperiod = VSSperiodbuffer / 2;

}
