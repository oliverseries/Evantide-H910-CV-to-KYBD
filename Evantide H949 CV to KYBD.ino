const long NORMALIZATION_FACTOR = 1000;

const long evantideHarmonicFreqs[37] = {625000, 662164.4, 701538.8, 743254.5, 787450.7, 834274.9, 883883.5, 936441.9, 992125.7, 1051120.5, 1113623.4, 1179842.9, 1250000, 1324328.9, 1403077.6, 1486508.9, 1574901.3, 1668549.8, 1767766.9, 1872883.8, 1984251.3, 2102241.0, 2227246.8, 2359685.8, 2500000, 2648657.7, 2806155.1, 2973017.8, 3149802.6, 3337099.6, 3535533.9, 3745767.7, 3968502.6, 4204482.1, 4454493.6, 4719371.6, 5000000};

long CV;

void setup() {
  Serial.begin(9600);
  pinMode(9, OUTPUT);
}

void loop() {
  CV = analogRead(A0);
  if (CV < 204.8) {
    CV = 204.8;  
  }
  if (CV > 819.2) {
    CV = 819.2;  
  }
  int CVArrayIndex = map(CV, 204.8, 819.2, 0, 36);
  setPWM(evantideHarmonicFreqs[CVArrayIndex], 500);
}

/**
 * Chooses the smallest prescaler for a given frequency. This way maximum duty
 * cycle resolution is achieved.
 */
int getPrescalerForFrequency(long frequency) {
//  tc = base frequency / target frequency / prescaler / 2
// 65535 = 8M / (prescaler * frequency)
// prescaler = 8M / (65535 * frequency)
  float prescaler = (8000000.0 / 65535.0) /
                              (1.0 * frequency / NORMALIZATION_FACTOR);
  if(prescaler <= 1)
    return 1;
  else if(prescaler <= 8)
    return 8;
  else if(prescaler <= 64)
    return 64;
  else if(prescaler <= 256)
    return 256;
  else if(prescaler <= 1024)
    return 1024;
  else // effectively stop the timer
    return 0;
}

/**
 * Returns an integer that holds the proper bits set for the given prescaler.
 * This value should be set to the TCCR1B register.
 * 
 * Atmel Atmega 328 Datasheet:
 * Table 16-5
 */
int preparePrescaler(int prescaler) {
  switch(prescaler){
    case 1: return _BV(CS10);
    case 8: return _BV(CS11);
    case 64: return _BV(CS10) | _BV(CS11);
    case 256: return _BV(CS12);
    case 1024: return _BV(CS12) | _BV(CS10);
    // effectively stops the timer. This should show the user that wrong input
  // was provided.
    default: return 0;
  }
}

/**
 * This sets a phase and frequency correct PWM mode.
 * Counting up from 0 to ICR1 (inclusive).
 * 
 * Atmel Atmega 328 Datasheet:
 * Table 16-4
 */
inline int prepareWaveGenMode() {
  return _BV(WGM13);
}

/**
 * Clear OC1A/OC1B on Compare Match when upcounting. Set OC1A/OC1B on Compare
 * Match when downcounting.
 * 
 * Atmel Atmega 328 Datasheet:
 * Table 16-3
 */
inline int prepareNormalCompareOutputMode() {
  return _BV(COM1A1) | _BV(COM1B1);
}

/**
 * Sets the Timer/Counter1 Control Register A
 * 
 * Atmel Atmega 328 Datasheet:
 * Section 16.11.1
 */
void setTCCR1A() {
  TCCR1A = prepareNormalCompareOutputMode();
}

/**
 * Sets the Timer/Counter1 Control Register B
 * 
 * Atmel Atmega 328 Datasheet:
 * Section 16.11.2
 */
void setTCCR1B(int prescaler) {
  TCCR1B = prepareWaveGenMode() | preparePrescaler(prescaler);
}


/**
 * Note that ICR1 will never be less than a thousand. This way better
 * duty cycle resolution can be achieved. In fact, below 800 Hz ICR1
 * will occasionally drop under 10000
 */
void setPWM(long frequency, long duty) {

  int prescaler = getPrescalerForFrequency(frequency);
  if(prescaler == 0) return;
  
  setTCCR1A();
  setTCCR1B(prescaler);
  long top = (long)(8000000.0 /
                (1.0 * prescaler * frequency / NORMALIZATION_FACTOR) + 0.5);
  ICR1 = top;
  OCR1A = (long)(1.0 * top * duty / NORMALIZATION_FACTOR + 0.5);
}
