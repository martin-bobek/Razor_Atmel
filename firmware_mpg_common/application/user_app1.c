/**********************************************************************************************************************
File: user_app1.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
 1. Copy both user_app1.c and user_app1.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app1" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app1.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserApp1Initialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserApp1RunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

extern AntSetupDataType G_stAntSetupData;

extern u32 G_u32AntApiCurrentDataTimeStamp;
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;
extern u8 G_au8AntApiCurrentData[ANT_APPLICATION_MESSAGE_BYTES];

/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */
static u32 UserApp1_u32Timeout;                      /* Timeout counter used across states */

static bool bSendRequest = FALSE;
static u8 strTMessageA[TEXT_MESSAGE_LENGTH + 1];
static u8 strTMessageB[TEXT_MESSAGE_LENGTH + 1];
static u8 *pTMessage = strTMessageA;

static u8 strRMessage[TEXT_MESSAGE_LENGTH + 1];
static bool bMsgAvailable = FALSE;

/*
static bool SendRequest = FALSE;
static u8 TextMessage[21] = "";

static u32 UserApp1_u32DataMsgCount = 0;
static u32 UserApp1_u32TickMsgCount = 0;
*/
static u8 u8LastState = 0xFF;
//static u8 au8TickMessage[] = "EVENT x\n\r";
//static u8 au8DataContent[] = "xxxxxxxxxxxxxxxx";
//static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//static u8 au8TestMessage[] = {0, 0, 0, 0, 0xA5, 0, 0, 0};
//bool bGotNewData;


