/**********************************************************************************************************************
File: user_app.c                                                                

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
This is a user_app.c file template 

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
volatile u32 G_u32UserAppFlags;                       /* Global state flags */


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
static fnCode_type ANT_StateMachine;
static u32 UserApp_u32Timeout;                            /* Timeout counter used across states */
static fnCode_type Game_StateMachine;                     /* Game state machine function pointer */

static u8 strTMessageA[TEXT_MESSAGE_LENGTH + 1];
static u8 strTMessageB[TEXT_MESSAGE_LENGTH + 1];
static u8 *pTMsgChat = strTMessageA;                      
static u8 *pTMsgAnt = strTMessageB;

static u8 au8FirstNonPunct[CHAT_NUM_LINES]; // 21 for space beyond end
static u8 astrChatLines[CHAT_NUM_LINES][LCD_MAX_LINE_DISPLAY_SIZE + 1] = {"        CHAT"};
static u8 u8CurrentLine = CHAT_NUM_LINES - 1;
static u8 u8MsgStart = CHAT_NUM_LINES - 1;
static u8 u8IncomingStart;
static u8 u8LinesInit = 0;
static u8 u8REOBit;                                       // Even/Odd bit received udring last message
static u8 u8TEOBit;                                       // Even/Odd bit transmitted in current message

static bool bSendRequest = FALSE;                         // pTMsgAnt is valid and ready to be sent
static bool bRAck = FALSE;                                // Received message was valid so acknowledge
static bool bRResend = FALSE;                             // Send a request resend since received message was invalid
static bool bTResend = FALSE;                             // Resend last transmitted message
static bool bWaitingAck = FALSE;
static u16 u16AckTimeout;

static u8 strRMessage[TEXT_MESSAGE_LENGTH + 1];
static bool bMsgAvailable = FALSE;

