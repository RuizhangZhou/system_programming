#include "lcd.h"
#ifdef VERSUCH
#include "util.h"
#endif

#pragma GCC push_options
#pragma GCC optimize("O3")

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

//! Global var, stores character count.
/*!
 *  \internal
 *  This value is in [0;32]
 *       ... yes, this is no mistake it can be both 0 and 32
 */
uint8_t charCtr;

/*!
 *  Internally used to turn on LCD Pin EN (Enable) for 1us.
 *  \internal
 */
void lcd_enable(void) {
  sbi(LCD_PORT_DATA, 5); // Enable
  _delay_us(1);
  cbi(LCD_PORT_DATA, 5); // Disable
}

/*!
 *  Prepares the LCD to be used with the defined output-port.
 *  Delay times as specified with some reserve
 */
void lcd_init(void) {
  // Write on LCD Port (reading is not needed)
  LCD_PORT_DDR = 0xFF;

  // Init routine (see specifications)
  delayMs(15);
  LCD_PORT_DATA = LCD_INIT;
  lcd_enable();
  delayMs(5);
  lcd_enable();
  delayMs(1);
  lcd_enable();
  delayMs(1);

  // LCD is connected with 4 pins for data
  LCD_PORT_DATA = LCD_4BIT_MODE;
  lcd_enable();
  delayMs(1);

  // Display type is 2 line / 5x7 character set
  lcd_command(LCD_TWO_LINES | LCD_5X7);
  lcd_command(LCD_DISPLAY_ON | LCD_HIDE_CURSOR);

  // Do not increment DDRAM address or move display
  lcd_command(LCD_NO_INC_ADDR | LCD_NO_MOVE);
  lcd_clear();

  // Register custom characters
  lcd_registerCustomChar(LCD_CC_IXI, LCD_CC_IXI_BITMAP);
  lcd_registerCustomChar(LCD_CC_TILDE, LCD_CC_TILDE_BITMAP);
  lcd_registerCustomChar(LCD_CC_DEGREE, LCD_CC_DEGREE_BITMAP);
  lcd_registerCustomChar(LCD_CC_ACCENT, LCD_CC_ACCENT_BITMAP);
  lcd_registerCustomChar(LCD_CC_BACKSLASH, LCD_CC_BACKSLASH_BITMAP);
  lcd_registerCustomChar(LCD_CC_MU, LCD_CC_MU_BITMAP);

  lcd_clear();
}

/*!
 *  Moves the cursor to the first character of the first line of the LCD.
 */
void lcd_line1(void) {
  lcd_command(LCD_LINE_1);
  charCtr = 0;
}

/*!
 *  Moves the cursor to the first character of the second line of the LCD.
 */
void lcd_line2(void) {
  lcd_command(LCD_LINE_2);
  charCtr = 16;
}

/*!
 * Moves the cursor one step back.
 */
void lcd_back(void) {
  lcd_goto(1 + (charCtr - 1) / 16, 1 + (charCtr - 1) % 16);
}

/*!
 * Moves the cursor one step forward.
 */
void lcd_forward(void) {
  lcd_goto(1 + (charCtr + 1) / 16, 1 + (charCtr + 1) % 16);
}

/*!
 * Moves the cursor to the first char
 */
void lcd_home(void) { lcd_goto(1 + charCtr / 16, 0); }

/*!
 * Relatively moves the cursor on the LCD.
 */
void lcd_move(char row, char column) {
  // There are two rows
  lcd_goto(1 + (2 + charCtr / 16 + row) % 2, 1 + (16 + charCtr + column) % 16);
}

/*!
 *  Moves the cursor to a specific position on the LCD. Row and column start
 *  counting at 1, so (1,1) is top left, (2,16) is bottom right.
 *
 *  \param row     The row to jump to (may be 1 or 2).
 *  \param column   The column to jump to (may be 1...16).
 */
void lcd_goto(unsigned char row, unsigned char column) {
  // Restrict params to valid values
  if (--row > 1) {
    row = 0;
  }
  if (--column > 15) {
    column = 0;
  }

  // Calculate position and get command
  char command = LCD_CURSOR_MOVE_R + column + row * LCD_NEXT_ROW;

  // Update char counter
  charCtr = row * 16 + column;

  lcd_command(command);
}

/*!
 *  Sends a stream to the LCD. The stream is a two-char pair which either
 *  holds a command or a printable char.
 *  This function is used by lcd_command and lcd_writeChar.
 *
 *  \param firstByte The first value to send.
 *  \param secondByte The second value to send.
 */
