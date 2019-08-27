unsigned long period = 1000000;
unsigned long halfwave = 200;
unsigned long periodflag1;
unsigned long delayflag1;
unsigned long signalstart;
unsigned long signalsend;
unsigned long signalend;
unsigned long periodtimer1;
unsigned long microsISRbuffer;
unsigned long periodbuffer;

int complement1 = 97;
int complement2 = 3;

#define WSS1PIN1  3
#define WSS1PIN2  4
#define WSS2PIN1  5
#define WSS2PIN2  6
#define OE9921    15
#define DIAG9921  16
#define ERR9921   17
#define OUT19921  18
#define OUT29921  19
#define VSSPIN    20


const unsigned long inputwheelteeth = 3;
const unsigned long outputwheelteeth = 1;


bool signalstarted = false;
bool signalsent = false;
bool signalended = false;
bool inputfound = false;

void setup() {
  // put your setup code here, to run once:
//  Serial.begin(115200);
//  Serial.println("start");
  pinMode(WSS1PIN1, OUTPUT); //wheelspeed 1 output 1
  pinMode(WSS1PIN2, OUTPUT); //wheelspeed 1 output 2
  pinMode(WSS2PIN1, OUTPUT);//wheelspeed 2 output 1
  pinMode(WSS2PIN2, OUTPUT);//wheelspeed 2 output 2
  pinMode(14, OUTPUT);//VSS output
  pinMode(15, OUTPUT);//Output enable to MAX9921
  pinMode(16, OUTPUT);//Diag to MAX9921
  pinMode(17, INPUT); //ERR from MAX9921
  pinMode(18, INPUT); //Hall 1 signal from MAX9921
  pinMode(19, INPUT); //Hall 2 signal from MAX9921
  digitalWriteFast(13, LOW);
  attachInterrupt(digitalPinToInterrupt(14), input1ISR, RISING);
  delayflag1 = micros();
  periodflag1 = micros();
  signalsend = period - halfwave;
  signalend = period;
  signalstart = signalsend - halfwave;

}

void loop() {
  periodtimer1 = micros();

//  Write to the serial buffer when the ISR has been accessed
  if (inputfound) {
    //    Serial.println("ping");
    periodbuffer = microsISRbuffer - periodflag1;
    periodbuffer = periodbuffer * inputwheelteeth;
    periodbuffer = periodbuffer / outputwheelteeth;
    periodbuffer = periodbuffer*complement1;
    period = period*complement2;
    period = period+periodbuffer;
    period = period/100;
    periodflag1 = microsISRbuffer;
    signalsend = period - halfwave;
    signalend = period;
    signalstart = signalsend - halfwave;
    inputfound = false;
  }

  if (periodtimer1 - delayflag1 >= signalstart) {
    if (!signalstarted) {
      digitalWriteFast(WSS1PIN1, HIGH);
      digitalWriteFast(WSS1PIN2, HIGH);
      signalstarted = true;
    }
    if (!signalsent) {
      if (periodtimer1 - delayflag1 >= signalsend) {
        digitalWriteFast(WSS1PIN1, LOW);
        digitalWriteFast(WSS1PIN2, LOW);
        signalsent = true;
        signalended = false;
      }
    }
    if (!signalended) {
      if (periodtimer1 - delayflag1 >= signalend) {
        digitalWriteFast(WSS1PIN1, HIGH);
        digitalWriteFast(WSS1PIN2, LOW);
        delayflag1 = periodtimer1;
        signalended = true;
        signalstarted = false;
        signalsent = false;
      }
    }
    // put your main code here, to run repeatedly:

  }
//  delayMicroseconds(10);
}

void input1ISR() {
  microsISRbuffer = micros();
  inputfound = true; //set a flag to indicate that the ISR has been accessed
}
