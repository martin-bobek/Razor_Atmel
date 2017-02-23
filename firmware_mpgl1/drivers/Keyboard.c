#include "configuration.h"

/* Protected Globals */
volatile fnCode_type PS2_Handler = PS2Start_ReceiveState;

/* Private Globals */
static volatile u8 au8PS2_Buffer[PS2_BUFFER_SIZE];
static volatile u8 u8PS2_BufferPosition = 0;
static u8 u8PS2_BufferUnread = 0;

static u8 au8Char_Buffer[CHAR_BUFFER_SIZE];
static u8 u8Char_BufferPosition = 0;
static u8 u8Char_BufferUnread = 0;

static volatile u8 u8BitCount = 0;
static volatile u8 u8Parity = 0;

static u8 u8Modifiers = 0;

/* Public Functions */
u8 KeyboardData(void)
{
  if (u8Char_BufferPosition == u8Char_BufferUnread)
  {
    return '\0';
  }
  u8 u8Char = au8Char_Buffer[u8Char_BufferUnread];
  
  u8Char_BufferUnread++;
  if (u8Char_BufferUnread == CHAR_BUFFER_SIZE)
  {
    u8Char_BufferUnread = 0;
  }
  
  return u8Char;
}


/* Protected Functions */
void KeyboardInitialize(void)
{
  
  
  AT91C_BASE_PIOA->PIO_FELLSR = PS2_CLOCK_MSK;              /* Selects Falling Edge/Low Level detection on Clock line */
  AT91C_BASE_PIOA->PIO_ESR = PS2_CLOCK_MSK;                 /* Selects Edge detection on Clock line */
  AT91C_BASE_PIOA->PIO_AIMER = PS2_CLOCK_MSK;               /* Enables additional interrupt modes for Clock line */
  AT91C_BASE_PIOA->PIO_IER = PS2_CLOCK_MSK;                 /* Enables interrupts on clock line */
}

void KeyboardRunActiveState(void)
{
  static PS2Alt_t alternate = NOMOD;
  while (u8PS2_BufferUnread != u8PS2_BufferPosition)
  {
    u8 u8DataPacket = au8PS2_Buffer[u8PS2_BufferUnread];
    
    u8PS2_BufferUnread++;
    if (u8PS2_BufferUnread == PS2_BUFFER_SIZE)
    {
      u8PS2_BufferUnread = 0;
    }
    
    switch (u8DataPacket)
    {
    case 0xF0:  /* Break Modifier */
      if (alternate == ALT0)
      {
        alternate = ALT0BRK;
      }
      else
      {
        alternate = BRK;
      }
      break;
    case 0xE0:  /* Alternate 0 Modifier */
      alternate = ALT0;
      break;
      
    default:
      if (alternate == BRK) 
      {
        alternate = NOMOD;
        switch (u8DataPacket)
        {
        case 0x12:  /* L Shift */
          u8Modifiers &= ~MOD_LSHIFT;
          ComputeUpper();
          break;
        case 0x59:  /* R Shift */
          u8Modifiers &= ~MOD_RSHIFT;
          ComputeUpper();
          break;
        }
      }
      else if (alternate == ALT0)
      {
        alternate = NOMOD;
        switch (u8DataPacket)
        {
        case 0x4A:  /* NUM / */
          PushChar('/');
          break;
        case 0x5A:  /* NUM Enter */
          PushChar('\r');
          PushChar('\n');
          break;
        }
      }
      else if (alternate == ALT0BRK)
      {
        alternate = NOMOD;
      }
      else
      {
        switch (u8DataPacket)
        {
        case 0x1C:  /* A */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'A' : 'a');
          break;
        case 0x32:  /* B */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'B' : 'b');
          break;
        case 0x21:  /* C */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'C' : 'c');
          break;
        case 0x23:  /* D */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'D' : 'd');
          break;
        case 0x24:  /* E */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'E' : 'e');
          break;
        case 0x2B:  /* F */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'F' : 'f');
          break;
        case 0x34:  /* G */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'G' : 'g');
          break;
        case 0x33:  /* H */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'H' : 'h');
          break;
        case 0x43:  /* I */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'I' : 'i');
          break;
        case 0x3B:  /* J */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'J' : 'j');
          break;
        case 0x42:  /* K */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'K' : 'k');
          break;
        case 0x4B:  /* L */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'L' : 'l');
          break;
        case 0x3A:  /* M */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'M' : 'm');
          break;
        case 0x31:  /* N */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'N' : 'n');
          break;
        case 0x44:  /* O */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'O' : 'o');
          break;
        case 0x4D:  /* P */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'P' : 'p');
          break;
        case 0x15:  /* Q */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'Q' : 'q');
          break;
        case 0x2D:  /* R */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'R' : 'r');
          break;
        case 0x1B:  /* S */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'S' : 's');
          break;
        case 0x2C:  /* T */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'T' : 't');
          break;
        case 0x3C:  /* U */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'U' : 'u');
          break;
        case 0x2A:  /* V */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'V' : 'v');
          break;
        case 0x1D:  /* W */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'W' : 'w');
          break;
        case 0x22:  /* X */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'X' : 'x');
          break;
        case 0x35:  /* Y */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'Y' : 'y');
          break;
        case 0x1A:  /* Z */
          PushChar((u8Modifiers & MOD_SCL_UPPER) ? 'Z' : 'z');
          break;
        case 0x45:  /* 0 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? ')' : '0');
          break;
        case 0x16:  /* 1 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '!' : '1');
          break;
        case 0x1E:  /* 2 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '@' : '2');
          break;
        case 0x26:  /* 3 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '#' : '3');
          break;
        case 0x25:  /* 4 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '$' : '4');
          break;
        case 0x2E:  /* 5 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '%' : '5');
          break;
        case 0x36:  /* 6 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '^' : '6');
          break;
        case 0x3D:  /* 7 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '&' : '7');
          break;
        case 0x3E:  /* 8 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '*' : '8');
          break;
        case 0x46:  /* 9 */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '(' : '9');
          break;
        case 0x0E:  /* ` */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '~' : '`');
          break;
        case 0x4E:  /* - */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '_' : '-');
          break;
        case 0x55:  /* = */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '+' : '=');
          break;
        case 0x5D:  /* \ */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '|' : '\\');
          break;
        case 0x29:  /* Space */
          PushChar(' ');
          break;
        case 0x66:  /* Backspace */
          PushChar('\b');
          PushChar(' ');
          PushChar('\b');
          break;
        case 0x5A:  /* Enter */
          PushChar('\r');
          PushChar('\n');
          break;
        case 0x0D:  /* Tab */
          PushChar('\t');
          break;
        case 0x58:  /* Caps Lock */
          u8Modifiers ^= MOD_CAPS_LOCK;
          ComputeUpper();
          break;
        case 0x12:  /* L Shift */
          u8Modifiers |= MOD_LSHIFT;
          ComputeUpper();
          break;
        case 0x59:  /* R Shift */
          u8Modifiers |= MOD_RSHIFT;
          ComputeUpper();
          break;
        case 0x54:  /* [ */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '{' : '[');
          break;
        case 0x7C:  /* NUM * */
          PushChar('*');
          break;
        case 0x7B:  /* NUM - */
          PushChar('-');
          break;
        case 0x79:  /* NUM + */
          PushChar('+');
          break;
        case 0x71:  /* NUM . */
          PushChar('.');
          break;
        case 0x70:  /* NUM 0 */
          PushChar('0');
          break;
        case 0x69:  /* NUM 1 */
          PushChar('1');
          break;
        case 0x72:  /* NUM 2 */
          PushChar('2');
          break;
        case 0x7A:  /* NUM 3 */
          PushChar('3');
          break;
        case 0x6B:  /* NUM 4 */
          PushChar('4');
          break;
        case 0x73:  /* NUM 5 */
          PushChar('5');
          break;
        case 0x74:  /* NUM 6 */
          PushChar('6');
          break;
        case 0x6C:  /* NUM 7 */
          PushChar('7');
          break;
        case 0x75:  /* NUM 8 */
          PushChar('8');
          break;
        case 0x7D:  /* NUM 9 */
          PushChar('9');
          break;
        case 0x5B:  /* ] */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '}' : ']');
          break;
        case 0x4C:  /* ; */
          PushChar((u8Modifiers & MOD_S_UPPER) ? ':' : ';'); 
          break;
        case 0x52:  /* ' */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '"' : '\'');
          break;
        case 0x41:  /* , */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '<' : ',');
          break;
        case 0x49:  /* . */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '>' : '.');
          break;
        case 0x4A:  /* / */
          PushChar((u8Modifiers & MOD_S_UPPER) ? '?' : '/');
          break;
        }
      }
    }
  }
}

