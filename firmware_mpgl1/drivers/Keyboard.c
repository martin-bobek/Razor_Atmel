#include "configuration.h"

/* Protected Globals */
volatile fnCode_type PS2_Handler = PS2Start_ReceiveState;

/* Private Globals */
static volatile u8 au8PS2_RBuffer[PS2_RBUFFER_SIZE];
static volatile u8 u8PS2_RBufferPosition = 0;
static u8 u8PS2_RBufferUnread = 0;

static volatile u8 au8PS2_TBuffer[PS2_TBUFFER_SIZE];
static u8 u8PS2_TBufferPosition = 0;
static volatile u8 u8PS2_TBufferUnsent = 0;

static u8 au8Char_Buffer[CHAR_BUFFER_SIZE];
static u8 u8Char_BufferPosition = 0;
static u8 u8Char_BufferUnread = 0;

static volatile u8 u8BitCount = 0;
static volatile u8 u8Parity = 0;

//static volatile ResendState_t bRetransmit = NO_RESEND;
//static volatile LockUpdateState_t LockUpdate = NO_UPDATE;
static u8 u8Modifiers = 0;

/* Lookup Tables */
static const u8 au8NormalTable[128] = {
/*    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
      0 ,  0 ,F09_,F05_,F03_,F01_,F02_,F12_,  0 ,F10_,F08_,F06_,F04_,'\t', '`',  0 ,  /* 0 */
      0 ,LAL_,LSH_,  0 ,LCT_, 'q', '1',  0 ,  0 ,  0 , 'z', 's', 'a', 'w', '2',  0 ,  /* 1 */
      0 , 'c', 'x', 'd', 'e', '4', '3',  0 ,  0 , ' ', 'v', 'f', 't', 'r', '5',  0 ,  /* 2 */
      0 , 'n', 'b', 'h', 'g', 'y', '6',  0 ,  0 ,  0 , 'm', 'j', 'u', '7', '8',  0 ,  /* 3 */
      0 , ',', 'k', 'i', 'o', '0', '9',  0 ,  0 , '.', '/', 'l', ';', 'p', '-',  0 ,  /* 4 */
      0 ,  0 ,'\'',  0 , '[', '=',  0 ,  0 ,CPS_,RSH_,ENT_, ']',  0 ,'\\',  0 ,  0 ,  /* 5 */
      0 ,  0 ,  0 ,  0 ,  0 ,  0 ,BKS_,  0 ,  0 ,KP1_,  0 ,KP4_,KP7_,  0 ,  0 ,  0 ,  /* 6 */
    KP0_,KPE_,KP2_,KP5_,KP6_,KP8_,ESC_,NUM_,F11_,KAD_,KP3_,KSU_,KMU_,KP9_,  0 ,  0 }; /* 7 */
static const u8 au8ShiftedTable[128] = {
/*    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
      0 ,  0 ,F09_,F05_,F03_,F01_,F02_,F12_,  0 ,F10_,F08_,F06_,F04_,'\t', '~',  0 ,  /* 0 */
      0 ,LAL_,LSH_,  0 ,LCT_, 'Q', '!',  0 ,  0 ,  0 , 'Z', 'S', 'A', 'W', '@',  0 ,  /* 1 */
      0 , 'C', 'X', 'D', 'E', '$', '#',  0 ,  0 , ' ', 'V', 'F', 'T', 'R', '%',  0 ,  /* 2 */
      0 , 'N', 'B', 'H', 'G', 'Y', '^',  0 ,  0 ,  0 , 'M', 'J', 'U', '&', '*',  0 ,  /* 3 */
      0 , '<', 'K', 'I', 'O', ')', '(',  0 ,  0 , '>', '?', 'L', ':', 'P', '_',  0 ,  /* 4 */
      0 ,  0 ,'\"',  0 , '{', '+',  0 ,  0 ,CPS_,RSH_,ENT_, '}',  0 , '|',  0 ,  0 ,  /* 5 */
      0 ,  0 ,  0 ,  0 ,  0 ,  0 ,BKS_,  0 ,  0 ,KP1_,  0 ,KP4_,KP7_,  0 ,  0 ,  0 ,  /* 6 */
    KP0_,KPE_,KP2_,KP5_,KP6_,KP8_,ESC_,NUM_,F11_,KAD_,KP3_,KSU_,KMU_,KP9_,  0 ,  0 }; /* 7 */

