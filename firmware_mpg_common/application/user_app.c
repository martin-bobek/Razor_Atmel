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

static bool bSound_Active;                                /* Indicates a timed sound is being played */
static u16 u16Sound_Duration;                             /* Time in ms remaining before sound should be turned off */

static Game_Type CurrentGame = RUNNER;                    /* Used for modifying behaviour of general purpose functions */
static u16 u16LFSR;                                       /* Linear Feedback Shift Register for randInt */
static bool bLFSRinitialized = FALSE;                     /* Indicates LFSR has not been initialized with seed */
static u32 u32TickLength;                                 /* Initial game tick length */
static u32 u32Score = 0;                                  /* Number of gameticks the player has survived */
static u32 u32HighScore[3] = { 0, 0, 0 };                 /* Contains the high scores for each game type */

static s8 s8Frogger_Position;                             /* Horizontal position of character on the screen (0 at left) */
static FroggerLine_Type *aFrogger_Lines[2];               /* Pointers to the two lines currently displayed on the screen */
static FroggerLine_Type Frogger_LineA, Frogger_LineB;     /* Structure containing information about the line of logs */
static u8 au8Frogger_strLineA[21];                        /* String owned by Frogger_LineA structure */
static u8 au8Frogger_strLineB[21];                        /* String owned by Frogger_LineB structure */
static u32 u32Frogger_Counter;                            /* Used for counting ms's for each game tick */
static u8 u8Frogger_Forward = 0;                          /* Indicates character has moved forward number or game ticks before screen moves forward */
static bool bFrogger_ScreenUpdate;                        /* Indicates that contents of screen have changed and LCD needs to be updated */

static u8 au8Memory_Sequence[125];                        /* Stores pattern (up to 500 values) with two bits per value */
static u32 u32Memory_SequenceLength;                      /* Length of the current pattern */
static u32 u32Memory_SequencePosition;                    /* Position in the pattern for input and output */
static u32 u32Memory_PauseLength;                         /* Length of the pause during output between individual values */

static u8 au8Runner_Cactuses[21];                         /* Characters for the bottom row of LCD */
static u8 u8Runner_SequenceLength;                        /* Remaining length of sequence of cactuses/spaces */
static RunnerSequence_Type Runner_SequenceType;           /* True indicates current sequence is cactuses */
static bool bRunner_ScreenUpdate;                         /* Indicates that contents of screen have changed and LCD needs to be updated */
static u8 u8Runner_Jump;                                  /* Indicates the character has jumped and length of jump remaining */


static u8 strTMessageA[TEXT_MESSAGE_LENGTH + 1];
static u8 strTMessageB[TEXT_MESSAGE_LENGTH + 1];
static u8 *pTMsgChat = strTMessageA;                      
static u8 *pTMsgAnt = strTMessageB;

static u8 au8FirstNonPunct[CHAT_NUM_LINES]; // 21 for space beyond end
static u8 astrChatLines[CHAT_NUM_LINES][LCD_MAX_LINE_DISPLAY_SIZE + 1] = {"        CHAT     ESC"};
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

#define STICKMAN      "\x01"
#define SOLID         "\x02"
#define LOG           "\x03"
#define UP            "\x04"

