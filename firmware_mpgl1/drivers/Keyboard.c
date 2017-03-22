#include "configuration.h"

/* Protected Globals */
volatile fnCode_type PS2_Handler = PS2Start_ReceiveState;

/* Private Globals */
static volatile u8 au8PS2_RBuffer[PS2_RBUFFER_SIZE];
static volatile u8 u8PS2_RBufferPosition = 0;
static u8 u8PS2_RBufferUnread = 0;

//static volatile u8 au8PS2_TBuffer[PS2_TBUFFER_SIZE];
//static u8 u8PS2_TBufferPosition = 0;
//static volatile u8 u8PS2_TBufferUnsent = 0;

static u8 au8Char_Buffer[CHAR_BUFFER_SIZE];
static u8 u8Char_BufferPosition = 0;
static u8 u8Char_BufferUnread = 0;

static volatile u8 u8BitCount = 0;
static volatile u8 u8Parity = 0;
static volatile u8 u8TransmitPacket = 0;

static volatile bool bDResend = FALSE;
//static volatile DResendState_t DResend = NO_RESEND;
static volatile LockUpdateState_t LockUpdate = NO_UPDATE;
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
    KP0_,KPE_,KP2_,KP5_,KP6_,KP8_,ESC_,NUM_,F11_,KAD_,KP3_,KSU_,KMU_,KP9_,SCR_,  0 }; /* 7 */
static const u8 au8ShiftedTable[128] = {
/*    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
      0 ,  0 ,F09_,F05_,F03_,F01_,F02_,F12_,  0 ,F10_,F08_,F06_,F04_,'\t', '~',  0 ,  /* 0 */
      0 ,LAL_,LSH_,  0 ,LCT_, 'Q', '!',  0 ,  0 ,  0 , 'Z', 'S', 'A', 'W', '@',  0 ,  /* 1 */
      0 , 'C', 'X', 'D', 'E', '$', '#',  0 ,  0 , ' ', 'V', 'F', 'T', 'R', '%',  0 ,  /* 2 */
      0 , 'N', 'B', 'H', 'G', 'Y', '^',  0 ,  0 ,  0 , 'M', 'J', 'U', '&', '*',  0 ,  /* 3 */
      0 , '<', 'K', 'I', 'O', ')', '(',  0 ,  0 , '>', '?', 'L', ':', 'P', '_',  0 ,  /* 4 */
      0 ,  0 ,'\"',  0 , '{', '+',  0 ,  0 ,CPS_,RSH_,ENT_, '}',  0 , '|',  0 ,  0 ,  /* 5 */
      0 ,  0 ,  0 ,  0 ,  0 ,  0 ,BKS_,  0 ,  0 ,KP1_,  0 ,KP4_,KP7_,  0 ,  0 ,  0 ,  /* 6 */
    KP0_,KPE_,KP2_,KP5_,KP6_,KP8_,ESC_,NUM_,F11_,KAD_,KP3_,KSU_,KMU_,KP9_,SCR_,  0 }; /* 7 */

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