/*
static bool SendRequest = FALSE;
static u8 TextMessage[21] = "";

static u32 UserApp1_u32DataMsgCount = 0;
static u32 UserApp1_u32TickMsgCount = 0;
*/
static u8 u8LastState = 0xFF;

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
void UserAppInitialize(void)
{
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
  
  Game_StateMachine = Game_MainMenu;
  if ((AT91C_BASE_PIOA->PIO_PDSR & BUTTON0_MSK) != 0)
  {
    if(AntChannelConfig(ANT_SLAVE))
    {
      LedOff(RED);
      LedOn(YELLOW);
      ANT_StateMachine = ANT_SlaveIdle;
    }
    else
    {
      LedBlink(RED, LED_4HZ);
      ANT_StateMachine = ANT_FailedInit;
    }
  }
  else
  {
    if(AntChannelConfig(ANT_MASTER))
    {
      AntOpenChannel();
      LedOff(RED);
      LedOn(YELLOW);
      ANT_StateMachine = ANT_Master;
    }
    else
    {
      LedBlink(RED, LED_4HZ);
      ANT_StateMachine = ANT_FailedInit;
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
void UserAppRunActiveState(void)
{
  Game_StateMachine();
  if (bMsgAvailable)
  {
    ServiceIncoming();
  }
  ANT_StateMachine();
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
static void ChatInitialize(void) 
{
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "        CHAT");
    while (KeyboardData());
}
/* need to properly handle au8FirstNonPunct, etc... */
static void ServiceIncoming(void)
{
  static u8 strOpp[] = "OPP: ";
  static u8 strYou[] = "YOU: ";
  
  static u8 u8PrevCurrentLine;
  static u8 u8RLineNum;
  static u8 u8RCharNum;
  static bool bYouRestore;
  static bool bFirstEntry = TRUE;
  static u8 u8Index;
  
  if (bFirstEntry)
  {
    u8RLineNum = u8MsgStart;      // used to record message in astrChatLines
    u8RCharNum = 0;
    bYouRestore = FALSE;
    bFirstEntry = FALSE;
    u8IncomingStart = u8MsgStart; // used to print message
    u8PrevCurrentLine = u8CurrentLine;  // used to transfer u8FirstNonPunct
    for (u8Index = 0; u8Index < 5; u8Index++)
    {
      astrChatLines[u8RLineNum][u8Index] = strOpp[u8Index];
    }
  }
  if (!bYouRestore && u8LinesInit < 19)
  {
    u8LinesInit++;
  }
  for (; ; u8RCharNum++, u8Index++)
  {
    u8 u8Char;
    u8Char = bYouRestore ? pTMsgChat[u8RCharNum] : strRMessage[u8RCharNum];
    if (u8Char == '\n')
    {
      astrChatLines[u8RLineNum][u8Index] = '\0';
      if (u8RLineNum == 0)
        u8RLineNum = CHAT_NUM_LINES;
      u8RLineNum--;
      /*if (bYouRestore)
      {
        if (u8CurrentLine == 0)
          u8CurrentLine = CHAT_NUM_LINES;
        u8CurrentLine--;
        au8FirstNonPunct[u8RLineNum] = au8FirstNonPunct[u8CurrentLine];
      } */
      u8Index = 0;
      u8RCharNum++;
      break;
    }
    if (u8Char == '\0')
    {
      astrChatLines[u8RLineNum][u8Index] = '\0';
      if (bYouRestore == FALSE)
      {
        bYouRestore = TRUE;
        u8RCharNum = 0;
        if (u8RLineNum == 0)
          u8RLineNum = CHAT_NUM_LINES;
        u8RLineNum--;
        u8MsgStart = u8RLineNum;
        //au8FirstNonPunct[u8RLineNum] = au8FirstNonPunct[u8CurrentLine];
        for (u8Index = 0; u8Index < 5; u8Index++)
        {
          astrChatLines[u8RLineNum][u8Index] = strYou[u8Index];
        }
      }
      else
      {
        u8CurrentLine = u8RLineNum;
        au8FirstNonPunct[u8RLineNum] = au8FirstNonPunct[u8PrevCurrentLine];
        while (u8RLineNum != u8MsgStart) 
        {
          u8RLineNum++;
          if (u8RLineNum == CHAT_NUM_LINES)
            u8RLineNum = 0;
          u8PrevCurrentLine++;
          if (u8PrevCurrentLine == CHAT_NUM_LINES)
            u8PrevCurrentLine = 0;
          au8FirstNonPunct[u8RLineNum] = au8FirstNonPunct[u8PrevCurrentLine];
        } 
        u8RLineNum++;
        if (u8RLineNum == CHAT_NUM_LINES)
          u8RLineNum = 0;
        au8FirstNonPunct[u8RLineNum] = 20;
        bFirstEntry = TRUE;
        bMsgAvailable = FALSE;
      }
      break;
    }
    astrChatLines[u8RLineNum][u8Index] = u8Char;
  }
}
/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/
static void Game_MainMenu(void)
{
  static u8 strControls[] = "               ENTER";
  static u8 strChat[]     = "        CHAT        ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    LCDMessage(LINE1_START_ADDR, strChat);
    LCDMessage(LINE2_START_ADDR, strControls);
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON3))        /* Go to StartScreen state of the selected games */
  {
    ChatInitialize();
    Game_StateMachine = Game_Chat;
    bFirstEntry = TRUE;
  }
}
static void Game_Chat(void)
{  
  static u8 strYou[] = "YOU: ";
  static u8 u8ScrollBack = 0;
  static u8 u8TCharNum = 0;
  static u8 u8Col = 0;
  static u8 cStr[2] = "";
  static bool bMsgBegin = TRUE;
  
  if (bMsgAvailable)
  {
    Game_StateMachine = Game_PrintIncoming;
    astrChatLines[u8CurrentLine][u8Col] = '\0';
    pTMsgChat[u8TCharNum] = '\0';
    u8ScrollBack = 0;
    return;
  }
  if (bMsgBegin)
  {
    if (u8LinesInit < 19)
      u8LinesInit++;
    LCDMessage(LINE2_START_ADDR, strYou);
    au8FirstNonPunct[u8CurrentLine] = 5;
    au8FirstNonPunct[(u8CurrentLine + 1) % CHAT_NUM_LINES] = 20;
    for (u8Col = 0; u8Col < 5; u8Col++)
      astrChatLines[u8CurrentLine][u8Col] = strYou[u8Col];
    bMsgBegin = FALSE;
  }
  
  if (*cStr = KeyboardData())
  {
    if ((*cStr == ENT_) && (u8TCharNum > 0) && !bSendRequest)
    {
      bSendRequest = TRUE;
      pTMsgChat[u8TCharNum] = '\0';
      u8TCharNum = 0;
      pTMsgAnt = pTMsgChat;
      if (pTMsgChat == strTMessageA)
      {
        pTMsgChat = strTMessageB;
      }
      else
      {
        pTMsgChat = strTMessageA;
      }
      astrChatLines[u8CurrentLine][u8Col] = '\0';
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, astrChatLines[u8CurrentLine]);
      if (u8CurrentLine == 0)
        u8CurrentLine = CHAT_NUM_LINES;
      u8CurrentLine--;
      u8MsgStart = u8CurrentLine;
      bMsgBegin = TRUE;
    }
    else if ((*cStr == UAR_) && (u8ScrollBack + 1 < u8LinesInit))
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
        u8 i;
        for (i = 20 - au8FirstNonPunct[u8PastLine]; i > 0; i--)
        {
          pTMsgChat[u8TCharNum - i - 1] = pTMsgChat[u8TCharNum - i];
        }
        u8TCharNum--;
        int j;
        for (j = 0, i = au8FirstNonPunct[u8PastLine]; j + 1 < u8Col; j++, i++)
        {
          astrChatLines[u8PastLine][i] = astrChatLines[u8CurrentLine][j];
        }
        astrChatLines[u8PastLine][i] = '\0';
        u8CurrentLine = u8PastLine;
        LCDMessage(LINE2_START_ADDR, astrChatLines[u8CurrentLine]);
        LCDMessage(LINE1_START_ADDR, astrChatLines[(u8CurrentLine + 1) % CHAT_NUM_LINES]);
        u8Col = 20;
      }
      if (au8FirstNonPunct[u8CurrentLine] != 21)
      {
        u8Col--;
        u8TCharNum--;
        LCDMessage(LINE2_START_ADDR + u8Col, " ");
      }
      if (u8Col < au8FirstNonPunct[u8CurrentLine])
      {
        u8 i;
        for (i = u8Col; (i != 0) && !IsPunctuation(astrChatLines[u8CurrentLine][i - 1]); i--);
        au8FirstNonPunct[u8CurrentLine] = i;
      }
    }
    else if ((*cStr < ENT_) && (u8ScrollBack == 0) && (u8TCharNum + 1 < TEXT_MESSAGE_LENGTH))
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
        u8 u8PastLine = u8CurrentLine;      // remember currentLine
        if (u8CurrentLine == 0)
          u8CurrentLine = CHAT_NUM_LINES;
        u8CurrentLine--;                    // update current line
        u8Col = 0;                          // send cursor back to start of line
        u8 index = 0;
        if (au8FirstNonPunct[u8PastLine] != 0)        // if long word did not begin at start of previous line, push it to next line
        {
          for (u8 i = au8FirstNonPunct[u8PastLine]; i < 20; i++, u8Col++)         // copy beginning of word to next line             
          {
            astrChatLines[u8CurrentLine][u8Col] = astrChatLines[u8PastLine][i];
          }
          for (; 20 - index > au8FirstNonPunct[u8PastLine]; index++)
          {
            pTMsgChat[u8TCharNum - index] = pTMsgChat[u8TCharNum - index - 1];
          }
          if (au8FirstNonPunct[u8PastLine] <= 20)
            astrChatLines[u8PastLine][au8FirstNonPunct[u8PastLine]] = '\0';         // terminate the last line
          astrChatLines[u8CurrentLine][u8Col] = '\0';                             // terminate the current line
          LCDMessage(LINE2_START_ADDR, astrChatLines[u8CurrentLine]);
        }
        au8FirstNonPunct[u8CurrentLine] = 0;                                    // beginning of word must be start of current line
        pTMsgChat[u8TCharNum - index] = '\n';
        u8TCharNum++;
        LCDMessage(LINE1_START_ADDR, astrChatLines[u8PastLine]);
        if (u8LinesInit < 19)
          u8LinesInit++;
      }
      astrChatLines[u8CurrentLine][u8Col] = *cStr;
      LCDMessage(LINE2_START_ADDR + u8Col, cStr);
      pTMsgChat[u8TCharNum] = *cStr;
      u8TCharNum++;
      u8Col++;
      if (IsPunctuation(*cStr))
        au8FirstNonPunct[u8CurrentLine] = u8Col;
    }
  }
}
static void Game_PrintIncoming(void)
{
  static bool bFirstEntry = TRUE;
  static u32 u32Counter;
  
  if (bFirstEntry)
  {
    u32Counter = 100;
    LedOff(LCD_BLUE);
    LedOff(LCD_RED);
    bFirstEntry = FALSE;
  }
  
  u32Counter--;
  if (u32Counter == 0)
  {
    if (u8IncomingStart == u8CurrentLine)
    {
      Game_StateMachine = Game_Chat;
      LedOn(LCD_BLUE);
      LedOn(LCD_RED);
      bFirstEntry = TRUE; 
      return;
    }
    u32Counter = 250;
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, astrChatLines[u8IncomingStart]);
    if (u8IncomingStart == 0)
      u8IncomingStart = CHAT_NUM_LINES;
    u8IncomingStart--;
    LCDMessage(LINE2_START_ADDR, astrChatLines[u8IncomingStart]);
  }
}

