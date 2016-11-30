/**********************************************************************************************************************
File: user_app.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
 1. Copy both user_app.c and user_app.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
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
void UserAppInitialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserAppRunActiveState(void)
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


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp_StateMachine;            /* The state machine function pointer */
static u32 UserApp_u32Timeout;                      /* Timeout counter used across states */
static fnCode_type Game_StateMachine;               /* Game state machine function pointer */
static u32 u32Score = 0;                            /* Number of gameticks the player has survived */
static u8 au8Cactuses[21];                          /* Characters for the bottom row of LCD */
static u32 u32TickLength;                           /* Initial game tick length */
static u16 LFSR;                                    /* Linear Feedback Shift Register for randInt */
static bool bLFSRinitialized = FALSE;               /* Indicates LFSR has not been initialized with seed */
static u16 u16SequenceLength;                       /* Remaining length of sequence of cactuses/spaces */
static bool bCactusSequence;                        /* True indicates current sequence is cactuses */

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
Function: UserAppInitialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserAppInitialize(void)
{
  LCDCommand(LCD_CLEAR_CMD);
  
  Game_StateMachine = Game_StartScreen;
  
  /* If good initialization, set state to Idle */
  if( 1 )
  {
    UserApp_StateMachine = UserAppSM_Idle;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    UserApp_StateMachine = UserAppSM_FailedInit;
  }

} /* end UserAppInitialize() */


/*----------------------------------------------------------------------------------------------------------------------
Function UserAppRunActiveState()

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
  UserApp_StateMachine();

} /* end UserAppRunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static void Game_StartScreen()
{
  static bool bUninitialized = TRUE;
  u8 *strLine1 = "          CONTROLS: ";
  u8 *strLine2 = "START      JUMP  ESC";
  
  if (bUninitialized)
  {
    ButtonAcknowledge(BUTTON0);         /* Clears all button presses before entering Running state */
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    for (u8 i = 0; i < 20; i++)         /* Initialize Cactuses */
    {
      au8Cactuses[i] = ' ';
    }
    au8Cactuses[20] = '\0';
    u32TickLength = 250;
    u16SequenceLength = 0;
    bCactusSequence = FALSE;
    u32Score = 0;
    
    bUninitialized = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);         /* Clears all button presses before entering Running state */
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    if (bLFSRinitialized == FALSE)
    {
      LFSR = (u16)G_u32SystemTime1ms;
      bLFSRinitialized = TRUE;
    }
    
    LCDCommand(LCD_CLEAR_CMD);
    bUninitialized = TRUE;             /* Resets uninitialized flag: game gets reinitialized next time StartScreen is entered */
    Game_StateMachine = Game_Running;
  }
}

static void Game_Running()
{
  u8 strStickMan[] = { 171, '\0' };
  static u32 u32Counter = 250;
  static u8 u8Jump = 0;
  
  if (WasButtonPressed(BUTTON3))
  {
    Game_StateMachine = Game_StartScreen;
    return;
  }
  u32Counter--;
  if (u32Counter == 0)
  {
    u32Counter = u32TickLength;
    u32Score++;
    led_score();
    cactus_update();
    LCDMessage(LINE2_START_ADDR, au8Cactuses);
    
    if (u8Jump != 0)
    {
      u8Jump--;
    }
    else if (WasButtonPressed(BUTTON2))
    {
      u8Jump = 3;
    }
    ButtonAcknowledge(BUTTON2);              /* Clears button press even if button was pressed while still in air */
    
    if (u8Jump == 0)
    {
      LCDMessage(LINE1_START_ADDR + 1, " "); /* Clears stick man if in upper row */
      if (au8Cactuses[1] == ' ')
      {
        LCDMessage(LINE2_START_ADDR + 1, strStickMan);
      }
      else
      {
        LCDMessage(LINE2_START_ADDR + 1, "X");
        Game_StateMachine = Game_GameOver;
      }
    }
    else
    {
      LCDMessage(LINE1_START_ADDR + 1, strStickMan);
    }
  }
}

