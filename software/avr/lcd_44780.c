//////////////////////////////////////////////////////////////////////////////
///  @file lcd_44780.c
///  copyright(c) 2023,2014 William R Cooke
///  @brief Interfaces AVR to character LCDs using HD44780 or compatible
//////////////////////////////////////////////////////////////////////////////

#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "lcd_44780.h"


#define CMD 0   // Select CMD reg
#define DAT 1   // Select DATA reg
#define RD  1   // Select READ mode
#define WR  0   // Select WRITE mode

static uint8_t columns = 8;
static uint8_t rows = 1;


// RS -> PB4
// RW -> PC0
// E  -> PB5
// D4 -> PB0
// D5 -> PB1
// D6 -> PB2
// D7 -> PB3




static void RS_set(int rs)
{
  if(rs)
    {
      PORTB |= (1 << 4);
    }
  else
    {
      PORTB &= ~(1 << 4);
    }

}

static void RW_set(int rw)
{
  if(rw)
    {
      PORTC |= (1 << 0);
    }
  else
    {
      PORTC &= ~(1<<0);
    }

}

static void E_set(int e) // pb5
{
  if(e)
    {
      PORTB |= (1<<5);
    }
  else
    {
      PORTB &= ~(1 << 5);
    }
}


static void wait(void)
{
}

static void write_nibble(uint8_t nib)
{
  nib &= 0x0f;  // Use only low four bits
  uint8_t tmp = PORTB;
  tmp &= 0xf0;  // Keep port's high bits as-is
  tmp |= nib;   // Merge the nibble bits
  PORTB = tmp;
  _delay_us(5);//5);
  E_set(1);
  _delay_us(5); //5);
  E_set(0);
  _delay_us(5); //5);
  
}


//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_init
///  \brief Initializes LCD display
///  \param[in] num_cols Number of columns on display
///  \param[in] num_rows Number of rows on display
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_init(uint8_t num_cols, uint8_t num_rows)
{
   columns = num_cols;
   rows = num_rows;

   DDRB |= (1 << 4);  // RS as output
   DDRC |= (1 << 0);  // RW as output
   DDRB |= (1 << 5);  // E as output
   DDRB |= 0x0f;      // D4 to D7 as output

   RS_set(0);  // Command mode
   RW_set(0);  // Write mode
   E_set(0);
   _delay_ms(15);  // wait for it to finish initialization
   // Set 4 bit mode
   write_nibble(3);
   _delay_us(20); //50);
   write_nibble(3);
   _delay_us(20); //50);
   write_nibble(2);
   _delay_us(20); //50);
   // Now set it how we want it
   LCD_44780_function_set(LCD_44780_TWO_LINES);
   LCD_44780_entry_mode(LCD_44780_INCREMENT);
   LCD_44780_display_enable(LCD_44780_ON); // turn on the display
   LCD_44780_clear();
   LCD_44780_home();
}

//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_command
///  \brief Send a command to lcd
///  \param[in]  cmd  The command byte to send
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_write_command( uint8_t cmd)
{
  RS_set(CMD);
  write_nibble( (cmd >> 4) & 0x0f );
  write_nibble( cmd & 0x0f );
  _delay_us(20); //50);
}

//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_data
///  \param[in] data The data byte to write to LCD
/////////////////////////////////////////////////////////////////////////////
void LCD_44780_write_data(uint8_t data)
{
  RS_set(DAT);
  write_nibble( (data >> 4) & 0x0f );
  write_nibble( data & 0x0f );
  _delay_us(20); //50);
}

//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_clear
/// \brief Clears display and sets address to 0
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_clear(void)
{
  LCD_44780_write_command(0x01);
  _delay_ms(2);
}

//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_home
/// \brief Homes cursor
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_home(void)
{
  LCD_44780_write_command(0x02);
  _delay_ms(2);
}

//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_entry_mode
/// \brief Sets cursor move directon and display shift on entry
/// @param[in] p Entry Mode enums or'ed together.
/// @param
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_entry_mode(LCD_44780_param_t p)
{
  uint8_t cmd = 0x04;
  cmd |= p;
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_display_enable
/// @brief Enables or disables display, cursor, and blink
/// @param[in] p Display Enable enums or'ed together
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_display_enable(int p)
{
  uint8_t cmd = 0x08;
  cmd |= p;
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_cursor_display_shift
/// @brief Sets cursor or display shift mode
/// @param[in] p Cursor Display Shift enums or'ed together.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_cursor_display_shift(int p)
{
  uint8_t cmd = 0x10;
  cmd |= p;
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_function_set
/// @brief SEts display length, lines, and font
/// @param[in] p Function Set enums or'ed together.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_function_set(int p)
{
  uint8_t cmd = 0x20;
  cmd |= p;
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_set_CGRAM_address
/// @brief Sets CGRAM address pointer.
/// @param[in] adr  Address to set.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_set_CGRAM_address(int adr)
{
  uint8_t cmd = 0x40 + (uint8_t)(adr & 0x3f);
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_set_DDRAM_address
/// @brief Sets DDRAM address pointer.
/// @param[in] adr Address to set.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_set_DDRAM_address(int adr)
{
  uint8_t cmd = 0x80 + (uint8_t)(adr & 0x7f);
  LCD_44780_write_command(cmd);
  _delay_us(40);
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_read_status
/// @brief Reads busy flag and address.
/// @return Returns byte with busy flag and address.
//////////////////////////////////////////////////////////////////////////////
uint8_t LCD_44780_read_status(void)
{
  uint8_t rtn = 0;
  // TODO read byte here
  return rtn;
}

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_read_data
/// @brief Reads data from display.
/// @return The data.
//////////////////////////////////////////////////////////////////////////////
uint8_t LCD_44780_read_data(void)
{
  uint8_t rtn = 0;
  // TODO add stuff here
  return rtn;
}


/////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_string
///  \param[in] str String to write, null terminated
/// @return Number of characters written.
//////////////////////////////////////////////////////////////////////////////
int LCD_44780_write_string(char* str)
{
  int cnt = 0;
  char ch;
  while( ch = *str )
    {
      cnt++;
      LCD_44780_write_data(ch);
      str++;
    }
  return cnt;
}

  
