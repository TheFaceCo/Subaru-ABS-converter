unsigned long period = 1000;
unsigned long halfwave = 200;
unsigned long periodflag1;
unsigned long delayflag1;
unsigned long signalstart;
unsigned long signalsend;
unsigned long signalend;
unsigned long periodtimer1;


bool signalstarted = false;
bool signalsent = false;
bool signalended = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);//output enable to MAX9921
  pinMode(14, INPUT); //Hall 1 signal from MAX9921
  pinMode(15, INPUT); //Hall 2
  digitalWriteFast(13, LOW);
  attachInterrupt(digitalPinToInterrupt(14), input1ISR, RISING);
  delayflag1 = micros();
  periodflag1 = micros();
  signalsend = period - halfwave;
  signalend = period;
  signalstart = signalsend - halfwave;
  
}

void loop() {
//  periodtimer1 = micros();
//  
//  if (periodtimer1 - delayflag1 >= signalstart) {
//    if (!signalstarted) {
//      digitalWriteFast(11, HIGH);
//      digitalWriteFast(12, HIGH);
//      signalstarted = true;
//    }
//    if (!signalsent) {
//      if (periodtimer1 - delayflag1 >= signalsend) {
//        digitalWriteFast(11, LOW);
//        digitalWriteFast(12, LOW);
//        signalsent = true;
//        signalended = false;
//      }
//    }
//    if (!signalended) {
//      if (periodtimer1 - delayflag1 >= signalend) {
        digitalWriteFast(11, HIGH);
        digitalWriteFast(12, HIGH);
//        delayflag1 = periodtimer1;
//        signalended = true;
//        signalstarted = false;
//        signalsent = false;
//      }
//    }
//    // put your main code here, to run repeatedly:
//
//  }
}

void input1ISR() {
  period = micros() - periodflag1;
  periodflag1 = micros();
  signalsend = period - halfwave;
  signalend = period;
  signalstart = signalsend - halfwave;
}
