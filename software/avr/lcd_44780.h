//////////////////////////////////////////////////////////////////////////////
///  @file lcd_44780.h
///  copyright(c) 2023,2014 William R Cooke
///  @brief Interfaces AVR to character LCDs using HD44780 or compatible
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

typedef enum _LCD_44780
  {
    LCD_44780_INCREMENT         = (1<<1),            // Entry Mode
    LCD_44780_DECREMENT         = 0,                 // Entry Mode
    // What about 'S' bit?                           // Entry Mode
    LCD_44780_ON                = (1<<2),            // Display Enable
    LCD_44780_CURSOR            = (1<<1),            // Display Enable
    LCD_44780_BLINK             = (1<<0),            // Display Enable
    LCD_44780_SHIFT_RIGHT       = ((1<<3)|(1<<2)),   // Cursor Display Shift
    LCD_44780_SHIFT_LEFT        = (1<<3),            // Cursor Display Shift
    LCD_44780_8_BIT             = (1<<4),            // Function Set
    LCD_44780_TWO_LINES         = (1<<3),            // Function Set
    LCD_44780_5X10              = (1<<2),            // Function Set
  } LCD_44780_param_t;





//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_init
///  \brief Initializes LCD display
///  \param[in] num_cols Number of columns on display
///  \param[in] num_rows Number of rows on display
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_init(uint8_t num_cols, uint8_t num_rows);


//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_command
///  \brief Send a command to lcd
///  \param[in]  cmd  The command byte to send
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_write_command( uint8_t cmd);


//////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_data
///  \param[in] data The data byte to write to LCD
/////////////////////////////////////////////////////////////////////////////
void LCD_44780_write_data(uint8_t data);


//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_clear
/// \brief Clears display and sets address to 0
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_clear(void);


//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_home
/// \brief Homes cursor
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_home(void);


//////////////////////////////////////////////////////////////////////////////
/// \b LCD_44780_entry_mode
/// \brief Sets cursor move directon and display shift on entry
/// @param[in] p Entry Mode enums or'ed together.
/// @param
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_entry_mode(LCD_44780_param_t p);


//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_display_enable
/// @brief Enables or disables display, cursor, and blink
/// @param[in] p Display Enable enums or'ed together
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_display_enable(int p);


//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_cursor_display_shift
/// @brief Sets cursor or display shift mode
/// @param[in] p Cursor Display Shift enums or'ed together.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_cursor_display_shift(int p);


//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_function_set
/// @brief SEts display length, lines, and font
/// @param[in] p Function Set enums or'ed together.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_function_set(int p);


//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_set_CGRAM_address
/// @brief Sets CGRAM address pointer.
/// @param[in] adr  Address to set.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_set_CGRAM_address(int adr);

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_set_DDRAM_address
/// @brief Sets DDRAM address pointer.
/// @param[in] adr Address to set.
//////////////////////////////////////////////////////////////////////////////
void LCD_44780_set_DDRAM_address(int adr);

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_read_status
/// @brief Reads busy flag and address.
/// @return Returns byte with busy flag and address.
//////////////////////////////////////////////////////////////////////////////
uint8_t LCD_44780_read_status(void);

//////////////////////////////////////////////////////////////////////////////
/// @fn LCD_44780_read_data
/// @brief Reads data from display.
/// @return The data.
//////////////////////////////////////////////////////////////////////////////
uint8_t LCD_44780_read_data(void);


/////////////////////////////////////////////////////////////////////////////
///  \b LCD_44780_write_string
///  \param[in] str String to write, null terminated
/// @return Number of characters written.
//////////////////////////////////////////////////////////////////////////////
int LCD_44780_write_string(char* str);

