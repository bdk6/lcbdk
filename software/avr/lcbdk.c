
#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd_44780.h"

#define PI 3.14159265
#define READ_PER_SEC 5.0



// ///////////////////////////////////////////////////////////////////////////
// L measure button on PD7
// C measure button on PD6
// Zero button on PD5
// ///////////////////////////////////////////////////////////////////////////
typedef enum ButtonPress
{
  BTN_ALL     = 0,
  BTN_LC      = 1,
  BTN_LZ      = 2,
  BTN_L       = 3,
  BTN_CZ      = 4,
  BTN_C       = 5,
  BTN_Z       = 6,
  BTN_NONE    = 7
} ButtonPress_t;

typedef enum OpMode
  {
    MODE_MEASURE_n,
    MODE_MEASURE_u,
    MODE_MATCH_n,
    MODE_MATCH_u,
    MODE_MATCH_PERCENT
  } OpMode_t;

OpMode_t current_mode = MODE_MEASURE_n;

volatile static int t1_count = 0;
volatile static int t0_count = 0;

static float osc_l = 0.000068; // 68 uH
static float osc_c = 680E-12;

void init_buttons(void);
ButtonPress_t get_buttons(void);

int itos(int32_t i, char* s, int c);
float find_c(float f);
float find_l(float f);
int calibrate(float* l, float* c, const float* k);
uint32_t measure_frequency(void);


void timer0_init(void)
{
  // TCCR0
  // [ -|-|-|-|-|cs2|cs1|cs0]
  TCCR0 = 0;  // clock source none
  // TIMSK
  // [ocie2|toie2|tocie1|ocie1a|ocie1b|toie1| - |toie0 ]
  TIMSK |= (1 << 0);   // enable ovf interrupt
  TCNT0 = 0;
}

// Timer1 (16 bit) times the measure interval
// It interrupts on overflow
// prescale 1024: 16 Mhz / 1024 = 15625
// 15625 / 5 = 3125
// prescale 64: 16 Mhz / 64 = 250,000
// 250,000 / 5 = 50,000 for 1/5th sec
// Use normal mode and int on ovf
// Set timer to 15536 (65536-50000) and start
void timer1_init(void)
{
  //TCCR1A
  // [com1a1|com1a0|com1b1|com1b0|foc1a|foc1b|wgm11|wgm10]
  TCCR1A = 0;  
  //TCCR1B
  // [icnc1|ices1| - | wgm13|wgm12|cs12|cs11|cs10]
  TCCR1B = 0;  // clock source none

  // TIMSK
  // [ ocie2|toie2|ticie1|ocie1a|ocie1b|toie1| - |toie0 ]
  //              [       timer 1            ]
  TIMSK |= (1 << 2);  // enable ovf int
  TCNT1 = 0;
}

static uint16_t upper_freq = 0;
static float freq = 0.0;
static volatile ready_flag = 0;
//void start_measure(void)
//{
//  // clear variables
//  upper_freq = 0;
//  freq = 0;
//  
//  // clear timers
//  TCNT0 = 0;
//  TCNT1 = 15536;  // 65536 - 50000
//  
//  // start timers
//  TCCR1B  |= 3; // div 64 -- start
//  TCCR0 |= 6; // clock on rising edge of t0
//}

uint32_t measure_frequency(void)
{
  uint32_t rtn = 0;
  
  // clear variables
  upper_freq = 0;
  freq = 0;

  // clear timers
  TCNT0 = 0;
  TCNT1 = 15536;  // 65536 - 50000

  // start timers
  TCCR1B |= 3; // Set clock to div by 64 to start
  TCCR0 |= 6;  // External clock on rising edge

  while(!ready_flag);  // Wait for timeout
  ready_flag = 0;

  return freq;
}


//////////////////////////////////////////////////////////////////////////////
/// @fn TIMER1_OVF_vect ISR
/// @brief On timer 1 overflow, stop count, calc frequency, set ready flag
//////////////////////////////////////////////////////////////////////////////
ISR(TIMER1_OVF_vect)
{
  // stop timers
  TCCR0 &= ~(0x07);  // turn off clock to timer 0
  TCCR1A &= ~(0x07);  // turn off clock to timer 1
  // get count
  freq = READ_PER_SEC * (((uint32_t) upper_freq) << 8 ) + TCNT0;
  t1_count++;
  ready_flag = 1;
}


//////////////////////////////////////////////////////////////////////////////
/// @fn TIMER0_OVF_vect ISR
/// @brief On timer 0 overflow, increment upper bits of frequency count.
/////////////////////////////////////////////////////////////////////////////
ISR(TIMER0_OVF_vect)
{
  upper_freq++;
}