void lcd_sendStream(uint8_t firstByte, uint8_t secondByte) {
  // Check if interrupts are set and store that state
  uint8_t sreg = SREG & (1 << 7);

  // Interrupts off
  cli();
  uint16_t iterations = 0;
  bool busy = false;

  // Wait while LCD is busy or timeout was reached
  do {
    // Read busy flag state:
    // Set R/W port to high, all others to low
    LCD_PORT_DATA = 0x40;

    // Set enable port to high to read first nibble
    sbi(LCD_PORT_DATA, 5);

    // Enable reading from pins 1 to 4
    LCD_PORT_DDR = 0xF0;

    // Set pull-ups
    LCD_PORT_DATA |= 0x0F;

    // Read busy flag (port 4) and store state to 'busy'
    busy = LCD_PIN & 0x08;

    // Set enable port back to low
    cbi(LCD_PORT_DATA, 5);

    // Second nibble is not used, waste it by calling lcd_enable
    lcd_enable();

    // Increase count of iterations
    iterations++;
    if (iterations == LCD_BUSY_TIMEOUT) {
      // Timeout: Try to reset LCD
      lcd_enable();

      // Restore interrupt flag
      SREG |= sreg;
      return;
    }
  } while (busy);

  // Transmit command:
  LCD_PORT_DDR = 0xFF;

  // Send first Byte
  LCD_PORT_DATA = firstByte;
  lcd_enable();

  // Send second Byte
  LCD_PORT_DATA = secondByte;
  lcd_enable();

  // Restore interrupt flag
  SREG |= sreg;
}

/*!
 *  Sends a specific command to the LCD. This function is only used
 *  internally. There is no need to explicitly call it as its functionality is
 *  encapsulated within the other functions.
 *
 *  \param command The command to be executed.
 *  \internal
 */
void lcd_command(uint8_t command) {
  lcd_sendStream((command >> 4) & 0xF, command & 0xF);
}

/*!
 *  Writes an 8-Bit ASCII-like-value to the LCD.
 *  Supports automatic line breaks.
 *
 *  \param character  The character to be written.
 */
void lcd_writeChar(char character) {
  // Check if interrupts are set and store that state
  uint8_t sreg = SREG & (1 << 7);
  cli();

  // Check if line shall be changed
  if (character == '\n') {
    charCtr = (charCtr & 0x10) + 0x10; // <16 -> 16, <32 -> 32
  }

  if (charCtr == 0x10) {
    lcd_line2();
  } else if (charCtr == 0x20) {
    lcd_clear();
    lcd_line1();
  }

  if (character != '\n') {
    // Check for non-ASCII characters the LCD knows
    switch (character) {
    case '�':
      character = 0xE1;
      break;
    case '�':
      character = 0xEF;
      break;
    case '�':
      character = 0xF5;
      break;
    case '�':
      character = 0xE2;
      break;
    case 8:
      character = LCD_CC_IXI;
      break;
    case 9:
    case '~':
      character = LCD_CC_TILDE;
      break;
    case '\\':
      character = LCD_CC_BACKSLASH;
      break;
    case '�':
      character = LCD_CC_MU;
      break;
    case '�':
      character = LCD_CC_DEGREE;
      break;
    case '�':
      character = LCD_CC_ACCENT;
      break;
    }

    lcd_sendStream(0x10 | ((character & 0xF0) >> 4), 0x10 | (character & 0x0F));

    // Update char counter ... Do not modulo it down! we need it to become 32
    charCtr++;
  }

  // Restore interrupt flags
  SREG |= sreg;
}

/*!
 *  Erases the LCD and positions the cursor at the top left corner.
 */
void lcd_clear(void) {
  charCtr = 0;
  lcd_command(LCD_CLEAR);
}

/*!
 *  Erases one line of the LCD. Cursor will not be changed.
 *  \param line  the line which will be erased (may be 1 or 2).
 */
void lcd_erase(uint8_t line) {
  // Save counter
  uint8_t i = 0, oldCtr = charCtr;

  // Restrict param to a valid value
  if (line > 2) {
    line = 1;
  }

  // Go to the beginning of line
  lcd_goto(line, 1);

  // Clear the line
  for (i = 0; i < 16; i++) {
    lcd_writeChar(' ');
  }

  // Restore counter
  charCtr = oldCtr;

  // Restore cursor
  lcd_goto((charCtr / 16) + 1, (charCtr % 16) + 1);
}

/*!
 *  Writes a hexadecimal half-byte (one nibble)
 *
 *  \param number  The number to be written.
 */
void lcd_writeHexNibble(uint8_t number) {
  // Get low and high nibble
  uint8_t const low = number & 0xF;

  if (low < 10) {
    lcd_writeChar(low + '0'); // Write as ASCII number
  } else {
    lcd_writeChar(low - 10 + 'A'); // Write as ASCII letter
  }
}

/*!
 *  Writes a hexadecimal byte (equals two chars)
 *
 *  \param number  The number to be written.
 */
void lcd_writeHexByte(uint8_t number) {
  lcd_writeHexNibble(number >> 4);
  lcd_writeHexNibble(number & 0xF);
}

/*!
 *  Writes a hexadecimal word (equals two bytes)
 *
 *  \param number  The number to be written.
 */
void lcd_writeHexWord(uint16_t number) {
  lcd_writeHexByte(number >> 8);
  lcd_writeHexByte(number);
}