/* Interrupt Handlers */
void TC0_IrqHandler(void)
{
  AT91C_BASE_TC0->TC_SR;
  PS2Start_TransmitState();
}

/* Public Functions */
u8 KeyboardData(void)
{
  if (u8Char_BufferPosition == u8Char_BufferUnread)
    return '\0';
  u8 u8Char = au8Char_Buffer[u8Char_BufferUnread];
  
  u8Char_BufferUnread++;
  if (u8Char_BufferUnread == CHAR_BUFFER_SIZE)
    u8Char_BufferUnread = 0;
  
  return u8Char;
}

/* Protected Functions */
void KeyboardInitialize(void)
{ 
  AT91C_BASE_PIOA->PIO_FELLSR = PS2_CLOCK_MSK;                  /* Selects Falling Edge/Low Level detection on Clock line */
  AT91C_BASE_PIOA->PIO_ESR = PS2_CLOCK_MSK;                     /* Selects Edge detection on Clock line */
  AT91C_BASE_PIOA->PIO_AIMER = PS2_CLOCK_MSK;                   /* Enables additional interrupt modes for Clock line */
  AT91C_BASE_PIOA->PIO_IER = PS2_CLOCK_MSK;                     /* Enables interrupts on clock line */
  
  AT91C_BASE_TC0->TC_CMR = AT91C_TC_CPCSTOP | AT91C_TC_WAVE;    /* Clock stopped by RC compare and Waveform mode is enabled */
  AT91C_BASE_TC0->TC_RC = 2400;                                 /* Value of RC: corresponds to 100us */
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN;                      /* Enables Timer Counter 0 clock */
  AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;                       /* Enables interrupt on RC compare */
  
  AT91C_BASE_NVIC->NVIC_ICPR[0] = 1 << IRQn_TC0;                /* Clears any pending Timer Counter 0 interrupts */
  AT91C_BASE_NVIC->NVIC_ISER[0] = 1 << IRQn_TC0;                /* Enables Timer Counter 0 interrupts */
}
void KeyboardRunActiveState(void)
{
  static PS2Alt_t alternate = NOMOD;
  while (u8PS2_RBufferUnread != u8PS2_RBufferPosition)
  {
    u8 character;
    u8 u8DataPacket = au8PS2_RBuffer[u8PS2_RBufferUnread];
    
    u8PS2_RBufferUnread++;
    if (u8PS2_RBufferUnread == PS2_RBUFFER_SIZE)
      u8PS2_RBufferUnread = 0;
    
    switch (u8DataPacket)
    {
    case 0xF0:  /* Break Modifier */
      if (alternate == ALT0)
        alternate = ALT0BRK;
      else
        alternate = BRK;
      break;
    case 0xE0:  /* Alternate 0 Modifier */
      alternate = ALT0;
      break;
    default:
      switch (alternate)
      {
      case BRK:
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
        break;
      case ALT0:
        alternate = NOMOD;
        switch (u8DataPacket)
        {
        case 0x4A:  /* Keypad / */
          PushChar(KSL_);
          break;
        case 0x5A:  /* NUM Enter */
          PushChar(KEN_);
          break;
        }
        break;
      case ALT0BRK:
        alternate = NOMOD;
        break;
      default:
        if (u8Modifiers & MOD_SHIFT)
          character = au8ShiftedTable[u8DataPacket];
        else
          character = au8NormalTable[u8DataPacket];
        if (u8Modifiers & MOD_CAPS_LOCK)
        {
          if (('a' <= character) && (character <= 'z'))
            character -= 'a' - 'A';
          else if (('A' <= character) && (character <= 'Z'))
            character += 'a' - 'A';
        }
        switch (character)
        {
        case LSH_:
          u8Modifiers |= MOD_LSHIFT;
          ComputeUpper();
          break;
        case RSH_:
          u8Modifiers |= MOD_RSHIFT;
          ComputeUpper();
          break;
        case CPS_:
          u8Modifiers ^= MOD_CAPS_LOCK;
          ComputeUpper();
          break;
        }
        if (character)
          PushChar(character);
        break;
      }
    }
  }
  
  
#if 0
  static u16 u16Counter = 0;
  u8 u8DataPacket;
  if (u8PS2_RBufferUnread != u8PS2_RBufferPosition)
  {
    u8DataPacket = au8PS2_RBuffer[u8PS2_RBufferUnread];
    
    u8PS2_RBufferUnread++;
    if (u8PS2_RBufferUnread == PS2_RBUFFER_SIZE)
    {
      u8PS2_RBufferUnread = 0;
    }
    
    PushChar(u8DataPacket);  
  }
  
  if (u16Counter == 0)
  {
    u16Counter++;
    
    au8PS2_TBuffer[u8PS2_TBufferPosition] = 0xED;
    u8PS2_TBufferPosition++;
    if (u8PS2_TBufferPosition == PS2_TBUFFER_SIZE)
    {
      u8PS2_TBufferPosition = 0;
    }
  }
  if (u16Counter == 1)
  {
    if (u8DataPacket == 0xFA)
    {
      u16Counter++;
    }
  }
  if (u16Counter == 2)
  {
    u16Counter++;
    
    au8PS2_TBuffer[u8PS2_TBufferPosition] = 0x7;
    u8PS2_TBufferPosition++;
    if (u8PS2_TBufferPosition == PS2_TBUFFER_SIZE)
    {
      u8PS2_TBufferPosition = 0;
    }
  }
  
  /*u16Counter++;
  if (u16Counter == 1000)
  {
    u16Counter = 0;
    
    au8PS2_TBuffer[u8PS2_TBufferPosition] = 0xEE;
    u8PS2_TBufferPosition++;
    if (u8PS2_TBufferPosition == PS2_TBUFFER_SIZE)
    {
      u8PS2_TBufferPosition = 0;
    }
  }*/
  if ((PS2_Handler == PS2Start_ReceiveState) && 
      (u8PS2_TBufferUnsent != u8PS2_TBufferPosition))
  {
    PS2Inhibit_TransmitState();
  }
#endif
#if 0
  static PS2Alt_t alternate = NOMOD;
  while (u8PS2_RBufferUnread != u8PS2_RBufferPosition)
  {
    u8 u8DataPacket = au8PS2_RBuffer[u8PS2_RBufferUnread];
    
    u8PS2_RBufferUnread++;
    if (u8PS2_RBufferUnread == PS2_RBUFFER_SIZE)
    {
      u8PS2_RBufferUnread = 0;
    }
    
    if (u8DataPacket == 0xFA)
      while(1);
    else if (u8DataPacket == 0xFE)
      while(1);
    else if (u8DataPacket == 0xEE)
      while(1);
    
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
          PushLockUpdate();
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
#endif
}

void PS2Start_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
    while(1);
  au8PS2_RBuffer[u8PS2_RBufferPosition] = 0;
  PS2_Handler = PS2Data_ReceiveState;
}
void PS2Data_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
  {
    u8Parity++;
    u8 u8BitMsk = 1 << u8BitCount;
    u8 u8Index = u8PS2_RBufferPosition;
    au8PS2_RBuffer[u8Index] |= u8BitMsk;
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
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
    u8Parity++;
  if (!(u8Parity % 2))
    while(1);
  u8Parity = 0;
  PS2_Handler = PS2Stop_ReceiveState;
}
void PS2Stop_ReceiveState(void)
{
  if (!(AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK))
    while(1);
  u8PS2_RBufferPosition++;
  if (u8PS2_RBufferPosition == PS2_RBUFFER_SIZE)
    u8PS2_RBufferPosition = 0;
  
  if (u8PS2_TBufferUnsent != u8PS2_TBufferPosition)
    PS2Inhibit_TransmitState();
  else
    PS2_Handler = PS2Start_ReceiveState;
}

