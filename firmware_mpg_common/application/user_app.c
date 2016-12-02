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

Game_Type CurrentGame = RUNNER;                     /* Used for modifying behaviour of general purpose functions */
static u32 u32TickLength;                           /* Initial game tick length */
static u32 u32Score = 0;                            /* Number of gameticks the player has survived */
static u32 u32HighScore[2] = { 0, 0 };
static u16 LFSR;                                    /* Linear Feedback Shift Register for randInt */
static bool bLFSRinitialized = FALSE;               /* Indicates LFSR has not been initialized with seed */

static union {
  struct {
    u8 au8Cactuses[21];                           /* Characters for the bottom row of LCD */
    u8 u8SequenceLength;                          /* Remaining length of sequence of cactuses/spaces */
    RunnerSequence_Type SequenceType;             /* True indicates current sequence is cactuses */
    bool bScreenUpdate;
  } Run;
  struct {
    s8 s8Position;
    u8 au8Line1[21];
    u8 au8Line2[21];
    FroggerLine_Type Line1, Line2;
    FroggerLine_Type *lines[2];
    u32 u32Counter;
    u8 line_stickman;
    bool bPositionChanged;
  } Frog;
} Glob;

/* 
Known Bugs:
Runner: Returning from ConfirmExit to Running leaves message on top line.
Runner: Exiting game leaves led's on.

Game: A highscore of more than 8 digits is not representable in ScoreBoard state

*/



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
  Game_StateMachine = Game_MainMenu;
  
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
static void Game_MainMenu()
{
  u8 *au8Controls = "SCORE  \x7F    \x7E  ENTER";
  u8 *au8Runner = "       RUNNER       ";
  u8 *au8Frogger = "      FROGGER       ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    if (CurrentGame == RUNNER)
    {
      LCDMessage(LINE1_START_ADDR, au8Runner);
    }
    else
    {
      LCDMessage(LINE1_START_ADDR, au8Frogger);
    }
    LCDMessage(LINE2_START_ADDR, au8Controls);
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON3))        /* Go to StartScreen state of the selected games */
  {
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    
    if (bLFSRinitialized == FALSE)
    {
      LFSR = (u16)G_u32SystemTime1ms;
      bLFSRinitialized = TRUE;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON0))        /* Go to HighScore state */
  {
    Game_StateMachine = Game_HighScore;
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON1))        /* Move LEFT in list of games */
  {
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    if (CurrentGame == FROGGER)
    {
      CurrentGame = RUNNER;
      LCDMessage(LINE1_START_ADDR, au8Runner);
    }
  }
  else if (WasButtonPressed(BUTTON2))        /* Move RIGHT in list of games */
  {
    ButtonAcknowledge(BUTTON2);
    if (CurrentGame == RUNNER)
    {
      CurrentGame = FROGGER;
      LCDMessage(LINE1_START_ADDR, au8Frogger);
    }
  }
}
static void Game_HighScore()
{
  u8 au8Score_str[23] = "High Score: ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDCommand(LCD_CLEAR_CMD);
    
    to_decimal(&au8Score_str[12], u32HighScore[CurrentGame]);
    LCDMessage(LINE1_START_ADDR, au8Score_str);
    
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
  u8 *au8Line1_str = "      EXIT TO:      ";
  u8 *au8Line2_str = "MENU START    CANCEL";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON3);
    
    LCDMessage(LINE1_START_ADDR, au8Line1_str);
    LCDMessage(LINE2_START_ADDR, au8Line2_str);
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON3))  /* Go back to Running state of CurrentGame */
  {
    ButtonAcknowledge(BUTTON2); /* Clearing buttons common to both games */
    ButtonAcknowledge(BUTTON3);
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_Running;
    }
    else
    {
      ButtonAcknowledge(BUTTON0);
      ButtonAcknowledge(BUTTON1);
      Game_StateMachine = Frogger_Running;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON1)) /* Go to StartScreen state of CurrentGame */
  {
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    bFirstEntry = TRUE;
  }
  else if (WasButtonPressed(BUTTON0)) /* Go to MainMenu state */
  { 
    Game_StateMachine = Game_MainMenu;
    bFirstEntry = TRUE;
  }
}