static void ANT_SlaveIdle(void)
{
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    AntOpenChannel();
    LedOff(YELLOW);
    LedBlink(GREEN, LED_2HZ);
    UserApp_u32Timeout = G_u32SystemTime1ms;
    ANT_StateMachine = ANT_SlaveWaitChannelOpen;
  }
}   
static void ANT_SlaveWaitChannelOpen(void)
{
  if (AntRadioStatus() == ANT_OPEN)
  {
    LedOn(GREEN);
    ANT_StateMachine = ANT_SlaveChannelOpen;
  }
  if (IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE)) 
  {
    AntCloseChannel();
    LedOff(GREEN);
    LedOn(YELLOW);
    ANT_StateMachine = ANT_SlaveIdle;
  }
}
static void ANT_SlaveChannelOpen(void)
{
  if (bWaitingAck && u16AckTimeout)
  {
    u16AckTimeout--;
  }
  
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    
    AntCloseChannel();
    u8LastState = 0xFF;
    LedOff(RED);
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
    
    UserApp_u32Timeout = G_u32SystemTime1ms;
    ANT_StateMachine = ANT_SlaveWaitChannelClose;
  }  
  if (AntRadioStatus() != ANT_OPEN)
  {
    LedBlink(GREEN, LED_2HZ);
    LedOff(RED);
    LedOff(BLUE);
    u8LastState = 0xFF;
    
    UserApp_u32Timeout = G_u32SystemTime1ms;
    ANT_StateMachine = ANT_SlaveWaitChannelClose;
  }
  if (AntReadData())
  {
    if (G_eAntApiCurrentMessageClass == ANT_DATA)
    { 
      AntDecode();
    }
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      if (G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX] == RESPONSE_NO_ERROR)
      {
        u8 au8DataPacket[ANT_DATA_BYTES] = {0};
        AntGeneratePacket(au8DataPacket);
        if (au8DataPacket[0] != 0)
        {
          AntQueueBroadcastMessage(au8DataPacket);
        }
      }
      if (u8LastState != G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        u8LastState = G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX];
        
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
    }
  }
}
static void ANT_SlaveWaitChannelClose(void)
{
  if (AntRadioStatus() == ANT_CLOSED)
  {
    LedOff(GREEN);
    LedOn(YELLOW);
    
    ANT_StateMachine = ANT_SlaveIdle;
  }
  
  if (IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE))
  {
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);
    
    ANT_StateMachine = ANT_Error;
  }
}

