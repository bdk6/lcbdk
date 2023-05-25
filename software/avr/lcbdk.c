
#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd_44780.h"

#define VERSION_MAJOR      1
#define VERSION_MINOR      1
#define VERSION_BUILD      0

#define PI 3.14159265
#define READ_PER_SEC 5.0



// ///////////////////////////////////////////////////////////////////////////
// L measure button on PD7
// C measure button on PD6
// Zero button on PD5
// ///////////////////////////////////////////////////////////////////////////
typedef enum ButtonPress
{
  BTN_ALL     = 7,
  BTN_LC      = 6,
  BTN_CZ      = 5,
  BTN_C       = 4,
  BTN_LZ      = 3,
  BTN_L       = 2,
  BTN_Z       = 1,
  BTN_NONE    = 0
} ButtonPress_t;

typedef enum OpMode
  {
    MODE_MEASURE_NANO,
    MODE_MEASURE_MICRO,
    MODE_MATCH_NANO,
    MODE_MATCH_MICRO,
    MODE_MATCH_PERCENT,
    MODE_CALIBRATE,
    MODE_LIST_END
  } OpMode_t;

OpMode_t current_mode = MODE_MEASURE_NANO;

volatile static int t1_count = 0;
volatile static int t0_count = 0;

static float osc_l = 68E-6;
static float osc_c = 680E-12;
static float stray_l = 0.0;
static float stray_c = 0.0;
static float match_l = 0.0;
static float match_c = 0.0;


void init_buttons(void);
ButtonPress_t get_buttons(void);
int itos(int32_t i, char* s, int c);
float find_c(float f);
float find_l(float f);
int calibrate(float* l, float* c, const float* k);
uint32_t measure_frequency(void);
void display_c(float c);
void display_l(float l);
void display_mode( OpMode_t mode);
void print_int(int32_t num, int digits, int dp, int sign);

//////////////////////////////////////////////////////////
/// @fn timer0_init
/// @brief set timer 0 to interrupt on overflow
/// @remark It's clock will be the output of oscillator
//////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////
///  @fn timer1_init
///  @brief Sets up timer to measure count interval
/////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////
/// @var upper_freq
/// @brief Holds upper two bytes of oscillator count
/////////////////////////////////////////////////////////
static uint16_t upper_freq = 0;

/////////////////////////////////////////////////////////
/// @var freq
/// @brief Holds calculated frequency
static float freq = 0.0;

/////////////////////////////////////////////////////////
/// @var ready_flag
/// @brief Indicates when measurement is complete
/////////////////////////////////////////////////////////
static volatile uint8_t ready_flag = 0;