CustomChar_t CustomChars[] = {
  { 0x01, {0x0E, 0x0E, 0x04, 0x1F, 0x04, 0x04, 0x0A, 0x11} },   // Stickman
  { 0x02, {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F} },   // Solid Block
  { 0x03, {0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00} },    // Log
  { 0x04, {0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x00, 0x00} },    // Up
};

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
      AntOpenChannel();
      UserApp_u32Timeout = G_u32SystemTime1ms;
      ANT_StateMachine = ANT_SlaveWaitChannelOpen;
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
      ANT_StateMachine = ANT_Master;
    }
    else
    {
      LedBlink(RED, LED_4HZ);
      ANT_StateMachine = ANT_FailedInit;
    } 
  } 
  
  LCDCharSetup(CustomChars, sizeof(CustomChars)/sizeof(CustomChar_t));
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
  Sound_Service();
} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static bool IsPunctuation(u8 u8Char)
{
  return !((u8Char >= 'a' && u8Char <= 'z') ||
           (u8Char >= 'A' && u8Char <= 'Z') ||
           (u8Char >= '0' && u8Char <= '9') ||
           (u8Char == '\''));
}
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
static void Sound_Service(void)
{
  if (bSound_Active == FALSE)
    return;
  
  if (u16Sound_Duration == 0)
  {
    PWMAudioOff(BUZZER1);
    
    bSound_Active = FALSE;
    return;
  }
  u16Sound_Duration--;
}
/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/
static void Game_MainMenu(void)
{
  static u8 strControls[] = "ENTER  \x7F    \x7E  SCORE";
  static u8 strCtrlsChat[]= "ENTER  \x7F    \x7E       ";
  static u8 strRunner[]   = "       RUNNER       ";
  static u8 strFrogger[]  = "      FROGGER       ";
  static u8 strMemory[]   = "    MEMORY GAME     ";
  static u8 strChat[]     = "        CHAT        ";
  static bool bFirstEntry = TRUE;
  
  //static u8 strControls[] = "               ENTER";
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    LCDCommand(LCD_CLEAR_CMD);
    
    if (CurrentGame == RUNNER)
    {
      LCDMessage(LINE1_START_ADDR, strRunner);
    }
    else if (CurrentGame == FROGGER)
    {
      LCDMessage(LINE1_START_ADDR, strFrogger);
    }
    else if (CurrentGame == MEMORY)
    {
      LCDMessage(LINE1_START_ADDR, strMemory);
    }
    else 
    {
      LCDMessage(LINE1_START_ADDR, strChat);
    }
    LCDMessage(LINE2_START_ADDR, (CurrentGame == CHAT) ? strCtrlsChat : strControls);
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0))        /* Go to StartScreen state of the selected games */
  {
    ButtonAcknowledge(BUTTON0);
    
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else if (CurrentGame == FROGGER)
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    else if (CurrentGame == MEMORY)
    {
      Game_StateMachine = Memory_StartScreen;
    }
    else
    {
      if (bMsgAvailable)
        Game_StateMachine = Game_PrintIncoming;
      else
        Game_StateMachine = Game_Chat;
    }
    
    if (bLFSRinitialized == FALSE)
    {
      u16LFSR = (u16)G_u32SystemTime1ms;
      bLFSRinitialized = TRUE;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON3))        /* Go to HighScore state */
  {
    ButtonAcknowledge(BUTTON3);
    if (CurrentGame != CHAT)
    {
      Game_StateMachine = Game_HighScore;
      bFirstEntry = TRUE;
    }
  }
  else if (WasButtonPressed(BUTTON1))        /* Move LEFT in list of games */
  {
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    if (CurrentGame == FROGGER)
    {
      CurrentGame = RUNNER;
      LCDMessage(LINE1_START_ADDR, strRunner);
    }
    else if (CurrentGame == MEMORY)
    {
      CurrentGame = FROGGER;
      LCDMessage(LINE1_START_ADDR, strFrogger);
    }
    else if (CurrentGame == CHAT)
    {
      CurrentGame = MEMORY;
      LCDMessage(LINE1_START_ADDR, strMemory);
      LCDMessage(LINE2_START_ADDR, strControls);
    }
  }
  else if (WasButtonPressed(BUTTON2))        /* Move RIGHT in list of games */
  {
    ButtonAcknowledge(BUTTON2);
    if (CurrentGame == RUNNER)
    {
      CurrentGame = FROGGER;
      LCDMessage(LINE1_START_ADDR, strFrogger);
    }
    else if (CurrentGame == FROGGER)
    {
      CurrentGame = MEMORY;
      LCDMessage(LINE1_START_ADDR, strMemory);
    }
    else if (CurrentGame == MEMORY)
    {
      CurrentGame = CHAT;
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, strChat);
      LCDMessage(LINE2_START_ADDR, strCtrlsChat);
    }
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
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, astrChatLines[(u8CurrentLine + 1) % 20]);
    LCDMessage(LINE2_START_ADDR, astrChatLines[u8CurrentLine]);
    while(KeyboardData());
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON3))
  {
    ButtonAcknowledge(BUTTON3);
    astrChatLines[u8CurrentLine][u8Col] = '\0';
    pTMsgChat[u8TCharNum] = '\0';
    u8ScrollBack = 0;
    bFirstEntry = TRUE;
    Game_StateMachine = Game_MainMenu;
    return;
  }
  
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
      astrChatLines[u8CurrentLine][u8Col] = '\0';
      if (pTMsgChat[u8TCharNum - 1] == '\n')
        pTMsgChat[u8TCharNum - 1] = '\0';
      else
      {
        LCDCommand(LCD_CLEAR_CMD);
        LCDMessage(LINE1_START_ADDR, astrChatLines[u8CurrentLine]);
        if (u8CurrentLine == 0)
          u8CurrentLine = CHAT_NUM_LINES;
        u8CurrentLine--;
        pTMsgChat[u8TCharNum] = '\0';
      }
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
      while(KeyboardData());
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