static void ANT_Master(void)
{
  if (bWaitingAck && u16AckTimeout)
  {
    u16AckTimeout--;
  }
  
  if(AntReadData())
  {
    if (G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      AntDecode();
    }
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      u8 au8DataPacket[ANT_DATA_BYTES] = {0};
      AntGeneratePacket(au8DataPacket);
      AntQueueBroadcastMessage(au8DataPacket);
    }
  }
}

static void AntGeneratePacket(u8 *pDataPacket)
{
  static u8 u8CyanTimer = 0;
  static u8 u8WhiteOn = 0;
  static u8 u8PurpleOn = 0;
  if (u8CyanTimer > 0)
  {
    u8CyanTimer--;
    if(u8CyanTimer == 0)
      LedOff(CYAN);
  }
  if (u8WhiteOn > 0)
  {
    u8WhiteOn--;
    if (u8WhiteOn == 0)
      LedOff(WHITE);
  }
  if (u8PurpleOn > 0)
  {
    u8PurpleOn--;
    if (u8PurpleOn == 0)
      LedOff(PURPLE);
  }
  
  static u8 u8PacketNumber = 0;
  static u8 u8CharIndex = 0;
  if (bRResend)
  {
    LedOn(PURPLE);
    u8PurpleOn = 3;
    pDataPacket[0] = RSD_CODE | u8REOBit;
    bRResend = FALSE;
    return;
  }
  if (bRAck)
  {
    LedOn(WHITE);
    u8WhiteOn = 3;
    pDataPacket[0] = ACK_CODE | u8REOBit;
    bRAck = FALSE;
    return;
  }
  if (bWaitingAck && u16AckTimeout == 0)
  {
    pDataPacket[0] = RQT_CODE | u8TEOBit;
    LedOn(CYAN);
    u8CyanTimer = 3;
    u16AckTimeout = ACK_TIMEOUT;
    return;
  }
  if (bTResend)
  {
    bTResend = FALSE;
    u8CharIndex = 0;
    u8PacketNumber = 0;
  }
  if (bSendRequest && !bWaitingAck)
  {
    pDataPacket[0] = u8TEOBit | (u8PacketNumber & PACKET_NUM_MSK);
    u8 i;
    for(i = 1; ; u8CharIndex++, i++)
    {
      if (pTMsgAnt[u8CharIndex] == '\0')
      {
        u8PacketNumber = 0;
        u8CharIndex = 0;
        for (; i < PACKET_LENGTH; i++)
          pDataPacket[i] = 0;
        pDataPacket[0] |= END_CODE;
        bWaitingAck = TRUE;
        u16AckTimeout = ACK_TIMEOUT;
        break;
      }
      if (i >= PACKET_LENGTH)
      {
        u8PacketNumber++;
        pDataPacket[0] |= MSG_CODE;
        break;
      }
      pDataPacket[i] = pTMsgAnt[u8CharIndex];
    }
  }
}
void AntDecode(void)
{
  static enum { IDLE, RECEIVING } DecodeState = IDLE;
  static u8 u8CharIndex = 0;
  static u8 u8PacketNumber = 0;
  static bool bRSuccess = FALSE;
  
  u8 u8Code = G_au8AntApiCurrentData[CODE_BYTE];
  
  if (bWaitingAck)
  {
    if (u8Code == (RSD_CODE | u8TEOBit))
    {
      bTResend = TRUE;
      bWaitingAck = FALSE;
      return;
    }
    if (u8Code == (ACK_CODE | u8TEOBit))  // packet is an acknowledgement and Transmit EO Bit matches last sent packet
    {
      bWaitingAck = FALSE;
      bSendRequest = FALSE;     // permits sending of next packet
      u8TEOBit ^= MSG_EO_BIT;   // toggles Transmit EO Bit
      return;
    }
  }
  if ((u8Code & 0xEF) == RQT_CODE) // sender did not receive ack or resend code and is asking for it to be sent
  {
    if (bRSuccess && (u8Code & u8REOBit) == MSG_EO_BIT) // last packet was received successfuly and Receive EO Bit matches request packet
    {
      bRAck = TRUE; // send acknowledgement
    }
    else
    {
      bRResend = TRUE;
      u8REOBit = u8Code & MSG_EO_BIT;  // Update Receive EO Bit in case packet was missed
      DecodeState = IDLE;
    }
    return;
  }
  if (DecodeState == IDLE)
  {
    if ((u8Code & 0xAF) == 0xA0) // is a MSG or END packet with packet number 0 (first packet)
    {
      DecodeState = RECEIVING;
      u8CharIndex = 0;          // receive receiving mechanism and prepair to decode packet
      u8PacketNumber = 0;
      bRSuccess = FALSE;              // resets successful reception flag
      u8REOBit = u8Code & MSG_EO_BIT; // The Receive EO is set to that of the first packet
    }
    else
    {
      // maybe do nothing here? not sure
    }
  }
  if (DecodeState == RECEIVING)
  {
    if (((u8Code & 0xB0) != (0xA0 | u8REOBit)) || ((u8Code & 0xF) != u8PacketNumber))
    { // invalid code and Receive EO Bit or invalid packet number (must have missed one) or too many packets. Either way, ask for resend
      bRResend = TRUE; // ask for resend
      DecodeState = IDLE;  // and await packet restart
      return;
    }
    
    for (u8 u8ByteNum = 1; u8ByteNum < ANT_DATA_BYTES; u8ByteNum++, u8CharIndex++)
    {
      strRMessage[u8CharIndex] = G_au8AntApiCurrentData[u8ByteNum];
    }
    if ((u8Code & 0xE0) == 0xA0) // Last Packet Code
    {
      DecodeState = IDLE;        // decoder is awaiting the start of the next packet
      bRSuccess = TRUE;           // sets successful reception flag
      strRMessage[u8CharIndex] = '\0';
      bMsgAvailable = TRUE;
      bRAck = TRUE;
    }
    else
    {
      u8PacketNumber++;
    }
  }
  
  
  /*
  static enum { IDLE, RECEIVING, ERROR } ReceiveState = IDLE;
  static u8 u8CharIndex = 0;
  static u8 u8PacketNumber = 0;
  static bool bRSuccess = FALSE;
  
  u8 Code = G_au8AntApiCurrentData[0];
  
  if (bWaitingAck)
  {
    if (G_au8AntApiCurrentData[0] == (RSD_CODE | u8TEOBit))
    {
      bTResend = TRUE;
      bWaitingAck = FALSE;
      return;
    }
    if (G_au8AntApiCurrentData[0] == (ACK_CODE | u8TEOBit))
    {
      u8TEOBit ^= MSG_EO_BIT;
      bWaitingAck = FALSE;
      bSendRequest = FALSE;
      return;
    }
  }
  if (ReceiveState == ERROR)
  {
    if (!bRResend)
    {
      ReceiveState = IDLE;
    }
    else
    {
      return;
    }
  }
  if ((Code & 0xEF) == RQT_CODE) // ACK Request
  {
    u8CharIndex = 0;
    if (bRSuccess && (Code & MSG_EO_BIT) == u8REOBit)
    {
      bRAck = TRUE;
    }
    else
    {
      u8REOBit = Code & MSG_EO_BIT;
      bRResend = TRUE;
    }
    return;
  }
  if (ReceiveState == IDLE)
  {
    if ((Code & 0xA0) == 0xA0) // Message Code or Last Packet code
    {
      ReceiveState = RECEIVING;
      bRSuccess = FALSE;
      u8REOBit = Code & MSG_EO_BIT;
    }
  }
  if (ReceiveState == RECEIVING)
  {
    if ((G_au8AntApiCurrentData[0] & 0xF) != u8PacketNumber)
    {
      ReceiveState = ERROR;
      bRResend = TRUE;
      return;
    }
    for (u8 i = 1; i < 8; i++, u8CharIndex++)
      strRMessage[u8CharIndex] = G_au8AntApiCurrentData[i];
    if ((Code & 0xE0) == 0xA0) { // Last Packet Code
      ReceiveState = IDLE;
      bRSuccess = TRUE;
      strRMessage[u8CharIndex] = '\0';
      bMsgAvailable = TRUE;
      u8PacketNumber = 0;
      u8CharIndex = 0;
      bRAck = TRUE;
    }
    else
      u8PacketNumber++;
  }
  */
}

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void ANT_Error(void)          
{
  
} /* end UserApp1SM_Error() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* State to sit in if init failed */
static void ANT_FailedInit(void)          
{
    
} /* end UserApp1SM_FailedInit() */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