static void Frogger_StartScreen()
{
  u8 *strLine1 = "START    CONTROLS:  ";
  u8 *strLine2 = "\x7F     ^      \x7E   ESC";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE1_START_ADDR, strLine1);
    LCDMessage(LINE2_START_ADDR, strLine2);
    
    for (u8 i = 0; i < 20; i++)
      Glob.Frog.au8Line1[i] = '=';
    Glob.Frog.Line1 = (FroggerLine_Type){ Glob.Frog.au8Line1, LEFT, 0, LOGS };
    Glob.Frog.Line2 = (FroggerLine_Type){ Glob.Frog.au8Line2, RIGHT, 0, WATER };
    new_line(&Glob.Frog.Line2);
    Glob.Frog.lines[0] = &Glob.Frog.Line1;
    Glob.Frog.lines[1] = &Glob.Frog.Line2;
    
    u32Score = 0;
    Glob.Frog.s8Position = 10;
    u32TickLength = 500;
    Glob.Frog.u32Counter = u32TickLength;
    Glob.Frog.line_stickman = 0;
    Glob.Frog.bPositionChanged = FALSE;
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0)) /* Go to Running state of Frogger */
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE2_START_ADDR, Glob.Frog.lines[0]->line_ptr);
    LCDMessage(LINE2_START_ADDR + 10, "\xAB");
    LCDMessage(LINE1_START_ADDR, Glob.Frog.lines[1]->line_ptr);
    
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
  if (WasButtonPressed(BUTTON3))
  {
    ButtonAcknowledge(BUTTON3);
    Game_StateMachine = Game_ConfirmExit;
    Glob.Frog.bPositionChanged = TRUE;
    return;
  }
  
  Glob.Frog.u32Counter--;
  if (Glob.Frog.u32Counter == 0)
  {
    Glob.Frog.u32Counter = u32TickLength;
    if (Glob.Frog.line_stickman == 1)
    {
      Glob.Frog.line_stickman = 0;
      FroggerLine_Type *temp = Glob.Frog.lines[0];
      Glob.Frog.lines[0] = Glob.Frog.lines[1];
      new_line(temp);
      Glob.Frog.lines[1] = temp;
    }
    if (Glob.Frog.line_stickman > 1)
      Glob.Frog.line_stickman--;
    if (u32Score != 0)
    {
      if (Glob.Frog.lines[(Glob.Frog.line_stickman == 0) ? 0 : 1]->direction == RIGHT)
      {
        Glob.Frog.s8Position++;
      }
      else
      {
        Glob.Frog.s8Position--;
      }
      shift_line(Glob.Frog.lines[0]);
    }
    shift_line(Glob.Frog.lines[1]);
    Glob.Frog.bPositionChanged = TRUE;
  }
  
  if (WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);
    if (Glob.Frog.line_stickman == 0)
    {
      u32Score++;
      if (u32TickLength > 250)
      {
        u32TickLength--;
      }
      Glob.Frog.bPositionChanged = TRUE;
      Glob.Frog.line_stickman = 2;
    }
  }
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    Glob.Frog.s8Position--;
    Glob.Frog.bPositionChanged = TRUE;
  }
  if (WasButtonPressed(BUTTON2))
  {
    ButtonAcknowledge(BUTTON2);
    Glob.Frog.s8Position++;
    Glob.Frog.bPositionChanged = TRUE;
  }
  
  if (Glob.Frog.bPositionChanged)
  {
    if (Glob.Frog.s8Position < 0)
    {
      Glob.Frog.s8Position = 0;
    }
    else if (Glob.Frog.s8Position >= 20)
    {
      Glob.Frog.s8Position = 19;
    }
    LCDMessage(LINE2_START_ADDR, Glob.Frog.lines[0]->line_ptr);
    LCDMessage(LINE1_START_ADDR, Glob.Frog.lines[1]->line_ptr);
    if (Glob.Frog.lines[(Glob.Frog.line_stickman == 0) ? 0 : 1]->line_ptr[Glob.Frog.s8Position] == ' ')
    {
      LCDMessage(((Glob.Frog.line_stickman == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + Glob.Frog.s8Position, "X");
      u32Score--;
      Game_StateMachine = Game_GameOver;
    }
    else
    {
      LCDMessage(((Glob.Frog.line_stickman == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + Glob.Frog.s8Position, "\xAB");
    }
    Glob.Frog.bPositionChanged = FALSE;
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
        line->sequence_length = 1 + randInt() % 4; /* 1 to 4 */
      }
      else
      {
        line->sequence_type = LOGS;
        line->sequence_length =  3 + randInt() % 3; /* 3 to 5 */
      }
    }
    if (line->sequence_type == LOGS)
    {
      line->line_ptr[i] = '=';
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
      line->sequence_length = 1 + randInt() % 4; /* 1 to 4 */
    }
    else
    {
      line->sequence_type = LOGS;
      line->sequence_length =  3 + randInt() % 3; /* 3 to 5 */
    }
  }
  if (line->sequence_type == LOGS)
  {
    line->line_ptr[new_index] = '=';
  }
  else
  {
    line->line_ptr[new_index] = ' ';
  }
  line->sequence_length--;
}


static void Runner_StartScreen()
{
  u8 *strLine1 = "          CONTROLS: ";
  u8 *strLine2 = "START      JUMP  ESC";
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
      Glob.Run.au8Cactuses[i] = ' ';
    }
    Glob.Run.au8Cactuses[20] = '\0';
    u32TickLength = 250;
    Glob.Run.u8SequenceLength = 0;
    Glob.Run.SequenceType = GROUND;
    u32Score = 0;
    Glob.Run.bScreenUpdate = TRUE;
    
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
  static u8 u8Jump = 0;
  
  if (WasButtonPressed(BUTTON3))
  {
    ButtonAcknowledge(BUTTON3);
    Game_StateMachine = Game_ConfirmExit;
    Glob.Run.bScreenUpdate = TRUE;
    return;
  }
  
  u32Counter--;
  if (u32Counter == 0)
  {
    u32Counter = u32TickLength;
    if (u32TickLength > 125)
    {
      u32TickLength--;
    }
    u32Score++;
    led_score();
    cactus_update();
    
    if (u8Jump != 0)
    {
      u8Jump--;
    }
    else if (WasButtonPressed(BUTTON2))
    {
      u8Jump = 3;
    }
    ButtonAcknowledge(BUTTON2);              /* Clears button press even if button was pressed while still in air */
    
    Glob.Run.bScreenUpdate = TRUE;
  }
  
  if (Glob.Run.bScreenUpdate)
  {
    LCDMessage(LINE2_START_ADDR, Glob.Run.au8Cactuses);
    
    if (u8Jump == 0)
    {
      LCDMessage(LINE1_START_ADDR + 1, " "); /* Clears stick man if in upper row */
      if (Glob.Run.au8Cactuses[1] == ' ')
      {
        LCDMessage(LINE2_START_ADDR + 1, "\xAB");
      }
      else
      {
        LCDMessage(LINE2_START_ADDR + 1, "X");
        Game_StateMachine = Game_GameOver;
      }
    }
    else
    {
      LCDMessage(LINE1_START_ADDR + 1, "\xAB");
    }
    
    Glob.Run.bScreenUpdate = FALSE;
  }
}