static void ANT_SlaveWaitChannelOpen(void)
{
  if (AntRadioStatus() == ANT_OPEN)
  {
    ANT_StateMachine = ANT_SlaveChannelOpen;
  }
  if (IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE)) 
  {
    AntCloseChannel();
    ANT_StateMachine = ANT_SlaveWaitChannelClose;
  }
}
static void ANT_SlaveChannelOpen(void)
{
  if (bWaitingAck && u16AckTimeout)
  {
    u16AckTimeout--;
  }
  
  if (AntRadioStatus() != ANT_OPEN)
  {
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
    }
  }
}
static void ANT_SlaveWaitChannelClose(void)
{
  if (AntRadioStatus() == ANT_CLOSED)
  {
    AntOpenChannel();
    UserApp_u32Timeout = G_u32SystemTime1ms;
    ANT_StateMachine = ANT_SlaveWaitChannelOpen;
  }
  
  if (IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE))
  {
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
  static u8 u8PacketNumber = 0;
  static u8 u8CharIndex = 0;
  if (bRResend)
  {
    pDataPacket[0] = RSD_CODE | u8REOBit;
    bRResend = FALSE;
    return;
  }
  if (bRAck)
  {
    pDataPacket[0] = ACK_CODE | u8REOBit;
    bRAck = FALSE;
    return;
  }
  if (bWaitingAck && u16AckTimeout == 0)
  {
    pDataPacket[0] = RQT_CODE | u8TEOBit;
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
}


static void Game_HighScore()
{
  static u8 strLine1[] = "High Score:";
  static u8 strLine2[] = "          ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDCommand(LCD_CLEAR_CMD);
    
    to_decimal(strLine2, u32HighScore[CurrentGame]);
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR + 6, strLine2);
    
    bFirstEntry = FALSE;
  }
  /* Go to MainMenu state */
  if (WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
  { 
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}
static void Game_ConfirmExit()
{
  static u8 strLine1[] = "      EXIT TO:      ";
  static u8 strLine2[] = "MENU START    CANCEL";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON3);
    
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON3))  /* Go back to Running state of CurrentGame */
  {
    ButtonAcknowledge(BUTTON2); /* Clearing buttons common to both games */
    ButtonAcknowledge(BUTTON3);
    if (CurrentGame == RUNNER)
    {
      LCDCommand(LCD_CLEAR_CMD);
      Game_StateMachine = Runner_Running;
    }
    else if (CurrentGame == FROGGER)
    {
      ButtonAcknowledge(BUTTON0);
      ButtonAcknowledge(BUTTON1);
      LedOff(LCD_GREEN);
      LedOff(LCD_RED);
      Game_StateMachine = Frogger_Running;
    }
    else
    {
      LCDCommand(LCD_CLEAR_CMD);
      Game_StateMachine = Memory_Output;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON1)) /* Go to StartScreen state of CurrentGame */
  {
    for (u8 i = 0; i < 8; i++)
    {
      LedOff((LedNumberType)i);
    }
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else if (CurrentGame == FROGGER)
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    else
    {
      Game_StateMachine = Memory_StartScreen;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON0)) /* Go to MainMenu state */
  { 
    for (u8 i = 0; i < 8; i++)
    {
      LedOff((LedNumberType)i);
    }
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}
static void Game_GameOver()
{
  static u8 u8Counter = 0;
  static u8 u8TickNumber = 0;
  
  if (u8Counter == 0) 
  {
    u8Counter = 250;
    
    if (u8TickNumber == 0)
    {
      LedOn(LCD_RED);
      LedOff(LCD_GREEN);
      LedOff(LCD_BLUE);
      ButtonAcknowledge(BUTTON0);
      ButtonAcknowledge(BUTTON1);
      ButtonAcknowledge(BUTTON2);
      ButtonAcknowledge(BUTTON3);
      timed_sound(C5, 1750);
    }
    if (u8TickNumber == 1)
    {
      LCDMessage(LINE1_START_ADDR, "     GAME OVER!     ");
    }
    if (u8TickNumber % 2)
    {
      for (u8 i = 0; i < 8; i++)
      {
        LedOff((LedNumberType)i);
      }
    }
    else
    {
      for (u8 i = 0; i < 8; i++)
      {
        LedOn((LedNumberType)i);
      }
    }
    
    u8TickNumber++;
  }
  u8Counter--;
  
  /* Go to ScoreBoard */
  if (u8TickNumber == 8)
  {
    for (u8 i = 0; i < 8; i++)
    {
      LedOff((LedNumberType)i);
    }
    LedOn(LCD_GREEN);
    LedOn(LCD_BLUE);
    u8TickNumber = 0;
    u8Counter = 0;
    Game_StateMachine = Game_ScoreBoard;
  }
}
static void Game_ScoreBoard()
{
  static u8 strLine1[18] = "Score: ";
  static u8 strLine2[17] = "High: ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    LCDCommand(LCD_CLEAR_CMD);
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    to_decimal(&strLine1[7], u32Score);
    if (u32Score > u32HighScore[CurrentGame])
      u32HighScore[CurrentGame] = u32Score;
    to_decimal(&strLine2[6], u32HighScore[CurrentGame]);
    
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
  {
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else if (CurrentGame == FROGGER)
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    else
    {
      Game_StateMachine = Memory_StartScreen;
    }
    bFirstEntry = TRUE;
  }
}


