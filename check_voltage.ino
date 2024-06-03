
/*

EJ05 oscilloscope project.
This sketch serves to measure the voltage on both channels and mathematically convert it
back to the original input voltage by undoing the gain and offset imparted by the input circuitry.

*/


#include "SPI.h"
#include <ADC.h>
#include <AnalogBufferDMA.h>
#include <DMAChannel.h>
#include <Encoder.h>

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define COLUMNS 15
#define ROWS 11

long vScale = 250;
long hScale = 40;
long prev_h;
long prev_v;

// Variables to store the last time the button state changed
unsigned long buttonPressStartTime = 0;
unsigned long buttonReleaseStartTime = 0;

// Time thresholds in milliseconds
const unsigned long pressThreshold = 1000; // Time the button must be held to edit vScale
const unsigned long releaseThreshold = 1000; // Time the button must be released to edit hScale

// Variable to track the current state of the button
bool buttonWasPressed = false;

#if defined(ADC_USE_DMA) && defined(ADC_USE_TIMER)

const int readPin_adc_0 = 14;
const int readPin_adc_1 = 15;

ADC *adc = new ADC();

const uint32_t buffer_size = 1600;
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff1[buffer_size];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2[buffer_size];
AnalogBufferDMA abdma1(dma_adc_buff1, buffer_size, dma_adc_buff2, buffer_size);

DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2_1[buffer_size];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2_2[buffer_size];
AnalogBufferDMA abdma2(dma_adc_buff2_1, buffer_size, dma_adc_buff2_2, buffer_size);

elapsedMillis elapsed_sinc_last_display;


Encoder trigger_Enc(21, 22);
int triggerIndex = 0;
Encoder scale_Enc(19, 20);
#define scale_select 18

int h_scale_min = 1;
int h_scale_max = 200;

double trigger;
bool trig_flag = false;

unsigned long int last = 0;
bool first_print = true;
volatile uint16_t *prev_data;


void setup() {
  Serial.begin(1000000);
  while (!Serial && millis() < 5000)
    ;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(readPin_adc_0, INPUT_DISABLE);
  pinMode(readPin_adc_1, INPUT_DISABLE);

  adc->adc0->setAveraging(1);
  adc->adc0->setResolution(10);
  adc->adc1->setAveraging(1);
  adc->adc1->setResolution(10);

  abdma1.init(adc, ADC_0);
  abdma1.userData(0);
  abdma2.init(adc, ADC_1);
  abdma2.userData(0);

  adc->adc0->startSingleRead(readPin_adc_0);
  adc->adc0->startTimer(500000);
  adc->adc1->startSingleRead(readPin_adc_1);
  adc->adc1->startTimer(500000);

}

void loop() {

  unsigned long int interval = 1;


 
  if (abdma1.interrupted() && abdma2.interrupted()) {
    ProcessAnalogData(&abdma1, 0);
    ProcessAnalogData(&abdma2, 1);
    elapsed_sinc_last_display = 0;
  }
  if (abdma1.interrupted()) {
    ProcessAnalogData(&abdma1, 0);
    elapsed_sinc_last_display = 0;
  }
  if (abdma2.interrupted()) {
    ProcessAnalogData(&abdma2, 1);
    elapsed_sinc_last_display = 0;
  }
}

void ProcessAnalogData(AnalogBufferDMA *pabdma, int8_t adc_num1) {




  volatile uint16_t *pbuffer = pabdma->bufferLastISRFilled();
  volatile uint16_t *end_pbuffer = pbuffer + pabdma->bufferCountLastISRFilled();

  if ((uint32_t)pbuffer >= 0x20200000u) {
    arm_dcache_delete((void *)pbuffer, sizeof(dma_adc_buff1));
  }


  int16_t x = 0;


  for (volatile uint16_t *i = pbuffer; i < pbuffer + 160; i++) {
    //Serial.println(*i); // Dereference the pointer to get the value
    float y1 = ((((*i*3.3)/1024)/1.65));
    float y2 = (y1-1.17)*20*(-1);
    Serial.println(y2);
    x++;
  }
  pabdma->clearInterrupt();

  delay(200);

}




#else
void setup() {}
void loop() {}
#endif