static void Game_GameOver()
{
  static u8 u8Counter = 250;
  static u8 u8TickNumber = 0;
  
  u8Counter--;
  if (u8Counter == 0) 
  {
    u8Counter = 250;
    
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
      for (u8 i = 0; i < 8; i++)
        LedOff((LedNumberType)i);
    }
    else
    {
      for (u8 i = 0; i < 8; i++)
        LedOn((LedNumberType)i);
    }
    
    u8TickNumber++;
  }
  
  /* Go to ScoreBoard */
  if ((u8TickNumber == 8) || WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
  {
    for (u8 i = 0; i < 8; i++)
    {
      LedOff((LedNumberType)i);
    }
    u8TickNumber = 0;
    u8Counter = 0;
    Game_StateMachine = Game_ScoreBoard;
  }
}
static void Game_ScoreBoard()
{
  u8 au8Line1_str[18] = "Score: ";
  u8 au8Line2_str[23] = "High Score: ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    LCDCommand(LCD_CLEAR_CMD);
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    
    to_decimal(&au8Line1_str[7], u32Score);
    if (u32Score > u32HighScore[CurrentGame])
      u32HighScore[CurrentGame] = u32Score;
    to_decimal(&au8Line2_str[12], u32HighScore[CurrentGame]);
    
    LCDMessage(LINE1_START_ADDR, au8Line1_str);
    LCDMessage(LINE2_START_ADDR, au8Line2_str);
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0) || WasButtonPressed(BUTTON1) || WasButtonPressed(BUTTON2) || WasButtonPressed(BUTTON3))
  {
    if (CurrentGame == RUNNER)
    {
      Game_StateMachine = Runner_StartScreen;
    }
    else
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    bFirstEntry = TRUE;
  }
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
    digit = 0;
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
static void cactus_update()
{ 
  for (u8 i = 0; i < 19; i++)               /* Shift Cactuses 1 left)  */
  {
    Glob.Run.au8Cactuses[i] = Glob.Run.au8Cactuses[i + 1];
  }
  if (Glob.Run.u8SequenceLength == 0)
  {
    if (Glob.Run.SequenceType == CACTUS)
    {
      Glob.Run.SequenceType = GROUND;
      Glob.Run.u8SequenceLength = 2 + randInt() % 6; /* Sequences of length 2 to 7 */
    }
    else
    {
      Glob.Run.SequenceType = CACTUS;
      Glob.Run.u8SequenceLength = 1 + randInt() % 3; /* Sequences of length 1 to 3 */
    }
  }
  if (Glob.Run.SequenceType == CACTUS)
  {
    Glob.Run.au8Cactuses[19] = 0x1D;
  }
  else
  {
    Glob.Run.au8Cactuses[19] = ' ';
  }
  Glob.Run.u8SequenceLength--;
}
static u8 randInt()
{
  for (u8 i = 0; i < 8; i++)
  {
    LFSR = (LFSR << 1) | (1u & ((LFSR >> 15) ^ (LFSR >> 14) ^ (LFSR >> 12) ^ (LFSR >> 3)));  /* [16, 15, 13, 4, 0] */
  }
  return (u8)LFSR;
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