void PS2Start_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
  {
    while(1);
  }
  
  au8PS2_Buffer[u8PS2_BufferPosition] = 0;
  PS2_Handler = PS2Data_ReceiveState;
}

void PS2Data_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
  {
    u8Parity++;
    u8 u8BitMsk = 1 << u8BitCount;
    u8 u8Index = u8PS2_BufferPosition;
    au8PS2_Buffer[u8Index] |= u8BitMsk;
  }
  
  u8BitCount++;
  if (u8BitCount == 8)
  {
    u8BitCount = 0;
    PS2_Handler = PS2Parity_ReceiveState;
  }
}

void PS2Parity_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & (1 << 13))
  {
    u8Parity++;
  }
  if (!(u8Parity % 2))
  {
    while(1);
  }
  u8Parity = 0;
  PS2_Handler = PS2Stop_ReceiveState;
}

void PS2Stop_ReceiveState(void)
{
  if (!(AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK))
  {
    while(1);
  }
  u8PS2_BufferPosition++;
  if (u8PS2_BufferPosition == PS2_BUFFER_SIZE)
  {
    u8PS2_BufferPosition = 0;
  }
  
  PS2_Handler = PS2Start_ReceiveState;
}

/* Private Functions */
static void PushChar(u8 c)
{
  au8Char_Buffer[u8Char_BufferPosition] = c;
  
  u8Char_BufferPosition++;
  if (u8Char_BufferPosition == CHAR_BUFFER_SIZE)
  {
    u8Char_BufferPosition = 0;
  }
}

static void ComputeUpper()
{
  if ((u8Modifiers & MOD_LSHIFT) || (u8Modifiers & MOD_RSHIFT))
  {
    u8Modifiers |= MOD_S_UPPER;
    if (u8Modifiers & MOD_CAPS_LOCK)
    {
      u8Modifiers &= ~MOD_SCL_UPPER;
    }
    else
    {
      u8Modifiers |= MOD_SCL_UPPER;
    }
  }
  else
  {
    u8Modifiers &= ~MOD_S_UPPER;
    if (u8Modifiers & MOD_CAPS_LOCK)
    {
      u8Modifiers |= MOD_SCL_UPPER;
    }
    else
    {
      u8Modifiers &= ~MOD_SCL_UPPER;
    }
  }
}