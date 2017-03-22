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

static bool SendRequest = FALSE;
static u8 TextMessage[21] = "";

static u32 UserApp1_u32DataMsgCount = 0;
static u32 UserApp1_u32TickMsgCount = 0;

static u8 u8LastState = 0xFF;
static u8 au8TickMessage[] = "EVENT x\n\r";
static u8 au8DataContent[] = "xxxxxxxxxxxxxxxx";
static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static u8 au8TestMessage[] = {0, 0, 0, 0, 0xA5, 0, 0, 0};
bool bGotNewData;


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
Function: UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserApp1Initialize(void)
{
  LCDCommand(LCD_CLEAR_CMD);
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
  
  //UserApp1_StateMachine = UserApp1SM_SelectANT;
#if 1
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
#endif
  
#if 0
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
#endif
  
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
  KeyboardService();
  UserApp1_StateMachine();

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static void KeyboardService(void)
{
  static u8 CharNum = 0;
  static bool bFull = FALSE;
  static u8 cStr[2] = {0};
  /*
  if (SendRequest)
  {
    bFull = FALSE;
    LCDCommand(LCD_CLEAR_CMD);
  }
  */
  while (*cStr = KeyboardData())
  {
    if (*cStr > ENT_)
    {
      continue;
    }
    if (*cStr == ENT_)
    {
      LCDCommand(LCD_CLEAR_CMD);
      for (; CharNum < 20; CharNum++)
      {
        TextMessage[CharNum] = '\0';
      }
      CharNum = 0;
      SendRequest = TRUE;
      bFull = FALSE;
      continue;
    }
    if (!bFull)
    {
      TextMessage[CharNum] = *cStr;
      LCDMessage(LINE1_START_ADDR + CharNum, cStr);
      CharNum++;
      if (CharNum == 20)
      {
        bFull = TRUE;
      }
    }
  }
  /*
  //static u8 cStr[5] = "  \r\n";
  static u8 cStr[2] = {0};
  static u8 col = 0;
  static u8 strLine[20] = {0};
  while (*cStr = KeyboardData())
  {
    if (*cStr >= ENT_)
    {
      continue;
    }
    strLine[col] = *cStr;
    LCDMessage(LINE2_START_ADDR + col, cStr);
    col++;
    if (col == 20)
    {
      col = 0;
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, strLine);
    }
  }
  static u8 cStr[4] = "";
  while (cStr[0] = KeyboardData())
  {
    if (cStr[0] == ENT_)
    {
      cStr[0] = '\r';
      cStr[1] = '\n';
      cStr[2] = '\0';
    }
    else if (cStr[0] == BKS_)
    {
      cStr[0] = '\b';
      cStr[1] = ' ';
      cStr[2] = '\b';
    }
    else if (cStr[0] > BKS_)
      continue;
    else
      cStr[1] = '\0';
    DebugPrintf(cStr);
  }*/
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
    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
  }
}   
static void UserApp1SM_WaitChannelOpen(void)
{
  if (AntRadioStatus() == ANT_OPEN)
  {
    LedOn(GREEN);
    UserApp1_StateMachine = UserApp1SM_ChannelOpen;
  }
  if (IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE)) 
  {
    AntCloseChannel();
    LedOff(GREEN);
    LedOn(YELLOW);
    UserApp1_StateMachine = UserApp1SM_SlaveIdle;
  }
}
static void UserApp1SM_ChannelOpen(void)
{
  static u8 u8CharIndex = 0;
  
  static u8 au8DataPacket[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static u8 u8PacketNumber = 0;
  
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    
    AntCloseChannel();
    u8LastState = 0xFF;
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  }
      
  if (AntRadioStatus() != ANT_OPEN)
  {
    LedBlink(GREEN, LED_2HZ);
    LedOff(BLUE);
    u8LastState = 0xFF;
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  }
      
  if (AntReadData())
  {
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      UserApp1_u32DataMsgCount++;
      bGotNewData = FALSE;
      for (u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if (G_au8AntApiCurrentData[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentData[i];
        }
      }
      
      if (bGotNewData)
      {
        if (au8LastAntData[0] == 0)
        {
          u8CharIndex = 0;
          for (u8 i = 0; i < 20; i++)
          {
            TextMessage[i] = '\0';
          }
          LCDCommand(LCD_CLEAR_CMD);
        }
        for (u8 i = 1; i < 8 && au8LastAntData[i] != '\0'; i++, u8CharIndex++)
        {
          TextMessage[u8CharIndex] = au8LastAntData[i];
        }
        LCDMessage(LINE1_START_ADDR, TextMessage);
      }
    }
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      if (u8LastState != G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        u8LastState = G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX];
        au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
        DebugPrintf(au8TickMessage);
        
        switch (u8LastState) {
        case RESPONSE_NO_ERROR:
          LedOff(GREEN);
          LedOn(BLUE);
          break;
        case EVENT_RX_FAIL:
          LedOff(GREEN);
          LedBlink(BLUE, LED_4HZ);
          break;
        case EVENT_RX_FAIL_GO_TO_SEARCH:
          LedOff(BLUE);
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
    }
  }
}
static void UserApp1SM_WaitChannelClose(void)
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

static void UserApp1SM_Master(void)
{ 
  static u8 au8DataPacket[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  //static u8 au8DataContent1[21] = "\0";
  //static u8 au8DataContent2[21] = "\0";
  
  //static bool bReceiving = TRUE;
  static u8 u8PacketNumber = 0;
  static u8 u8CharIndex = 0;
  
  if(AntReadData())
  {
    if (G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      bGotNewData = FALSE;
      for (u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if (G_au8AntApiCurrentData[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentData[i];
        }
      }
      
      if (bGotNewData)
      {
        if (au8LastAntData[0] == 0)
        {
          u8CharIndex = 0;
          for (u8 i = 0; i < 20; i++)
          {
            TextMessage[i] = '\0';
          }
          LCDCommand(LCD_CLEAR_CMD);
        }
        for (u8 i = 1; i < 8 && au8LastAntData[i] != '\0'; i++, u8CharIndex++)
        {
          TextMessage[u8CharIndex] = au8LastAntData[i];
        }
        LCDMessage(LINE1_START_ADDR, TextMessage);
      }
    }
    /*
    
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      if (G_au8AntApiCurrentData[0] == 0)
      {
        bReceiving = FALSE;
      }
      
      for (u8 i = 1; i < ANT_DATA_BYTES && index < 40; i++, index++)
      {
        if (index < 20)
        {
          au8DataContent1[index] = G_au8AntApiCurrentData[i];
        }
        else
        {
          au8DataContent2[index - 20] = G_au8AntApiCurrentData[i];
        }  
        if (G_au8AntApiCurrentData[i] == 0x00)
        {
          break;          
        }
      }
      
      if (bReceiving == FALSE)
      {
        LCDCommand(LCD_CLEAR_CMD);
        LCDMessage(LINE1_START_ADDR, au8DataContent1);
        LCDMessage(LINE2_START_ADDR, au8DataContent2);
        au8DataContent1[0] = '\0';
        au8DataContent2[0] = '\0';
        index = 0;
        bReceiving = TRUE;
      }
    }
  */
    else if (G_eAntApiCurrentMessageClass == ANT_TICK)
    {
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