/*
Outgoing Request for Resend:
  Will only be performed if packet corruption is detected by Handlers, not parser (KeyboardRunActiveState).
  Must be performed as soon as the current incomming packet finishes.

  The last state of Receive is responsible for immediately running Inhibit_TransmitState.
  Inhibit_TransmitState cannot be called from anywhere else for the purpose of transmitting a Tequest to Resend.

Request for Led update:
  The need for a Led update will be determined only in by the parser (KeyboardRunActiveState).
  First CMD byte should be triggered immediately at the:
    -conclusion of KeyboardRunActiveState (after all new inputs have been parsed) IF current state is Start_ReceiveState
    -at the conclusion of Receive cycle(from Stop_ReceiveState ).
  After CMD has been transmitted, driver needs to wait for an acknowledgement from the keyboard (0xFA). No matter what byte is received, the
  Clock line is pulled low immediately at the conclusion of Reception (Stop_ReceiveState) to prevent further keyboard transmission until the 
  byte has been parsed. Once the byte has been parsed:
    -IF 0xFA is received, the driver can proceed with transmitting the data byte as soon as KeyboardRunActiveState finishes or 100us has
      passed since Clock line was pulled low, whichever occurs later.
    -IF 0xFE is received, the driver can proceed with retransmitting the CMD byte as soon as KeyboardRunActiveState finishes or 100us has
      passed since Clock line was pulled low, whichever occurs later. The driver should only attempt to retransmit 3 times before aborting
      the operation, and releasing the clock line.
    -IF anything else is received, the operation is aborted, and the clock line is released once the byte has been parsed. The parser should 
      respond to this byte as if no transmission was in progress (ie, as a regular Scan Code).
  After Data has been transmitted, driver needs to again wait for an acknowledgement from the keyboard (0xFA).  In this case the clock line 
  is not held low.
    -IF 0xFE is received, once the byte is parsed:
      -IF driver is receiving a byte (not in Start_ReceiveState), the operation is aborted.
      -IF driver is not receiving a byte (in Start_ReceiveState), the driver immediately enters Inhibit_TransmitState. The Data byte is 
        subsequently recent up the three times before the operation is aborted.
    -IF 0xFA is received, the operation is concluded successfuly. 
    -IF anything else is received, the operation is aborted. The parser should respond to this byte as if no transmission was in progress 
      (ie, as a regular Scan Code).
*/

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
    
    if (LockUpdate == CMD_SENT)
    {
      if (u8DataPacket == ACK_BYTE)
      {
        u8TransmitPacket = u8Modifiers & 0x7;
        AT91C_BASE_NVIC->NVIC_ISER[0] = 1 << IRQn_TC0;                /* Enables Timer Counter 0 interrupts */
        return;
      }
      if (u8DataPacket == RESEND_BYTE)
      {
        u8TransmitPacket = 0xED;
        LockUpdate = CMD_PEND;
        AT91C_BASE_NVIC->NVIC_ISER[0] = 1 << IRQn_TC0;                /* Enables Timer Counter 0 interrupts */
        return;
      }
      LockUpdate = ABORT;
      AT91C_BASE_NVIC->NVIC_ISER[0] = 1 << IRQn_TC0;                /* Enables Timer Counter 0 interrupts */
    }
    else if (LockUpdate == DATA_SENT)
    {
      if (u8DataPacket == RESEND_BYTE)
      {
        LockUpdate = CMD_SENT;
        PS2Inhibit_TransmitState();
        return;
      }
      LockUpdate = NO_UPDATE;
      if (u8DataPacket == ACK_BYTE)
        continue;
    }
    
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
          ComputeUpdate();
          break;
        case 0x59:  /* R Shift */
          u8Modifiers &= ~MOD_RSHIFT;
          ComputeUpdate();
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
          ComputeUpdate();
          break;
        case RSH_:
          u8Modifiers |= MOD_RSHIFT;
          ComputeUpdate();
          break;
        case CPS_:
          u8Modifiers ^= MOD_CAPS_LOCK;
          LockUpdate = CMD_PEND;
          if (PS2_Handler == PS2Start_ReceiveState)
            PS2Inhibit_TransmitState();
          break;
        case NUM_:
          u8Modifiers ^= MOD_NUM_LOCK;
          LockUpdate = CMD_PEND;
          if (PS2_Handler == PS2Start_ReceiveState)
            PS2Inhibit_TransmitState();
          break;
        case SCR_:
          u8Modifiers ^= MOD_SCROLL_LOCK;
          LockUpdate = CMD_PEND;
          if (PS2_Handler == PS2Start_ReceiveState)
            PS2Inhibit_TransmitState();
          break;
        }
        if (character)
          PushChar(character);
        break;
      }
    }
  }
}

