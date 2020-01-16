#include "arduinoFFT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SAMPLES 128             //Must be a power of 2
#define SAMPLING_FREQUENCY 20000

#define SCREEN_WIDTH 96 // OLED display width, in pixels
#define SCREEN_HEIGHT 16 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
 
arduinoFFT FFT = arduinoFFT();
 
volatile double vReal[SAMPLES];
volatile double vImag[SAMPLES];

volatile int num = 0;

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);



  
  // ADEN ADSC ADATE ADIF ADIE ADPS ADPS ADPS
  // ADC enable, ADC start conversion, ADC Auto Trigger, ADC Interrupt, ADC Interrupt Enable, ADC Prescaler
  ADCSRA = B11101101;
  
  // RESERV ACME RESERV RESERV RESERV ADTS
  // Analog Comparator Multiplexer Enable, ADC Auto Trigger Source
  ADCSRB = B00000000; //Turn off ACME, set ADTS to Freerunning
  
  // REFS REFS ADLAR RESERV MUX
  // Voltage Reference Selection, ADC Left Adjust Result, Analog Channel Select
  ADMUX =  B01100000;
  
  //RESRV RESERV ADCND 5..0
  // ADC Pin N Digital Input Disable
  DIDR0 =  B00000001;
  
  //What are the implications of this
  TIMSK0 = 0;
  
  //The First ADC Conversion takes 25 cycles instead of 13, do we need to care
  interrupts();
}
void do_fft() {
  
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  for(int i=0; i<(SAMPLES/2); i++)
  {   
      Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
      Serial.print(" ");
      Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }

}

ISR(ADC_vect) {
  //we only need 8-bit resolution (p247) so we skip ADCL. ADLAR NEEDS TO BE 1
  vReal[num] = ADCH;
  vImag[num++] = 0;
}

double find_max() {
  double out = 0;
  for(int i = 0; i < SAMPLES; i++) {
    if(vReal[i] > out) {
      out = vReal[i];
    }
  }
  return out;
}

void output() {
  display.clearDisplay();
  double m = find_max();
  int p = m / SCREEN_HEIGHT;
  for(int i = 0; i < SCREEN_WIDTH; i++) {
    for(int n = 0; n < SCREEN_HEIGHT; n++) {
      if(vReal[i] > p * n) {
        display.drawPixel(i, n, WHITE);
      }
    }
  }
  display.display();
}

void loop() {
  if(num >= SAMPLES) {
    ADCSRA &= ~(1 << ADIE); //turn off ADC Interrupt so samples isnt overwritten
    do_fft();
    num = 0;
    output();
    ADCSRA |= (1 << ADIE);
  }
  
}