void operate(void)
{
  // wait for button
  // if button is ZERO
  //   next mode
  // else if button is L
  //   measure L
  // else is button is C
  //  measure C

  // Get buttons
  // if none, calibrate
  // else if L measure L
  // else if C measure C
  // else if Z change mode
  // else if L and Z Zero L
  // else if C and Z Zero C
  // 

}

  
///////////////////////////////////////////////////////////////
int main()
{
  DDRD |= 0x01;  // D0 has LED
  //PORTD |= 0x01; // turn it on

  // set up for buttons
  DDRD &= ~0xc0; // set d6,d7 as inputs;
  PORTD |= 0xc0;  // turn on pullups
  init_buttons();
  
  timer0_init();
  timer1_init();
  LCD_44780_init(16,2);
  LCD_44780_clear();
  // LCD_44780_write_data('H');
  // LCD_44780_write_data('i');
  LCD_44780_goto(10, 1);
  
  LCD_44780_write_string("LC BDK -- Mostly Analog Electronics (c) 2023 William Cooke");
  char st[10];
  //itos(1000, st, 5);
  //LCD_44780_write_string(st);
  _delay_ms(1000);
  //  int c = 0;
  PORTD &= 0xfe;  // turn off LED
  sei();
  //start_measure();
  while(1)
    {
      // LCD_44780_write_data( (uint8_t)(c & 0xff) );
      //start_measure();
      int tmp = t1_count;
      //while(!ready_flag);
      //ready_flag = 0;
      uint32_t f = measure_frequency();
      
      if(t1_count & 1)
      {
       PORTD |= 1;
      }
      else
      {
       PORTD &= 0xfe;
      }
      LCD_44780_clear();
      itos((int32_t)f /*freq*/, st, 7);
      LCD_44780_write_string(st);

      float c = find_c(f); //freq);
      uint32_t ci = (int)(c * 1E13);
      
      itos(ci, st, 6);
      LCD_44780_goto(1, 4);
      LCD_44780_write_string(st);
      int btns = get_buttons(); //(PIND >> 6)  & 0x03;
      itos(btns, st, 3);
      LCD_44780_write_string(st);
      
      // c++;
    }
  return 0;
}


int itos(int32_t i, char* s, int c)
{
  uint8_t neg = 0;
  if (i < 0)
    {
      neg = 1;
      i = -i;
    }
  s[--c] = 0;
  for(int j = 0; j <c; j++)
    {
      s[j] = ' ';
    }
  while(i > 0 && c > 0)
    {
      c--;
      int ch = i % 10;
      s[c] = ch + '0';
      i /= 10;
    }

  return 0;
}



//#define pi  3.1415926538

float find_c(float f)
{
  float rtn = 0.0;
  float w = 2 * PI * f;
  float lc = (1/w) * (1/w);
  rtn = lc / osc_l; 
  return rtn;
}

float find_l(float f)
{
  float rtn = 0.0;
  float w = 2 * PI * f;
  float lc = (1/w) * (1/w);
  rtn = lc / osc_c;
  return rtn;
}



// ///////////////////////////////////////////////////////////////////////////
// Calibration
// l == osc inductance (68 uH)
// c == osc capacitance (680 pF)
// k == calibration capacitance (1000 pF)
//
// f1 = 1/(2 pi sqrt(lc))
// sqrt(lc) = 1/(2 pi f1)
// lc = (1/(2 pi f1))^2;
//
// f2 = 1/(2 pi sqrt(l(c+k))
// sqrt( l(c+k) ) = 1/(2 pi f2)
// l(c+k) = (1/(2 pi f2))^2
// lc + lk = (1/(2 pi f2))^2
// sub for lc
// (1/(2 pi f1))^2 + lk = (1/(2 pi f2))^2
// lk = (1/(2 pi f2))^2 - (1/(2 pi f1))^2
// new_l = ( (1/(2 pi f2))^2 - (1/(2 pi f1))^2 ) / k
// new_c = (1/(2 pi f1))^2/ new_l
//
// To calibrate
// Measure f1 with no added l or c
// Switch in calib cap
// Measure f2
// Find l from above
// using f1 and l, find c
// store l and c

int calibrate(float* l, float* c, const float* k)
{
  float f1;  // freq without calib c
  float f2;  // freq with calib c
  float w1;  // 2 pi f1
  float w2;  // 2 pi f2
  float lc;
  float new_l;
  float new_c;
  int rtn = 0;
  // measure f1
  w1 = 2 * PI * f1;
  lc = 1 / (w1 * w1);
  // enable calib c
  // measure f2
  new_l = ( 1/(w2 * w2) - 1/(w1 * w1) ) / *k;
  new_c = (1/ (w1 * w1)) / new_l;

  // make sure it makes sense, then...
  *l = new_l;
  *c = new_c;
  rtn = 1;
  return rtn;
}


void init_buttons(void)
{
  DDRD &= ~(0xe0); // 7,6,5 as inputs
  PORTD |= 0xe0;   // turn on pullups
}

ButtonPress_t get_buttons(void)
{
  uint8_t pins = (PIND >> 5) & 0x07;
  return (ButtonPress_t) pins;
}

  

  
  