/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function  : UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserApp1Initialize(void)
{
  MessageInitialize();
  LedOn(RED);
  
  G_stAntSetupData.AntChannel           = ANT_CHANNEL_USERAPP;
  G_stAntSetupData.AntSerialLo          = ANT_SERIAL_LO_USERAPP;
  G_stAntSetupData.AntSerialHi          = ANT_SERIAL_HI_USERAPP;
  G_stAntSetupData.AntDeviceType        = ANT_DEVICE_TYPE_USERAPP;
  G_stAntSetupData.AntTransmissionType  = ANT_TRANSMISSION_TYPE_USERAPP;
  G_stAntSetupData.AntChannelPeriodLo   = ANT_CHANNEL_PERIOD_LO_USERAPP;
  G_stAntSetupData.AntChannelPeriodHi   = ANT_CHANNEL_PERIOD_HI_USERAPP;
  G_stAntSetupData.AntFrequency         = ANT_FREQUENCY_USERAPP;
  G_stAntSetupData.AntTxPower           = ANT_TX_POWER_USERAPP;
  
  if ((AT91C_BASE_PIOA->PIO_PDSR & BUTTON0_MSK) != 0)
  {
    if(AntChannelConfig(ANT_SLAVE))
    {
      LedOff(RED);
      LedOn(YELLOW);
      UserApp1_StateMachine = UserApp1SM_SlaveIdle;
    }
    else
    {
      LedBlink(RED, LED_4HZ);
      UserApp1_StateMachine = UserApp1SM_FailedInit;
    }
  }
  else
  {
    if(AntChannelConfig(ANT_MASTER))
    {
      AntOpenChannel();
      LedOff(RED);
      LedOn(YELLOW);
      UserApp1_StateMachine = UserApp1SM_Master;
    }
    else
    {
      LedBlink(RED, LED_4HZ);
      UserApp1_StateMachine = UserApp1SM_FailedInit;
    } 
  }  
} /* end UserApp1Initialize() */

  
/*----------------------------------------------------------------------------------------------------------------------
Function UserApp1RunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserApp1RunActiveState(void)
{
  MessageService();
  UserApp1_StateMachine();

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static bool IsPunctuation(u8 u8Char)
{
  return !((u8Char >= 'a' && u8Char <= 'z') ||
           (u8Char >= 'A' && u8Char <= 'Z') ||
           (u8Char >= '0' && u8Char <= '9'));
}

static void MessageInitialize(void) 
{
  LCDCommand(LCD_CLEAR_CMD);
  LCDMessage(LINE1_START_ADDR, "        CHAT");
}

static void MessageService(void)
{
  static u8 strYou[] = "YOU: ";
  static u8 astrChatLines[CHAT_NUM_LINES][LCD_MAX_LINE_DISPLAY_SIZE + 1] = {"        CHAT"};
  static u8 au8FirstNonPunct[CHAT_NUM_LINES]; // 21 for space beyond end
  static u8 u8CurrentLine = CHAT_NUM_LINES - 1;
  static u8 u8MsgStart = CHAT_NUM_LINES - 1;
  static u8 u8LinesInit = 1;
  static u8 u8ScrollBack = 0;
  static u8 u8TCharNum = 0;
  static u8 u8Col = 0;
  static u8 cStr[2] = "";
  static bool bMsgBegin = TRUE;
  
  if (bMsgBegin)
  {
    LCDMessage(LINE2_START_ADDR, strYou);
    au8FirstNonPunct[u8CurrentLine] = 5;
    au8FirstNonPunct[(u8CurrentLine + 1) % CHAT_NUM_LINES] = 20;
    for ( ; u8Col < 5; u8Col++)
      astrChatLines[u8CurrentLine][u8Col] = strYou[u8Col];
    bMsgBegin = FALSE;
  }
  
  if (*cStr = KeyboardData())
  {
    if ((*cStr == UAR_) && (u8ScrollBack + 1 < u8LinesInit))
    {
      u8ScrollBack++;
      astrChatLines[u8CurrentLine][u8Col] = '\0';
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, astrChatLines[(u8CurrentLine + u8ScrollBack + 1) % CHAT_NUM_LINES]);
      LCDMessage(LINE2_START_ADDR, astrChatLines[(u8CurrentLine + u8ScrollBack) % CHAT_NUM_LINES]);
    }
    else if ((*cStr == DAR_) && (u8ScrollBack > 0))
    {
      u8ScrollBack--;
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, astrChatLines[(u8CurrentLine + u8ScrollBack + 1) % CHAT_NUM_LINES]);
      LCDMessage(LINE2_START_ADDR, astrChatLines[(u8CurrentLine + u8ScrollBack) % CHAT_NUM_LINES]);
    }
    else if ((*cStr == BKS_) && (u8ScrollBack == 0) &&  !(u8CurrentLine == u8MsgStart && u8Col == 5))
    {
      u8 u8PastLine = (u8CurrentLine + 1) % CHAT_NUM_LINES;
      
      if ((au8FirstNonPunct[u8PastLine] != 19 && u8Col + au8FirstNonPunct[u8PastLine] == 20) || (u8Col == 0))
      {        
        u8LinesInit--;
        LCDCommand(LCD_CLEAR_CMD);
        astrChatLines[u8PastLine][au8FirstNonPunct[u8PastLine]] = astrChatLines[u8CurrentLine][0];
        u8CurrentLine = u8PastLine;
        LCDMessage(LINE2_START_ADDR, astrChatLines[u8CurrentLine]);
        LCDMessage(LINE1_START_ADDR, astrChatLines[(u8CurrentLine + 1) % CHAT_NUM_LINES]);
        u8Col = 20;
      }
      if (au8FirstNonPunct[u8CurrentLine] != 21)
      {
        u8Col--;
        LCDMessage(LINE2_START_ADDR + u8Col, " ");
      }
      if (u8Col < au8FirstNonPunct[u8CurrentLine])
      {
        u8 i;
        for (i = u8Col; (i != 0) && !IsPunctuation(astrChatLines[u8CurrentLine][i - 1]); i--);
        au8FirstNonPunct[u8CurrentLine] = i;
          
      }
    }
    else if ((*cStr < ENT_) && (u8ScrollBack == 0))
    {
      if (*cStr == ' ')
      {
        if (u8Col == 20)
        {
          if (!IsPunctuation(astrChatLines[u8CurrentLine][19]))
          {
            au8FirstNonPunct[u8CurrentLine] = 21; // space beyond end
          }
          return;
        }
        if (u8Col == 0)
        {
          return;
        }
      }
      if (u8Col == 20)
      {
        LCDCommand(LCD_CLEAR_CMD);
        u8 u8PastLine = u8CurrentLine;
        if (u8CurrentLine == 0)
          u8CurrentLine = CHAT_NUM_LINES;
        u8CurrentLine--;
        u8Col = 0;
        if (au8FirstNonPunct[u8PastLine] != 0)
        {
          for (u8 i = au8FirstNonPunct[u8PastLine]; i < 20; i++, u8Col++)
            astrChatLines[u8CurrentLine][u8Col] = astrChatLines[u8PastLine][i];
          astrChatLines[u8PastLine][au8FirstNonPunct[u8PastLine]] = '\0';
          astrChatLines[u8CurrentLine][u8Col] = '\0';
          au8FirstNonPunct[u8CurrentLine] = 0;
          LCDMessage(LINE2_START_ADDR, astrChatLines[u8CurrentLine]);
        }
        LCDMessage(LINE1_START_ADDR, astrChatLines[u8PastLine]);
        if (u8LinesInit < 19)
          u8LinesInit++;
      }
      //pTMessage[u8TCharNum] = *cStr;
      astrChatLines[u8CurrentLine][u8Col] = *cStr;
      LCDMessage(LINE2_START_ADDR + u8Col, cStr);
      //u8TCharNum++;
      u8Col++;
      if (IsPunctuation(*cStr))
        au8FirstNonPunct[u8CurrentLine] = u8Col;
    }
  }
}

/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

static void UserApp1SM_SlaveIdle(void)
{
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    AntOpenChannel();
    LedOff(YELLOW);
    LedBlink(GREEN, LED_2HZ);
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_SlaveWaitChannelOpen;
  }
}   
static void UserApp1SM_SlaveWaitChannelOpen(void)
{
  if (AntRadioStatus() == ANT_OPEN)
  {
    LedOn(GREEN);
    UserApp1_StateMachine = UserApp1SM_SlaveChannelOpen;
  }
  if (IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE)) 
  {
    AntCloseChannel();
    LedOff(GREEN);
    LedOn(YELLOW);
    UserApp1_StateMachine = UserApp1SM_SlaveIdle;
  }
}
static void UserApp1SM_SlaveChannelOpen(void)
{
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    
    AntCloseChannel();
    u8LastState = 0xFF;
    LedOff(RED);
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_SlaveWaitChannelClose;
  }  
  if (AntRadioStatus() != ANT_OPEN)
  {
    LedBlink(GREEN, LED_2HZ);
    LedOff(RED);
    LedOff(BLUE);
    u8LastState = 0xFF;
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_SlaveWaitChannelClose;
  }
  if (AntReadData())
  {
    if (G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      static u8 strAsciiData[] = "XX XX XX XX XX XX XX XX\r\n";
      for (u8 i = 0; i < 8; i++)
      {
        strAsciiData[3 * i] = HexToASCIICharUpper(G_au8AntApiCurrentData[i] >> 4);
        strAsciiData[3*i+1] = HexToASCIICharUpper(G_au8AntApiCurrentData[i] & 0xF);
      }
      DebugPrintf(strAsciiData);
      
      AntParse();
    }
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      if (u8LastState != G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        u8LastState = G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX];
        //au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
        //DebugPrintf(au8TickMessage);
        
        switch (u8LastState) {
        case RESPONSE_NO_ERROR:
          LedOff(GREEN);
          LedOff(RED);
          LedOn(BLUE);
          break;
        case EVENT_RX_FAIL:
          LedOff(GREEN);
          LedOn(BLUE);
          LedOn(RED);
          break;
        case EVENT_RX_FAIL_GO_TO_SEARCH:
          LedOff(BLUE);
          LedOff(RED);
          LedOn(GREEN);
          break;
        case EVENT_RX_SEARCH_TIMEOUT:
          DebugPrintf("Search timeout\r\n");
          break;
        default:
          DebugPrintf("Unexpected Event\r\n");
          break;
        }
      }
      
      /*
      if (SendRequest == TRUE) {
        au8DataPacket[0] = u8PacketNumber;
        int i;
        for (i = 1; TextMessage[u8CharIndex] != '\0' && i < 8; i++, u8CharIndex++)
        {
          au8DataPacket[i] = TextMessage[u8CharIndex];
        }
        for (; i < 8; i++)
        {
          au8DataPacket[i] = 0;
        }
        u8PacketNumber++;
        if (TextMessage[u8CharIndex] == '\0')
        {
          u8CharIndex = 0;
          u8PacketNumber = 0;
          SendRequest = FALSE;
        }
      }
      
      AntQueueBroadcastMessage(au8DataPacket);
      */
    }
  }
}
static void UserApp1SM_SlaveWaitChannelClose(void)
{
  if (AntRadioStatus() == ANT_CLOSED)
  {
    LedOff(GREEN);
    LedOn(YELLOW);
    
    UserApp1_StateMachine = UserApp1SM_SlaveIdle;
  }
  
  if (IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE))
  {
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);
    
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}