static void Memory_StartScreen()
{
  static u8 strLine1[] = " REPEAT THE PATTERN ";
  static u8 strLine2[] = "START            ESC";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);         /* Clears all button presses before entering Running state */
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    u32Memory_SequenceLength = 0;
    u32Memory_SequencePosition = 0;
    u32Memory_PauseLength = 300;
    u32Score = 0;
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0)) /* Go to Output state for Memory Game */
  {
    LCDCommand(LCD_CLEAR_CMD);
    Game_StateMachine = Memory_Output;
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON3)) /* Go to MainMenu */
  {
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}
static void Memory_Input()
{
  static u32 u32GOTimer = 5000;
  static u32 u32Button;
  static bool bButtonHeld = FALSE;
  static bool bCompleted = FALSE;
  
  if (bButtonHeld && !IsButtonPressed(u32Button))
  {
    if (!bCompleted)
    {
      LCDCommand(LCD_CLEAR_CMD);
    }
    PWMAudioOff(BUZZER1);
    bButtonHeld = FALSE;
  }
  
  u32GOTimer--;
  if (bCompleted)
  {
    if (WasButtonPressed(BUTTON3))
    {
      Game_StateMachine = Game_ConfirmExit;
      PWMAudioOff(BUZZER1);
      bCompleted = FALSE;
      LedOn(LCD_RED);
      LedOn(LCD_BLUE);
      u32GOTimer = 5000;
    }
    else if (u32GOTimer == 600)
    {
      LCDCommand(LCD_CLEAR_CMD);
      PWMAudioOff(BUZZER1);
      bButtonHeld = FALSE;
      LedOn(LCD_RED);
      LedOn(LCD_BLUE);
    }
    else if (u32GOTimer == 0)
    {
      Game_StateMachine = Memory_Output;
      bCompleted = FALSE;
      u32GOTimer = 5000;
    }
    return;
  }
  
  for (u32 i = 0; i < 4; i++)
  {
    if (WasButtonPressed(i))
    {
      ButtonAcknowledge(i);
      u32GOTimer = 5000;
      u32Button = i;
      LCDCommand(LCD_CLEAR_CMD);
      switch (i)
      {
      case BUTTON0:
        PWMAudioSetFrequency(BUZZER1, C5);
        LCDMessage(LINE1_START_ADDR, SOLID);
        LCDMessage(LINE2_START_ADDR, SOLID);
        break;
      case BUTTON1:
        PWMAudioSetFrequency(BUZZER1, D5);
        LCDMessage(LINE1_START_ADDR + 6, SOLID);
        LCDMessage(LINE2_START_ADDR + 6, SOLID);
        break;
      case BUTTON2:
        PWMAudioSetFrequency(BUZZER1, E5);
        LCDMessage(LINE1_START_ADDR + 13, SOLID);
        LCDMessage(LINE2_START_ADDR + 13, SOLID);
        break;
      case BUTTON3:
        PWMAudioSetFrequency(BUZZER1, G5S);
        LCDMessage(LINE1_START_ADDR + 19, SOLID);
        LCDMessage(LINE2_START_ADDR + 19, SOLID);
        break;
      }
      if (i != (au8Memory_Sequence[u32Memory_SequencePosition / 4] >> (2 * (u32Memory_SequencePosition % 4))) % 4)
      {
        Game_StateMachine = Game_GameOver;
        bButtonHeld = FALSE;
        PWMAudioOff(BUZZER1);
      }
      else if(++u32Memory_SequencePosition == u32Memory_SequenceLength)
      {
        bCompleted = TRUE;
        bButtonHeld = TRUE;
        u32Score++;
        led_score();
        u32Memory_SequencePosition = 0;
        if (u32Memory_PauseLength > 50)
        {
          u32Memory_PauseLength -= 5;
        }
        u32GOTimer = 1000;
        LedOff(LCD_RED);
        LedOff(LCD_BLUE);
        PWMAudioOn(BUZZER1);
      }
      else
      {
        bButtonHeld = TRUE;
        PWMAudioOn(BUZZER1);
      }
      
      break;
    }
  }
    
  if (u32GOTimer == 0)
  {
    Game_StateMachine = Game_GameOver;
    u32GOTimer = 5000;
    LCDCommand(LCD_CLEAR_CMD);
    PWMAudioOff(BUZZER1);
  }
}
static void Memory_Output()
{
  static MemoryOutput_Type OutputState = PAUSE;
  static u32 u32Counter = 300;
  
  if (WasButtonPressed(BUTTON3))
  {
    ButtonAcknowledge(BUTTON3);
    OutputState = PAUSE;
    u32Counter = u32Memory_PauseLength;
    Game_StateMachine = Game_ConfirmExit;
    return;
  }
  
  u32Counter--;
  if (u32Counter == 0)
  {
    if (OutputState == PAUSE)
    {
      u32Counter = 300;
      OutputState = DISPLAY;
      
      if (u32Memory_SequencePosition == 0)
      {
        if (u32Memory_SequenceLength % 4 == 0)
        {
          if (u32Memory_SequenceLength == 500)
          {
            u32Memory_SequenceLength = 0;
            u32Memory_SequencePosition = 0;
          }
          au8Memory_Sequence[u32Memory_SequenceLength / 4] = randInt();
        }
        u32Memory_SequenceLength++;
      }
      if (u32Memory_SequenceLength == u32Memory_SequencePosition)
      {
        return;
      }
      
      switch ((au8Memory_Sequence[u32Memory_SequencePosition / 4] >> (2 * (u32Memory_SequencePosition % 4))) % 4)
      {
      case 0:
        timed_sound(C5, 300);
        LCDMessage(LINE1_START_ADDR, SOLID);
        LCDMessage(LINE2_START_ADDR, SOLID);
        break;
      case 1:
        timed_sound(D5, 300);
        LCDMessage(LINE1_START_ADDR + 6, SOLID);
        LCDMessage(LINE2_START_ADDR + 6, SOLID);
        break;
      case 2:
        timed_sound(E5, 300);
        LCDMessage(LINE1_START_ADDR + 13, SOLID);
        LCDMessage(LINE2_START_ADDR + 13, SOLID);
        break;
      case 3:
        timed_sound(G5S, 300);
        LCDMessage(LINE1_START_ADDR + 19, SOLID);
        LCDMessage(LINE2_START_ADDR + 19, SOLID);
        break;
      }
      u32Memory_SequencePosition++;
    }
    else
    {
      if (u32Memory_SequencePosition == u32Memory_SequenceLength)
      {
        ButtonAcknowledge(BUTTON0);
        ButtonAcknowledge(BUTTON1);
        ButtonAcknowledge(BUTTON2);
        ButtonAcknowledge(BUTTON3);
        Game_StateMachine = Memory_Input;
        u32Memory_SequencePosition = 0;
      }
      u32Counter = u32Memory_PauseLength;
      OutputState = PAUSE;
      LCDCommand(LCD_CLEAR_CMD);
    }
  }
}


