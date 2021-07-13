

#include "timers.h"

// const uint8_t RS = 5;
// const uint8_t RW = 6;
// const uint8_t CLK = 7;
#define RS      A4
#define CLK     A5
enum
{
  LCD_CLEAR = 1,
  LCD_HOME  = 2,
  
};

//volatile static uint32_t f_count = 0;
static char buffer[16];   // for number conversion

void my_delay(uint16_t dly);

void setup() 
{
 // delay(1000);
  pinMode(13, OUTPUT);
  pinMode(RS, OUTPUT);
//  pinMode(RW, OUTPUT);
  pinMode(CLK, OUTPUT);
  digitalWrite(RS, HIGH);
//  digitalWrite(RW, LOW);
  digitalWrite(CLK, HIGH);
  //my_delay(500);
  DDRC = 0xff;   // data outputs
  PORTC = 0;
  PORTC = 3;
  //my_delay(200);
  delay(5);
  digitalWrite(CLK, HIGH);
  my_delay(1);
  digitalWrite(CLK, LOW);
  my_delay(20);
  digitalWrite(CLK, HIGH);
  my_delay(1);
  digitalWrite(CLK, LOW);
  my_delay(20);
  PORTC = 2;
  digitalWrite(CLK, HIGH);
  my_delay(1);
  digitalWrite(CLK, LOW);
  my_delay(20);
//
//  TCCR0A = 0;   // [ com0a1 com0a0 com0b1 com0b0 res res wgm1 wgm0]
//  TCCR0B = 7;   // [ foc0a foc0b r r wgm2 cs02 cs01 cs00 ]  external T0 rising
//  TCNT0 = 0;   // clear the counter
//  TIMSK0 |= 4;   // Timer int mask [r r r r r OCIE0B OCIE0A TOIE0
//  OCR0B = 0;   // output compare instead of overflow
//  

//TCCR2A = 0;
//TCCR2B = 7;
//TCNT2 = 0;
//TIMSK2 |= 4;
//OCR2B = 255;

}

void to_decimal(uint32_t n, char *buff)
{
  uint8_t digit = 0;
  for(digit = 0; digit < 10; digit++)
  {
    buff[9-digit] = n % 10 + '0';
    n /= 10;
    
  }
  buff[10] = 0;
}
void LCD_printBuffer(uint8_t *buff)
{
  uint8_t index = 0;
  while(buff[index] != 0)
  {
    LCD_writeData(buff[index]);
    ++index;
  }
}
ISR(TIMER0_COMPB_vect)
//ISR(TIMER0_OVF_vect)
{
//  f_count++;
}
//ISR(TIMER2_COMPB_vect)
//{
  
//}

volatile static uint16_t dummy;
void my_delay(uint16_t dly)
{
  //delay(dly);
  //return;

  
  while(dly > 0)
  {
    for(uint16_t c = 0; c<700; ++c)
    {
      dummy = c;
    }
    --dly;
  }
}




uint8_t to_hex(uint8_t b)
{
  uint8_t rtn = ' ';
  if(b < 16)
  {
    rtn = b+48;
    
  }
  if(b>9)
  {
    rtn+= 7;
  }
  return rtn;
}

void LCD_writeCommand(uint8_t cmd)
{

  digitalWrite(RS, LOW);
  PORTC &= 0xf0;
  PORTC |= cmd >> 4;
  digitalWrite(CLK, HIGH);
  //my_delay(2);
  delayMicroseconds(5);
  digitalWrite(CLK, LOW);
  //my_delay(2);
  delayMicroseconds(5);
  PORTC &= 0xf0;
  PORTC |= cmd & 0xf;
  digitalWrite(CLK, HIGH);
  //my_delay(2);
  delayMicroseconds(5);
  digitalWrite(CLK, LOW);
  //my_delay(2);
  delayMicroseconds(500);
}

void LCD_writeData(uint8_t dat)
{
  digitalWrite(RS, HIGH);
  PORTC &= 0xf0;
  PORTC |= dat>>4;
  digitalWrite(CLK, HIGH);
  //my_delay(2);
  delayMicroseconds(5);
  digitalWrite(CLK, LOW);
  //my_delay(2);
  delayMicroseconds(5);
  PORTC &= 0xf0;
  PORTC |= dat & 0xf;
  digitalWrite(CLK, HIGH);
  //my_delay(2);
  delayMicroseconds(5);
  digitalWrite(CLK, LOW);
  //my_delay(2);
  delayMicroseconds(500);
}



void loop() 
{
  uint8_t c = TCNT0;
  uint8_t c0 = (c & 0xf);
  uint8_t c1 = (c>>4);
  static float f1 = 0;
  float f2 = 0;
  
  digitalWrite(13, HIGH);
  //LCD_writeCommand(LCD_CLEAR); // clear display
  //my_delay(5);
  LCD_writeCommand(LCD_HOME);
  my_delay(2);
  my_delay(5);
  LCD_writeData(to_hex(c1));
  my_delay(5);
  LCD_writeData(to_hex(c0));
  my_delay(5);
  LCD_writeData(' ');
  my_delay(5);
//  noInterrupts();
//  f_count = 0;
//  TCNT0=0;
//  interrupts();
//  my_delay(195);
//  noInterrupts();
//  to_decimal((f_count << 8) + TCNT0 , buffer);
//  interrupts();

  to_decimal(Timers_readCount(), buffer);
  
  LCD_printBuffer(buffer);
  LCD_writeData('*');
//  to_decimal(millis(), buffer);
 // LCD_printBuffer(buffer);
 // LCD_writeData('D');
 /* 
  for(uint8_t ch = '0'; ch < 'c'; ++ch)
  {
    LCD_writeData(ch);
    my_delay(20);
  }
  */
  digitalWrite(13, LOW);
  my_delay(800);
}