void AntParse(void)
{
  static enum { IDLE, RECEIVING } ReceiveState = IDLE;
  static u8 u8CharIndex = 0;
  static u8 u8PacketNumber = 0;
  static u16 u16RSuccess = 0;
  
  u8 Code = G_au8AntApiCurrentData[0];
  
  if ((Code & 0xEF) == 0xEF) // ACK Code
    return;
  if (ReceiveState == IDLE) {
    if ((Code & 0xA0) == 0xA0) // Message Code or Last Packet code
      ReceiveState = RECEIVING;
  }
  if (ReceiveState == RECEIVING) {
    u8 u8PNumDiff = (G_au8AntApiCurrentData[0] & 0xF) - u8PacketNumber;
    if (u8PNumDiff == 0)
      u16RSuccess |= 1 << u8PacketNumber;
    else {
      u8CharIndex += 7 * u8PNumDiff;
      u8PacketNumber += u8PNumDiff;
    }
    for (u8 i = 1; i < 8; i++, u8CharIndex++)
      strRMessage[u8CharIndex] = G_au8AntApiCurrentData[i];
    if ((Code & 0xE0) == 0xA0) { // Last Packet Code
      ReceiveState = IDLE;
      strRMessage[u8CharIndex] = '\0';
      bMsgAvailable = TRUE;
      u8PacketNumber = 0;
      u8CharIndex = 0;
    }
    else
      u8PacketNumber++;
  }
}