static void Frogger_StartScreen()
{
  static u8 strLine1[] = "START    CONTROLS:  ";
  static u8 strLine2[] = "\x7F     "UP"      \x7E   ESC";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    for (u8 i = 0; i < 20; i++)
      au8Frogger_strLineA[i] = LOG[0];
    Frogger_LineA = (FroggerLine_Type){ au8Frogger_strLineA, LEFT, 1, LOGS };
    Frogger_LineB = (FroggerLine_Type){ au8Frogger_strLineB, RIGHT, 0, WATER };
    new_line(&Frogger_LineB);
    aFrogger_Lines[0] = &Frogger_LineA;
    aFrogger_Lines[1] = &Frogger_LineB;
    
    u32Score = 0;
    s8Frogger_Position = 10;
    u32TickLength = 500;
    u32Frogger_Counter = u32TickLength;
    u8Frogger_Forward = 0;
    bFrogger_ScreenUpdate = FALSE;
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0)) /* Go to Running state of Frogger */
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE2_START_ADDR, aFrogger_Lines[0]->line_ptr);
    LCDMessage(LINE2_START_ADDR + 10, STICKMAN);
    LCDMessage(LINE1_START_ADDR, aFrogger_Lines[1]->line_ptr);
    LedOff(LCD_GREEN);
    LedOff(LCD_RED);
    
    Game_StateMachine = Frogger_Running;
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON3)) /* Go to MainMenu state */
  {
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}
static void Frogger_Running()
{
  bool bScoreChanged = FALSE;
  
  if (WasButtonPressed(BUTTON3))
  {
    Game_StateMachine = Game_ConfirmExit;
    LedOn(LCD_GREEN);
    LedOn(LCD_RED);
    bFrogger_ScreenUpdate = TRUE;
    return;
  }
  
  u32Frogger_Counter--;
  if (u32Frogger_Counter == 0)
  {
    u32Frogger_Counter = u32TickLength;
    if (u8Frogger_Forward == 1)
    {
      u8Frogger_Forward = 0;
      FroggerLine_Type *temp = aFrogger_Lines[0];
      aFrogger_Lines[0] = aFrogger_Lines[1];
      new_line(temp);
      aFrogger_Lines[1] = temp;
    }
    if (u8Frogger_Forward > 1)
      u8Frogger_Forward--;
    if (u32Score != 0)
    {
      if (aFrogger_Lines[(u8Frogger_Forward == 0) ? 0 : 1]->direction == RIGHT)
      {
        s8Frogger_Position++;
      }
      else
      {
        s8Frogger_Position--;
      }
      shift_line(aFrogger_Lines[0]);
    }
    shift_line(aFrogger_Lines[1]);
    bFrogger_ScreenUpdate = TRUE;
  }
  
  if (WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);
    if (u8Frogger_Forward == 0)
    {
      timed_sound(C6, 50);
      u32Score++;
      bScoreChanged = TRUE;
      if (u32TickLength > 200)
      {
        u32TickLength = 150000/(300 + 3*u32Score);
      }
      bFrogger_ScreenUpdate = TRUE;
      u8Frogger_Forward = 2;
    }
  }
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    timed_sound(C6, 50);
    s8Frogger_Position--;
    bFrogger_ScreenUpdate = TRUE;
  }
  if (WasButtonPressed(BUTTON2))
  {
    ButtonAcknowledge(BUTTON2);
    timed_sound(C6, 50);
    s8Frogger_Position++;
    bFrogger_ScreenUpdate = TRUE;
  }
  
  if (bFrogger_ScreenUpdate)
  {
    if (s8Frogger_Position < 0)
    {
      s8Frogger_Position = 0;
    }
    else if (s8Frogger_Position >= 20)
    {
      s8Frogger_Position = 19;
    }
    LCDMessage(LINE2_START_ADDR, aFrogger_Lines[0]->line_ptr);
    LCDMessage(LINE1_START_ADDR, aFrogger_Lines[1]->line_ptr);
    if (aFrogger_Lines[(u8Frogger_Forward == 0) ? 0 : 1]->line_ptr[s8Frogger_Position] == ' ')
    {
      LCDMessage(((u8Frogger_Forward == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + s8Frogger_Position, "X");
      if (u8Frogger_Forward != 0)
      {
        u32Score--;
      }
      bScoreChanged = FALSE;
      Game_StateMachine = Game_GameOver;
      
    }
    else
    {
      LCDMessage(((u8Frogger_Forward == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + s8Frogger_Position, STICKMAN);
    }
    bFrogger_ScreenUpdate = FALSE;
  }
  
  if (bScoreChanged)
  {
    led_score();
  }
}

static void new_line(FroggerLine_Type *line)
{
  line->sequence_length = 0;
  line->sequence_type = WATER;
  
  for (u8 i = (line->direction == RIGHT) ? 19 : 0; (line->direction == RIGHT) ? (i-- > 0) : (i < 20); (line->direction == RIGHT) ?  i : i++)
  {
    if (line->sequence_length == 0)
    {
      if (line->sequence_type == LOGS)
      {
        line->sequence_type = WATER;
        line->sequence_length = 3 + randInt() % 6; /* 3 to 8 */
      }
      else
      {
        line->sequence_type = LOGS;
        line->sequence_length =  3 + randInt() % 2; /* 3 to 4 */
      }
    }
    if (line->sequence_type == LOGS)
    {
      line->line_ptr[i] = LOG[0];
    }
    else
    {
      line->line_ptr[i] = ' ';
    }
    line->sequence_length--;
  }
}
static void shift_line(FroggerLine_Type *line)
{
  u8 new_index;
  if (line->direction == RIGHT)
  {
    for (u8 i = 20; i-- > 0;)
      line->line_ptr[i] = line->line_ptr[i - 1];
    new_index = 0;
  }
  else
  {
    for (u8 i = 0; i < 20; i++)
      line->line_ptr[i] = line->line_ptr[i + 1];
    new_index = 19;
  }
  if (line->sequence_length == 0)
  {
    if (line->sequence_type == LOGS)
    {
      line->sequence_type = WATER;
      line->sequence_length = 3 + randInt() % 6; /* 3 to 8 */
    }
    else
    {
      line->sequence_type = LOGS;
      line->sequence_length =  3 + randInt() % 2; /* 3 to 4 */
    }
  }
  if (line->sequence_type == LOGS)
  {
    line->line_ptr[new_index] = LOG[0];
  }
  else
  {
    line->line_ptr[new_index] = ' ';
  }
  line->sequence_length--;
}


static void Runner_StartScreen()
{
  static u8 strLine1[] = "          CONTROLS: ";
  static u8 strLine2[] = "START      JUMP  ESC";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);         /* Clears all button presses before entering Running state */
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    for (u8 i = 0; i < 20; i++)         /* Initialize Cactuses */
    {
      au8Runner_Cactuses[i] = ' ';
    }
    au8Runner_Cactuses[20] = '\0';
    u32TickLength = 250;
    u8Runner_SequenceLength = 0;
    Runner_SequenceType = GROUND;
    u32Score = 0;
    bRunner_ScreenUpdate = TRUE;
    u8Runner_Jump = 0;
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0)) /* Go to Running state for Runner */
  {
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    LCDCommand(LCD_CLEAR_CMD);
    Game_StateMachine = Runner_Running;
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON3)) /* Go to MainMenu */
  {
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}
static void Runner_Running()
{ 
  static u32 u32Counter = 250;
  
  if (WasButtonPressed(BUTTON3))
  {
    Game_StateMachine = Game_ConfirmExit;
    bRunner_ScreenUpdate = TRUE;
    return;
  }
  
  u32Counter--;
  if (u32Counter == 0)
  {
    u32Score++;
    if (u32TickLength > 75)
    {
      u32TickLength = 1000000/(4000 + 12*u32Score);
    }
    u32Counter = u32TickLength;
    
    led_score();
    cactus_update();
    
    if (u8Runner_Jump != 0)
    {
      u8Runner_Jump--;
    }
    else if (WasButtonPressed(BUTTON2))
    {
      u8Runner_Jump = 4;
      timed_sound(C6, 50);
    }
    ButtonAcknowledge(BUTTON2);              /* Clears button press even if button was pressed while still in air */
    
    bRunner_ScreenUpdate = TRUE;
  }
  
  if (bRunner_ScreenUpdate)
  {
    LCDMessage(LINE2_START_ADDR, au8Runner_Cactuses);
    
    if (u8Runner_Jump == 0)
    {
      LCDMessage(LINE1_START_ADDR + 1, " "); /* Clears stick man if in upper row */
      if (au8Runner_Cactuses[1] == ' ')
      {
        LCDMessage(LINE2_START_ADDR + 1, STICKMAN);
      }
      else
      {
        LCDMessage(LINE2_START_ADDR + 1, "X");
        ButtonAcknowledge(BUTTON0);
        ButtonAcknowledge(BUTTON1);
        Game_StateMachine = Game_GameOver;
      }
    }
    else
    {
      LCDMessage(LINE1_START_ADDR + 1, STICKMAN);
    }
    
    bRunner_ScreenUpdate = FALSE;
  }
}