void PS2Start_ReceiveState(void)
{
  if (AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK)
    bDResend = TRUE;
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
    bDResend = TRUE;
  u8Parity = 0;
  PS2_Handler = PS2Stop_ReceiveState;
}
void PS2Stop_ReceiveState(void)
{
  if (!(AT91C_BASE_PIOA->PIO_PDSR & PS2_DATA_MSK) || (bDResend == TRUE))
  {
    bDResend = FALSE;
    u8TransmitPacket = 0xFE;
    PS2Inhibit_TransmitState();
  }
  else
  {
    u8PS2_RBufferPosition++;
    if (u8PS2_RBufferPosition == PS2_RBUFFER_SIZE)
      u8PS2_RBufferPosition = 0;
    
    if ((LockUpdate == CMD_PEND) || (LockUpdate == CMD_SENT))
      PS2Inhibit_TransmitState();
    else
      PS2_Handler = PS2Start_ReceiveState;
  }
}

void PS2Inhibit_TransmitState(void)
{
  AT91C_BASE_PIOA->PIO_IDR = PS2_CLOCK_MSK;                     // Disables interrupts on clock line
  PS2_Handler = PS2Empty_State;                                 // In case invalid clock edge was detected before interrupt was disabled */
  if (LockUpdate == CMD_PEND)
    u8TransmitPacket = 0xED;
  else if (LockUpdate == CMD_SENT)
    AT91C_BASE_NVIC->NVIC_ICER[0] = 1 << IRQn_TC0;              // Disables interrupt. Will be enabled once 0xFA reception is confirmed
  u8BitCount = 0;
  u8Parity = 0;
  AT91C_BASE_PIOA->PIO_CODR = PS2_CLOCK_MSK;                    // Pull Clock line low
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;                      // Starts 100us timer
}
void PS2Start_TransmitState(void)
{
  if (LockUpdate == ABORT)
  {
    LockUpdate = NO_UPDATE;
    PS2_Handler = PS2Start_ReceiveState;
  }
  else
  {
    PS2_Handler = PS2Data_TransmitState;
    AT91C_BASE_PIOA->PIO_CODR = PS2_DATA_MSK;                   /* Brings Data line low */
  }
  AT91C_BASE_PIOA->PIO_ISR;                                     /* Clears the pending Clock line interrupt request */
  AT91C_BASE_PIOA->PIO_IER = PS2_CLOCK_MSK;                     /* Enables interrupts on Clock line */
  AT91C_BASE_PIOA->PIO_SODR = PS2_CLOCK_MSK;                    /* Releases Clock line */
}
void PS2Data_TransmitState(void)
{
  u8 u8BitMsk = 1 << u8BitCount;
  //u8 u8Index = u8PS2_TBufferUnsent;
  //if (au8PS2_TBuffer[u8Index] & u8BitMsk)
  if (u8TransmitPacket & u8BitMsk)
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
  
  //u8PS2TransmitPacket = 0;                                              // Is this required?
  /*
  au8PS2_TBuffer[u8PS2_TBufferUnsent] = 0;
  u8PS2_TBufferUnsent++;
  if (u8PS2_TBufferUnsent == PS2_TBUFFER_SIZE)
    u8PS2_TBufferUnsent = 0;
  */
  
  //if (u8PS2_TBufferUnsent != u8PS2_TBufferPosition)
  //  PS2Inhibit_TransmitState();
  //else
  
  if (LockUpdate == CMD_PEND)
    LockUpdate = CMD_SENT;
  else if (LockUpdate == CMD_SENT)
    LockUpdate = DATA_SENT;
  PS2_Handler = PS2Start_ReceiveState;
}

void PS2Empty_State(void) {}

/* Private Functions */
static void PushChar(u8 c)
{
  au8Char_Buffer[u8Char_BufferPosition] = c;
  
  u8Char_BufferPosition++;
  if (u8Char_BufferPosition == CHAR_BUFFER_SIZE)
    u8Char_BufferPosition = 0;
}

static void ComputeUpdate()
{
  if ((u8Modifiers & MOD_LSHIFT) || (u8Modifiers & MOD_RSHIFT))
    u8Modifiers |= MOD_SHIFT;
  else
    u8Modifiers &= ~MOD_SHIFT;
}