/* 
The protocol should be able to handle a Slave Handshake at any point.
Syncing:
  Once a Slave detects a Master, it sends out a Slave handshake repeatedly every 1 seconds until it receives the Master Handshake. The
    Master has 1/2 seconds to respond after seeing the Slave Handshake.
  Once the Slave receives the Master Handshake, it stops transmitting. If Master stops seeing the Slave Handshake, it assumes the 
    devices are paired successfully.

Message:
  Simply transmits packets 1 by 1. 
  After end packet, receiver has 1/2 seconds to respond with an acknowledgement or resend request. The sender continues sending Ack
    request every 1 seconds until it receives a reply from the receiver.


    Last 7 bytes contain the characters (unless the message ends before the end of the packet in which case they contain 0's.
    First byte acts as the MSG Code;
    
  Byte 1:
Message packet 1:     111x nnnn
  where x = (Message Number) % 2, and nnnn is the packet number in the message (0 to 15) with 
Last Packet:          101x nnnn
  where x and n are as before
Resend packets/Ack:   100x nnnn
  where x is a mirror of senders x, and nnnn is the number of bytes to resend. If nnnn is zero, this is an acknowledgement
  Remaining bytes: pppp mmmm, where pppp and nnnn are the packet numbers of the packets to be resent.
Resend remaining:     110x 0000   0000 nnnn   
  Resends all packets after nnnn up to the last packet. Remaining bytes are 0
Resend Entire:        110x 0001
  Remaining bytes are 0;
Ack request:          110x 0010
Handshake:            1100 0011 
  with next three bytes containing the Chars of the name code. Remaining bytes are zero.
Nothing (master):     0000 0000
  where all remaining bytes are also 0.
Slave Handshake:
Master Handshake:
*/

static void UserApp1SM_Master(void)
{ 
  static u8 u8PacketNumber = 0;
  static u8 u8CharIndex = 0;
  static bool bWaitingAck = FALSE;
  static u8 bMessageEO= MSG_EO_BIT;
  
  if(AntReadData())
  {
    u8 au8DataPacket[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    if (G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      if (bWaitingAck && (G_au8AntApiCurrentData[0] == (ACK_CODE | bMessageEO)))
      {
        bMessageEO ^= MSG_EO_BIT;
        bWaitingAck = FALSE;
        bSendRequest = FALSE;
      }
    }
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      if (bSendRequest && !bWaitingAck)
      {
        au8DataPacket[0] = bMessageEO | (u8PacketNumber & PACKET_NUM_MSK);
        u8 i;
        for(i = 1; ; u8CharIndex++, i++)
        {
          if (pTMessage[u8CharIndex] == '\0')
          {
            u8PacketNumber = 0;
            u8CharIndex = 0;
            for (; i < PACKET_LENGTH; i++)
              au8DataPacket[i] = 0;
            au8DataPacket[0] |= END_CODE;
            bWaitingAck = TRUE;
            break;
          }
          if (i >= PACKET_LENGTH)
          {
            u8PacketNumber++;
            au8DataPacket[0] |= MSG_CODE;
            break;
          }
          au8DataPacket[i] = pTMessage[u8CharIndex];
        }
      }
      AntQueueBroadcastMessage(au8DataPacket);
    }
  }
}
/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserApp1SM_Error(void)          
{
  
} /* end UserApp1SM_Error() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* State to sit in if init failed */
static void UserApp1SM_FailedInit(void)          
{
    
} /* end UserApp1SM_FailedInit() */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