static void cactus_update()
{ 
  for (u8 i = 0; i < 19; i++)               /* Shift Cactuses 1 left)  */
  {
    au8Runner_Cactuses[i] = au8Runner_Cactuses[i + 1];
  }
  if (u8Runner_SequenceLength == 0)
  {
    if (Runner_SequenceType == CACTUS)
    {
      Runner_SequenceType = GROUND;
      u8Runner_SequenceLength = 4 + randInt() % 4; /* Sequences of length 4 to 7 */
    }
    else
    {
      Runner_SequenceType = CACTUS;
      u8Runner_SequenceLength = 1 + randInt() % 3; /* Sequences of length 1 to 3 */
    }
  }
  if (Runner_SequenceType == CACTUS)
  {
    au8Runner_Cactuses[19] = 0x1D;
  }
  else
  {
    au8Runner_Cactuses[19] = ' ';
  }
  u8Runner_SequenceLength--;
}


static void timed_sound(u16 note, u16 duration_ms)
{
  bSound_Active = TRUE;
  u16Sound_Duration = duration_ms;
  
  PWMAudioSetFrequency(BUZZER1, note);
  PWMAudioOn(BUZZER1);
}
static void led_score()
{
  for (u8 i = 0; i < 8; i++)
  {
    if (u32Score & (1u << (7 - i)))
    {
      LedOn((LedNumberType)i);
    }
    else
    { 
      LedOff((LedNumberType)i);
    }
  }
}
static void to_decimal(u8 *str, u32 num)
{
  u8 decimal[10];
  u8 digit;
  if (num == 0)
  {
    digit = 1;
    decimal[0] = 0;
  }
  else
  {
    for (digit = 0; num != 0; digit++)
    {
      decimal[digit] = num % 10u;
      num /= 10u;
    }
  }
  for (; digit-- > 0; str++)
  {
    *str = decimal[digit] + '0';
  }
  *str = '\0';
}
static u8 randInt()
{
  for (u8 i = 0; i < 8; i++)
  {
    u16LFSR = (u16LFSR << 1) | (1u & ((u16LFSR >> 15) ^ (u16LFSR >> 14) ^ (u16LFSR >> 12) ^ (u16LFSR >> 3)));  /* [16, 15, 13, 4, 0] */
  }
  return (u8)u16LFSR;
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