/*!
 * Writes a hexadecimal word, without adding leading 0s
 *
 * \param number The number to be written.
 */
void lcd_writeHex(uint16_t number) {
  uint16_t nib = 16;
  uint8_t print = 0;
  // Iterate over all nibbles and start printing when we find the first non-zero
  while (nib) {
    nib -= 4;
    print |= number >> nib;
    if (print) {
      lcd_writeHexNibble(number >> nib);
    }
  }
}

/*!
 *  Writes a 16 bit integer as a decimal number without leading 0s
 */
void lcd_writeDec(uint16_t number) {
  if (!number) {
    lcd_writeChar('0');
    return;
  }

  uint32_t pos = 10000;
  uint8_t print = 0;
  do {
    uint8_t const digit = number / pos;
    number -= digit * pos;
    if (print |= digit) {
      lcd_writeChar(digit + '0');
    }
  } while (pos /= 10);
}

/*!
 *  Writes a string of 8-Bit ASCII-values to the LCD. This function
 *  supports automated line breaks. If you reach the end of one
 *  line, the next line will be erased and the cursor will
 *  jump to the beginning of the next line. The string needs to be
 *  null terminated correctly.
 *
 *  \param text The string to be written (a pointer to the first character).
 */
void lcd_writeString(char const *text) {
  char c;
  while ((c = *text++)) {
    lcd_writeChar(c);
  }
}

/*!
 *  Writes a draw bar
 *
 *  \param percent Percent of draw bar to be drawn
 */
void lcd_drawBar(uint8_t percent) {
  lcd_clear();
  // Calculate number of bars
  uint16_t const val = ((percent <= 100) ? percent : 100) * 16;
  uint16_t i = 0;
  // Draw bars
  for (i = 0; i < val; i += 100) {
    lcd_writeChar(LCD_CHAR_BAR);
  }
}

/*!
 *  Writes a string of 8-Bit ASCII-values from the program flash memory to
 *  the LCD. This function features automated line breaks as long as you
 *  don't manually do something with the cursor. This works fine in concert
 *  with lcd_write_char without messing up anything.\n
 *  After 32 character have been written, the LCD will be erased and the
 *  cursor starts again at the first character of the first line.
 *  So the function will automatically seize output after 32 characters.\n
 *  This is a mighty tool for saving SRAM memory.
 *
 *  \param string  The string to be written (a pointer to the first character).
 */

void lcd_writeProgString(char const *string) {
  char c;
  while ((c = (char)pgm_read_byte(string++))) {
    lcd_writeChar(c);
  }
}

/*!
 *  Registers a custom char with the LCD CGRAM.
 *  Note that only 5 distinct chars will be stored at a time. The sixth char
 * will override the first.
 *
 *  \param addr is the address where the character is to be stored. This value
 * is interpreted modulo 8. \param chr The passed value is one 32 bit integer
 * witch holds all rows of the character.
 */
void lcd_registerCustomChar(uint8_t addr, uint64_t chr) {
  uint8_t const sreg = SREG & (1 << 7);
  cli();
  lcd_command(0x40 | (0x38 & (addr << 3)));
  _delay_us(40);

  uint8_t i = 8;
  while (i--) {
    uint8_t const row = chr & 0xFF;
    lcd_sendStream(((1 << LCD_RS_PIN) & 0xF0) | ((row >> 4) & 0xF),
                   ((1 << LCD_RS_PIN) & 0xF0) | (row & 0xF));
    _delay_us(40);
    chr >>= 8;
  }
  SREG |= sreg;
}

/*!
 *  Writes a 32bit Number as Hex
 *  \param number is the number to write
 */
void lcd_write32bitHex(uint32_t number) {
  lcd_writeDec(0);
  lcd_writeChar('x');
  lcd_writeHexWord(number >> 16);
  lcd_writeHexWord(number);
}

/*! \brief Prints the passed voltage onto the display (three float places).
 *
 * \param voltage           Binary voltage value.
 * \param valueUpperBound   Upper bound of the binary voltage value (i.e. 1023
 * for 10-bit value). \param voltUpperBound    Upper bound of the float voltage
 * value (i.e. 5 for 5V).
 */
void lcd_writeVoltage(uint16_t voltage, uint16_t valueUpperBound,
                      uint8_t voltUpperBound) {
  uint8_t intVal;
  uint16_t floatVal;

  // Calculate integer and float part of the voltage
  voltage *= voltUpperBound;
  intVal = voltage / valueUpperBound;
  floatVal =
      (uint32_t)(voltage - (intVal * valueUpperBound)) * 1000 / valueUpperBound;

  // Show voltage on display
  lcd_writeDec(intVal);
  lcd_writeChar('.');
  if (floatVal < 100) {
    lcd_writeChar('0');
  }
  if (floatVal < 10) {
    lcd_writeChar('0');
  }
  lcd_writeDec(floatVal);
  lcd_writeChar('V');
}

#pragma GCC pop_options