void PS2Inhibit_TransmitState(void)
{
  AT91C_BASE_PIOA->PIO_IDR = PS2_CLOCK_MSK;                     /* Disables interrupts on clock line */
  u8BitCount = 0;
  u8Parity = 0;
  AT91C_BASE_PIOA->PIO_CODR = PS2_CLOCK_MSK;                    /* Bring Clock line low */
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;                      /* Starts 100us timer */
}
void PS2Start_TransmitState(void)
{
  PS2_Handler = PS2Data_TransmitState;
  AT91C_BASE_PIOA->PIO_ISR;                                     /* Clears the pending Clock line interrupt request */
  AT91C_BASE_PIOA->PIO_IER = PS2_CLOCK_MSK;                     /* Enables interrupts on Clock line */
  AT91C_BASE_PIOA->PIO_CODR = PS2_DATA_MSK;                     /* Brings Data line low */
  AT91C_BASE_PIOA->PIO_SODR = PS2_CLOCK_MSK;                    /* Releases Clock line */
}
void PS2Data_TransmitState(void)
{
  u8 u8BitMsk = 1 << u8BitCount;
  u8 u8Index = u8PS2_TBufferUnsent;
  if (au8PS2_TBuffer[u8Index] & u8BitMsk)
  {
    u8Parity++;
    AT91C_BASE_PIOA->PIO_SODR = PS2_DATA_MSK;                   /* Sets Data line */
  }
  else
    AT91C_BASE_PIOA->PIO_CODR = PS2_DATA_MSK;                   /* Resets Data line */
  
  u8BitCount++;
  if (u8BitCount == 8)
  {
    u8BitCount = 0;
    PS2_Handler = PS2Parity_TransmitState;
  } 
}
void PS2Parity_TransmitState(void)
{
  if (u8Parity % 2)
    AT91C_BASE_PIOA->PIO_CODR = PS2_DATA_MSK;                   /* Resets Data line */
  else
    AT91C_BASE_PIOA->PIO_SODR = PS2_DATA_MSK;                   /* Sets Data line */
  u8Parity = 0;
  PS2_Handler = PS2Stop_TransmitState;
}
void PS2Stop_TransmitState(void)
{
  AT91C_BASE_PIOA->PIO_SODR = PS2_DATA_MSK;                     /* Releases Data line */
  PS2_Handler = PS2Acknowledge_TransmitState;
}
void PS2Acknowledge_TransmitState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
    while(1);
  
  au8PS2_TBuffer[u8PS2_TBufferUnsent] = 0;
  u8PS2_TBufferUnsent++;
  if (u8PS2_TBufferUnsent == PS2_TBUFFER_SIZE)
    u8PS2_TBufferUnsent = 0;
  
  if (u8PS2_TBufferUnsent != u8PS2_TBufferPosition)
    PS2Inhibit_TransmitState();
  else
    PS2_Handler = PS2Start_ReceiveState;
}