///////////////////////////////////////////////////////
/// @fn measure_frequency
/// @brief 
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
  TCCR1B |= 3; // Set clock to div by 64 (250KHz ) to start it
  TCCR0 |= 6;  // External clock on rising edge

  while(!ready_flag);  // Wait for timeout
  ready_flag = 0;
  // Stop timers
  TCCR1B &= ~7;
  TCCR0 &= ~7;

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

  
///////////////////////////////////////////////////////////////
int main()
{
  //DDRD |= 0x01;  // D0 has LED
  //PORTD |= 0x01; // turn it on

  // set up for calibrate relay PD3
  PORTD |= (1<<3);  // turn it off 1=off, 0=on
  DDRD  |= (1<<3);  // make it an output
 
  // set up for buttons
  DDRD &= ~0xc0; // set d6,d7 as inputs;
  PORTD |= 0xc0;  // turn on pullups
  init_buttons();
  
  timer0_init();
  timer1_init();
  LCD_44780_init(16,2);
  LCD_44780_clear();
  
  LCD_44780_write_string("LC BDK (c) 2023 William Cooke");
  
  char st[10];
  LCD_44780_goto(0,1);
  LCD_44780_write_string("Wait--warming up");
  _delay_ms(1000);

  sei();   // Enable interrupts before measuring
  
  LCD_44780_goto(0,1);
  LCD_44780_write_string("Wait--calibrating");
  
  // calibrate
  float l,c,k;
  k = 1000E-12;
  calibrate(&l, &c, &k);
  osc_l = l;
  osc_c = c;
  
  LCD_44780_goto(0,1);
  display_c(c);
  LCD_44780_goto(8,1);
  display_l(l);
  _delay_ms(1000);
  LCD_44780_clear();
  LCD_44780_goto(0,0);
  display_mode(current_mode);
  
  // gone!  PORTD &= 0xfe;  // turn off LED

  while(1)
    {      
      ButtonPress_t  btns = get_buttons();
      //LCD_44780_goto(8,0);
      //print_int(btns, 2,0,0 );  // num, digits, dp, sign
      float f1 = 0.0;
      float calc_l = 0.0;
      float calc_c = 0.0;
      uint32_t fi = 0;
      if(btns == BTN_Z)
	{
	  current_mode++;
	  if(current_mode == MODE_LIST_END)
	    {
	      current_mode = MODE_MEASURE_NANO;
	    }
	  LCD_44780_goto(0,0);
	  display_mode(current_mode);
	  while(get_buttons() == BTN_Z); // wait for release
	  switch(current_mode)
	    {
	    case MODE_MEASURE_NANO:
	    case MODE_MEASURE_MICRO:
	      break;
	    case MODE_MATCH_NANO:
	    case MODE_MATCH_MICRO:
	    case MODE_MATCH_PERCENT:
	      match_l = 0.0;
	      match_c = 0.0;
	      break;
	    case MODE_CALIBRATE:
	      break;
	    default:
	      break;
	    }
	}

      // Get rid of "stray" on display if there.
      LCD_44780_goto(8,1);
      LCD_44780_write_string("        ");
      LCD_44780_goto(8,1);
      print_int(btns, 3, 0, 0);
      
      switch(current_mode)
	{
	case MODE_MEASURE_NANO:
	case MODE_MEASURE_MICRO:
	  switch(btns)
	    {
	    case BTN_L:
	      // measure freq
	      fi = measure_frequency();
	      // make sure it is valid
	      btns = get_buttons();
	      if(btns == BTN_L && fi != 0)
              {	
	        LCD_44780_goto(8,0);
	        print_int(fi, 6,0,0);
	        f1 = (float)fi;
	        // calc l
	        calc_l = find_l(f1);
	        // display l
	        LCD_44780_goto(0,1);
	        display_l(calc_l - osc_l - stray_l);
              }
	      break;
	    case BTN_C:
	      // measure freq
	      fi = measure_frequency();
	      // make sure it is valid
	      btns = get_buttons();
	      if(btns == BTN_C && fi != 0)
	      {
	        LCD_44780_goto(8,0);
	        print_int(fi, 6, 0, 0);
	        f1 = (float)fi;
	        // calc c
	        calc_c = find_c(f1);
  	        // display c
	        LCD_44780_goto(0,1);
	        display_c(calc_c - osc_c - stray_c);
	      }
	      break;
	    case BTN_LZ:
	      // measure freq
	      fi = measure_frequency();
	      // make sure its valid
	      btns = get_buttons();
	      if( (btns == BTN_LZ || btns == BTN_L) && fi != 0)
		{
		  LCD_44780_goto(8,0);
		  print_int(fi, 6, 0, 0);
		  f1 = (float)fi;
		  // calc l
		  calc_l = find_l(f1);
	          // store stray
		  LCD_44780_goto(0,1);
		  display_l(calc_l - osc_l);
		  stray_l = calc_l - osc_l;
		  LCD_44780_goto(8,1);
		  LCD_44780_write_string("Stray");
		}
	      break;
	    case BTN_CZ:
	      // measure freq
	      fi = measure_frequency();
	      // make sure its valid
	      btns = get_buttons();
	      LCD_44780_goto(8,1);
	      print_int(btns, 3, 0, 0);
	      LCD_44780_write_string("CZ");
	      
	      if( (btns == BTN_CZ || btns == BTN_C) && fi != 0)
		{
		  LCD_44780_goto(8,0);
		  print_int(fi, 6, 0, 0);
		  f1 = (float)fi;
	      // calc c
		  calc_c = find_c(f1);
	      // store stray
		  LCD_44780_goto(0,1);
		  LCD_44780_write_string("zero c");
		  //display_c(calc_c - osc_c);
		  stray_c = calc_c - osc_c;
		  LCD_44780_goto(10,1);
		  LCD_44780_write_string("Stray");
		  _delay_ms(300);
		}
	      break;
	    case BTN_Z:
	      // already handled
	      break;
	    case BTN_LC:  // not valid
	      break;
	    case BTN_ALL:  // not valid
	      break;
	    default:
	      break;
	    }
            break;
	case MODE_MATCH_NANO:
	case MODE_MATCH_MICRO:
	case MODE_MATCH_PERCENT:
	  switch(btns)
	    {
	    case BTN_L:
	      // measure frequency
	      fi = measure_frequency();
	      // 
	      btns = get_buttons();
	      LCD_44780_goto(8,0);
	      print_int(fi, 6, 0, 0);
	      f1 = (float)fi;
	      calc_l = find_l(f1);
	      
	      if( btns == BTN_L )
		{
		  if(match_l != 0.0)
		    {
		      // Show match
		      float diff = calc_l - osc_l - stray_l - match_l;
		      display_l(diff);
		      
		    }
		}
	      
	      if( btns == BTN_LZ )
		{
		  match_l = calc_l - osc_l - stray_l;
		}
	      

	      break;
	    case BTN_C:
	      break;
	    }
	  break;
	 
	case MODE_CALIBRATE:
	  if( btns == BTN_Z )
	    {
	      // calibrate
	    }
	  break;
	  
	default:
	  break;
	}
    }
  return 0;
}