static void Game_GameOver()
{
  const u8 u8TickLength = 250;
  static u8 u8Counter = 250;
  static u8 u8TickNumber = 0;
  
  u8Counter--;
  if (u8Counter == 0) 
  {
    u8Counter = u8TickLength;
    u8TickNumber++;
    
    if (u8TickNumber == 0)
    {
      LCDMessage(LINE1_START_ADDR, "     GAME OVER!     ");
      ButtonAcknowledge(BUTTON0);
      ButtonAcknowledge(BUTTON1);
      ButtonAcknowledge(BUTTON2);
      ButtonAcknowledge(BUTTON3);
    }
    if (u8TickNumber % 2)
    {
      for (LedNumberType i = 0; i < 8; i++)
        LedOff(i);
    }
    else
    {
      for (LedNumberType i = 0; i < 8; i++)
        LedOn(i);
    }
    if ((u8TickNumber == 8) || WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
    {
      for (LedNumberType i = 0; i < 8; i++)
        LedOff(i);
      
      ButtonAcknowledge(BUTTON0);
      ButtonAcknowledge(BUTTON1);
      ButtonAcknowledge(BUTTON2);
      ButtonAcknowledge(BUTTON3);
      
      u8TickNumber = 0;
      Game_StateMachine = Game_ScoreBoard;
    }
  }
}

static void Game_ScoreBoard()
{
  static bool bFirstEntry = TRUE;
  static u8 au8String[18] = "Score: ";
  static u8 decimalScore[10];
  
  if (bFirstEntry)
  {
    LCDCommand(LCD_CLEAR_CMD);
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    u32 u32ScoreTemp = u32Score;
    u8 digit, index;
    for (digit = 0; u32ScoreTemp != 0; digit++)
    {
      decimalScore[digit] = u32ScoreTemp % 10u;
      u32ScoreTemp /= 10u;
    }
    for (index = 7, digit--; digit-- > 0; index++)
    {
      au8String[index] = decimalScore[digit] + '0';
    }
    au8String[index] = '\0';
    LCDMessage(LINE1_START_ADDR, au8String);
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    bFirstEntry = TRUE;
    
    Game_StateMachine = Game_StartScreen;
  }
}

static void led_score()
{
  for (LedNumberType i = 0; i < 8; i++)
  {
    if (u32Score & (1u << (7 - i)))
    {
      LedOn(i);
    }
    else
    { 
      LedOff(i);
    }
  }
}

static void cactus_update()
{ 
  for (u8 i = 0; i < 19; i++)               /* Shift Cactuses 1 left)  */
  {
    au8Cactuses[i] = au8Cactuses[i + 1];
  }
  if (u16SequenceLength == 0)
  {
    if (bCactusSequence)
    {
      bCactusSequence = FALSE;
      u16SequenceLength = 2 + randInt() % 6; /* Sequences of length 2 to 7 */
    }
    else
    {
      bCactusSequence = TRUE;
      u16SequenceLength = 1 + randInt() % 3; /* Sequences of length 1 to 3 */
    }
  }
  if (bCactusSequence)
  {
    au8Cactuses[19] = 29;
  }
  else
  {
    au8Cactuses[19] = ' ';
  }
  u16SequenceLength--;
}

static u16 randInt()
{
  for (u8 i = 0; i < 16; i++)
  {
    LFSR = (LFSR << 1) | (1u & ((LFSR >> 15) ^ (LFSR >> 14) ^ (LFSR >> 12) ^ (LFSR >> 3)));  /* [16, 15, 13, 4, 0] */
  }
  return LFSR;
}

/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserAppSM_Idle(void)
{
  Game_StateMachine();
} /* end UserAppSM_Idle() */
     

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserAppSM_Error(void)          
{
  
} /* end UserAppSM_Error() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* State to sit in if init failed */
static void UserAppSM_FailedInit(void)          
{
    
} /* end UserAppSM_FailedInit() */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