/* Private Functions */
static void PushChar(u8 c)
{
  au8Char_Buffer[u8Char_BufferPosition] = c;
  
  u8Char_BufferPosition++;
  if (u8Char_BufferPosition == CHAR_BUFFER_SIZE)
    u8Char_BufferPosition = 0;
}
static void PushLockUpdate()
{
  au8PS2_TBuffer[u8PS2_TBufferPosition] = 0xED;
  u8PS2_TBufferPosition++;
  if (u8PS2_TBufferPosition == PS2_TBUFFER_SIZE)
  {
    u8PS2_TBufferPosition = 0;
  }
  /*
  au8PS2_TBuffer[u8PS2_TBufferPosition] = u8Modifiers & 0x7;
  u8PS2_TBufferPosition++;
  if (u8PS2_TBufferPosition == PS2_TBUFFER_SIZE)
  {
    u8PS2_TBufferPosition = 0;
  }
  */
  if (PS2_Handler == PS2Start_ReceiveState)
  {
    PS2Inhibit_TransmitState();
  }
}
static void ComputeUpper()
{
  if ((u8Modifiers & MOD_LSHIFT) || (u8Modifiers & MOD_RSHIFT))
  {
    u8Modifiers |= MOD_SHIFT;
    if (u8Modifiers & MOD_CAPS_LOCK)
      u8Modifiers &= ~MOD_SCL_UPPER;
    else
      u8Modifiers |= MOD_SCL_UPPER;
  }
  else
  {
    u8Modifiers &= ~MOD_SHIFT;
    if (u8Modifiers & MOD_CAPS_LOCK)
      u8Modifiers |= MOD_SCL_UPPER;
    else
      u8Modifiers &= ~MOD_SCL_UPPER;
  }
}