//////////////////////////////////////////////////
/// @fn itos
/// @brief convert an int to a string
/// @param[in] i The integer to convert
/// @param[out] s Pointer to receiving string
/// @param[in] c Length of output string
/// @return
/////////////////////////////////////////////////
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




///////////////////////////////////////////////
/// @fn find_c
/// @brief Calculate capacitance from measured freq
/// @param[in] f The measured frequency.
/// @return Calculated capacitance.
///////////////////////////////////////////////
float find_c(float f)
{
  float rtn = 0.0;
  float w = 2 * PI * f;
  float lc = (1/w) * (1/w);
  rtn = lc / osc_l; 
  return rtn;
}

/////////////////////////////////////////////////
/// @fn find_l
/// @brief Calculate inductance from measured freq
/// @param[in] f The measured frequency.
/// @return Calculated inductance
////////////////////////////////////////////////
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
// l == osc inductance (68 uH nominal)
// c == osc capacitance (680 pF nominal)
// k == calibration capacitance (1000 pF assumed accurate)
//
// f1 is freq without calibration cap
// f1 = 1/(2 pi sqrt(lc))
// sqrt(lc) = 1/(2 pi f1)
// lc = (1/(2 pi f1))^2;
//
// f2 is freq with calibration cap
// f2 = 1/(2 pi sqrt(l(c+k))
// sqrt( l(c+k) ) = 1/(2 pi f2)
// l(c+k) = (1/(2 pi f2))^2
// lc + lk = (1/(2 pi f2))^2
// sub for lc from f1 above
// (1/(2 pi f1))^2 + lk = (1/(2 pi f2))^2
// lk = (1/(2 pi f2))^2 - (1/(2 pi f1))^2
// new_l = ( (1/(2 pi f2))^2 - (1/(2 pi f1))^2 ) / k
// new_c = (1/(2 pi f1))^2/ new_l
//
// To calibrate
// Measure f1 with no added l or c
// Switch in calibration cap
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

  int32_t tmp;
  char st[10];
  LCD_44780_clear();
  
  // measure f1
  tmp = measure_frequency();
  itos(tmp, st, 7);
  LCD_44780_clear();
  LCD_44780_goto(0,0);
  LCD_44780_write_string(st);
  LCD_44780_write_string(":");

  f1 = (float) tmp;
  w1 = 2 * PI * f1;
  lc = 1 / (w1 * w1);
  
  // enable calibration cap
  PORTD &= ~(1<<3);  // turn on relay
  _delay_ms(100); // Let relay and osc stabilize
  
  // measure f2
  tmp = measure_frequency();
  itos(tmp + 10, st, 7);
   LCD_44780_goto(0,1);
  LCD_44780_write_string(st);
  
  f2 = (float) tmp;

  // disable calibration cap
  PORTD |= (1<<3);  // turn off relay
  
  w2 = 2 * PI * f2;
  new_l = ( 1/(w2 * w2) - 1/(w1 * w1) ) / *k;
  new_c = (1/ (w1 * w1)) / new_l;
  
  //_delay_ms(4000);

  // make sure it makes sense, then...
  *l = new_l;
  *c = new_c;
  
  LCD_44780_goto(0,1);
  display_c(new_c);
  LCD_44780_goto(8,1);
  display_l(new_l);
  //_delay_ms(5000);

  rtn = 1;
  //_delay_ms(10000);
  return rtn;
}


