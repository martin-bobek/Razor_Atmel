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
static u8 au8Cactuses[21];                         /* Characters for the bottom row of LCD */

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
  for (u8 i = 0; i < 20; i++)
  {
    au8Cactuses[i] = ' ';
  }
  au8Cactuses[20] = '\0';
  
  LCDCommand(LCD_CLEAR_CMD);
  
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
void cactus_update()
{
  static u8 u8SequenceLength = 1;  /* change to actual random value*/
  static bool bCactusSequence = TRUE;
  
  for (u8 i = 0; i < 19; i++)
  {
    if (au8Cactuses[i + 1] == 29)
    {
      au8Cactuses[i] = 29;
    }
    else
    {
      au8Cactuses[i] = ' ';
    }
  }
  if (u8SequenceLength == 0)
  {
    if (bCactusSequence)
    {
      bCactusSequence = FALSE;
      u8SequenceLength = randInt(6) + 2; /* Sequences of length 2 to 7 */
    }
    else
    {
      bCactusSequence = TRUE;
      u8SequenceLength = randInt(3) + 1; /* Sequences of length 1 to 3 */
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
  u8SequenceLength--;
}

u32 randInt(u32 modulo)
{
  static u32 LFSR = 0x6A42BE71;
  for (u8 i = 0; i < 32; i++)
  {
    LFSR = (LFSR << 1) | (1u & ((LFSR >> 31) ^ (LFSR >> 29) ^ (LFSR >> 28) ^ (LFSR >> 24))); /* [32, 30, 29, 25, 0] */
  }
  return LFSR % modulo;
}

/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserAppSM_Idle(void)
{
  u8 *msgGameOver = "     GAME OVER!     ";
  u8 strStickMan[] = { 171, '\0' };
  static u32 u32TickLength = 250;
  static u32 u32Counter = 251;
  static u8 u8Jump = 0;
  static bool bGameOver = FALSE;
  
  if (!bGameOver)
  {
    u32Counter--;
  }
  if (u32Counter == 0)
  {
    u32Counter = u32TickLength;
    cactus_update();
    if (u8Jump != 0)
    {
      u8Jump--;
    }
    else if (WasButtonPressed(BUTTON2))
    {
      ButtonAcknowledge(BUTTON2);
      u8Jump = 3;
    }
    if (u8Jump == 0)
    {
      LCDMessage(LINE1_START_ADDR + 1, " "); /* Clears stick man if in upper row */
      if (au8Cactuses[1] != ' ')
      {
        au8Cactuses[1] = 'X';
        bGameOver = TRUE;
        LCDMessage(LINE1_START_ADDR, msgGameOver);
      }
      else
      {
        au8Cactuses[1] = 171; /* StickMan */
      }
    }
    else
    {
      LCDMessage(LINE1_START_ADDR + 1, strStickMan);
    }
    LCDMessage(LINE2_START_ADDR, au8Cactuses);
  }
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