//////////////////////////////////////////////////////////////////////
/// @fn init_buttons
/// @brief Set up I/O to read buttons
//////////////////////////////////////////////////////////////////////
void init_buttons(void)
{
  DDRD &= ~(0xe0); // 7,6,5 as inputs
  PORTD |= 0xe0;   // turn on pullups
}

/////////////////////////////////////////////////////////////////////
/// @fn get_buttons
/// @brief Debounce and read all buttons
/// @return ButtonPress_t value indicating all buttons pressed
////////////////////////////////////////////////////////////////////
ButtonPress_t get_buttons(void)
{
  // Buttons on D7(L), D6(C), D5(ZERO)
  // Low when pressed so invert reading
  // AND together 5 readings at 2 ms intervals
  uint8_t pins = 0x07;
  for(int i = 0; i < 5; i++)
    {
      pins &= ( ~(PIND >> 5) & 0x07);
      _delay_ms(2);
    }
  return (ButtonPress_t) pins;
}

#define MAX_CHARS 12  // ten digits plus sign, dec pt
// dp of 0 won't put it in
//////////////////////////////////////////////////////////////////
/// @fn print_int
/// @brief Prints decimal int up to ten digits plus sign and DP.
/// @param[in] num  The integer to convert.
/// @param[in] digits Number of digits to use, right justified.
/// @param[in] dp Number of places from right to put decimal point.
/// @param[in] sign Zero for no sign, non-zero for sign.
//////////////////////////////////////////////////////////////////
void print_int(int32_t num, int digits, int dp, int sign)
{
  int s = 0;
  if(num < 0 & sign != 0)
  {
    s = 1;
    num = -num;
  }
  uint8_t buff[MAX_CHARS];
  int next = 0;  // count of characters
  while(next < MAX_CHARS -1 ) // leave room for sign
  {
    buff[next] = num %10 + '0';
    num /= 10;
    next++;
    if(next == dp)
    {
       buff[next] = '.';
       next++;
    }
  }
  // TODO add sign code

  // Add one to digit count if dp displayed
  if(dp != 0)
    {
      digits++;
    }
  while(digits > 0)
    {
      digits--;
      LCD_44780_write_data(buff[digits]);
      //  digits--;
    }
}

      
      
		      
		      
///////////////////////////////////////////////////////////////////
/// @fn display_c
/// @brief Displays capacitance value
/// @param[in] c Capacitance value in Farads.
//////////////////////////////////////////////////////////////////
void display_c(float c)
{
  int scale = 0;
  // Convert the float value of c to an int in tenths of pf
  // Then scale it to be less than 10,000
  // The display will go like this:
  // 999.9 pf   scale 0
  // 9.999 nF   scale 1
  // 99.99 nF         2
  // 999.9 nF         3
  // 9.999 uF         4
  // 99.99 uF         5
  uint32_t c_scaled = (uint32_t) (c * 1E13 + 0.5); // tenths of pf
  //char st[12];
  //itos(c_scaled, st, 9);
  //LCD_44780_clear();
  //LCD_44780_goto(0,0);
  //LCD_44780_write_string(st);
  //LCD_44780_write_string(" C");
  //_delay_ms(3000);
  
  while(c_scaled > 9999)
    {
      c_scaled /= 10;
      scale++;
    }
  int dp = 0;
  // scale of 0,3, or 6 has one decimal place
  // scale of 1, 4, or 7 has three decimal places
  // scale of 2, 5, or 8 has two decimal places
  if(scale == 0 || scale == 3 || scale == 6)
    {
      dp = 1;
      print_int(c_scaled, 4, 1, 0);
    }
  else if(scale == 2 || scale == 5 || scale == 8)
    {
      dp = 2;
      print_int(c_scaled, 4, 2, 0);
    }
  else if(scale == 1 || scale == 4 || scale == 7)
    {
      dp = 3;
      print_int(c_scaled, 4, 3, 0);
    }
  // print_int(c_scaled, 4,dp,0);
  switch (scale)
    {
    case 0: LCD_44780_write_string(" pF");
      break;
    case 1:
    case 2:
    case 3: LCD_44780_write_string(" nF");
      break;
    default: LCD_44780_write_string(" uF");
    }
  
}

///////////////////////////////////////////////////////////////
/// @fn display_l
/// @brief Displays Inductance value.
/// @param[in] l Inductance in Henries.
///////////////////////////////////////////////////////////////
void display_l(float l)
{
  int scale = 0;
    // Convert the float value of l to an int in nH
  // Then scale it to be less than 10,000
  // The display will go like this:
  // 9.999 uH         0
  // 99.99 uH         1
  // 999.9 uH         2
  // 9.999 mH         3
  // 99.99 mH         4
  // 999.9 mH         5
  // 9.999 H          6
  
  uint32_t l_scaled = (uint32_t) (l * 1E9 + 0.5); // nanohenries
  while(l_scaled > 9999)
    {
      l_scaled /= 10;
      scale++;
    }
  switch(scale)
    {
    case 2:
    case 5:
      print_int(l_scaled, 4, 1, 0);
      break;
    case 1:
    case 4:
      print_int(l_scaled, 4, 2, 0);
      break;
    case 0:
    case 3:
      print_int(l_scaled, 4, 3, 0);
      break;
    }
  switch(scale)
    {
    case 0:
    case 1:
    case 2:
      LCD_44780_write_string(" uH");
      break;
    case 3:
    case 4:
    case 5:
      LCD_44780_write_string(" mH");
      break;
    case 6:
      LCD_44780_write_string(" H");
      break;
    default:
      break;
    }
}

////////////////////////////////////////////////////////
///  @fn display_mode
///  @brief Writes mode name to top line of display
////////////////////////////////////////////////////////
void display_mode(OpMode_t mode)
{
  LCD_44780_goto(0,0);
  switch(mode)
    {
    case MODE_MEASURE_NANO:
      LCD_44780_write_string("Measure NANO    ");
      break;
    case MODE_MEASURE_MICRO:
      LCD_44780_write_string("Measure u       ");
      break;
    case MODE_MATCH_NANO:
      LCD_44780_write_string("Match n         ");
      break;
    case MODE_MATCH_MICRO:
      LCD_44780_write_string("Match u         ");
      break;
    case MODE_MATCH_PERCENT:
      LCD_44780_write_string("Match Percent   ");
      break;
    case MODE_CALIBRATE:
      LCD_44780_write_string("Calibrate       ");
      break;
    default:
      LCD_44780_write_string("MODE ERROR!     ");
    }
  return;
}
