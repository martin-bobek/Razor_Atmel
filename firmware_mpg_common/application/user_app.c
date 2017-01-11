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

static bool bSound_Active;
static u16 u16Sound_Duration;

static Game_Type CurrentGame = RUNNER;              /* Used for modifying behaviour of general purpose functions */
static u32 u32TickLength;                           /* Initial game tick length */

static u32 u32Score = 0;                            /* Number of gameticks the player has survived */
static u32 u32HighScore[3] = { 0, 0, 0 };

static u8 au8Cactuses[21];                          /* Characters for the bottom row of LCD */
static u16 LFSR;                                    /* Linear Feedback Shift Register for randInt */
static bool bLFSRinitialized = FALSE;               /* Indicates LFSR has not been initialized with seed */
static u8 u8SequenceLength;                         /* Remaining length of sequence of cactuses/spaces */
static RunnerSequence_Type SequenceType;            /* True indicates current sequence is cactuses */
static bool bScreenUpdate;
static u8 u8Jump;

static s8 s8Position;
static u8 au8Line1[21];
static u8 au8Line2[21];
static FroggerLine_Type Line1, Line2;
static FroggerLine_Type *lines[2];
static u32 u32Counter;
static u8 line_stickman = 0;
static bool bPositionChanged;

static u8 au8Memory_Sequence[125];
static u32 u32Memory_SequenceLength;
static u32 u32Memory_PauseLength;
static u32 u32Memory_SequencePosition;

/*
Tested Change Log:

Adjust cactus spacing, speed and jump length in Runner.
Adjust log spacing and speed in Frogger.
Add speed up to Memory Game as game progresses.
Adjust timing between transition from Memory_Input to Memory_Output.
Add escape option during Memory Input.
Make the screen blue for Frogger (water).
Add Led score system to Frogger and Memory.
Add jump sounds to Runner.
Add sounds to Memory Game.
Add losing sound to GameOver.
Add sounds to Frogger.
*/

/*
Untested changes since last merge:

*/

/*
To Do:

*/

/* 
Known Bugs:

During frogger_running and memory_running, glitched characters sometimes appear.
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
static void Sound_Service()
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

static void Game_MainMenu()
{
  u8 *au8Controls = "SCORE  \x7F    \x7E  ENTER";
  u8 *au8Runner = "       RUNNER       ";
  u8 *au8Frogger = "      FROGGER       ";
  u8 *au8Memory = "    MEMORY GAME     ";
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
    else if (CurrentGame == FROGGER)
    {
      LCDMessage(LINE1_START_ADDR, au8Frogger);
    }
    else
    {
      LCDMessage(LINE1_START_ADDR, au8Memory);
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
    else if (CurrentGame == FROGGER)
    {
      Game_StateMachine = Frogger_StartScreen;
    }
    else
    {
      Game_StateMachine = Memory_StartScreen;
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
    else if (CurrentGame == MEMORY)
    {
      CurrentGame = FROGGER;
      LCDMessage(LINE1_START_ADDR, au8Frogger);
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
    else if (CurrentGame == FROGGER)
    {
      CurrentGame = MEMORY;
      LCDMessage(LINE1_START_ADDR, au8Memory);
    }
  }
}
static void Game_HighScore()
{
  u8 au8Score_str1[] = "High Score:";
  u8 au8Score_str2[] = "          ";
  static bool bFirstEntry = TRUE;
  
  if (bFirstEntry)
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDCommand(LCD_CLEAR_CMD);
    
    to_decimal(au8Score_str2, u32HighScore[CurrentGame]);
    LCDMessage(LINE1_START_ADDR, au8Score_str1);
    LCDMessage(LINE2_START_ADDR + 6, au8Score_str2);
    
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
  u8 au8Line1_str[18] = "Score: ";
  u8 au8Line2_str[17] = "High: ";
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
    to_decimal(&au8Line2_str[6], u32HighScore[CurrentGame]);
    
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
  static u8 *strLine1 = " REPEAT THE PATTERN ";
  static u8 *strLine2 = "START            ESC";
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
        LCDMessage(LINE1_START_ADDR, "\x23");
        LCDMessage(LINE2_START_ADDR, "\x23");
        break;
      case BUTTON1:
        PWMAudioSetFrequency(BUZZER1, D5);
        LCDMessage(LINE1_START_ADDR + 6, "\x23");
        LCDMessage(LINE2_START_ADDR + 6, "\x23");
        break;
      case BUTTON2:
        PWMAudioSetFrequency(BUZZER1, E5);
        LCDMessage(LINE1_START_ADDR + 13, "\x23");
        LCDMessage(LINE2_START_ADDR + 13, "\x23");
        break;
      case BUTTON3:
        PWMAudioSetFrequency(BUZZER1, G5S);
        LCDMessage(LINE1_START_ADDR + 19, "\x23");
        LCDMessage(LINE2_START_ADDR + 19, "\x23");
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
      
      switch ((au8Memory_Sequence[u32Memory_SequencePosition / 4] >> (2 * (u32Memory_SequencePosition % 4))) % 4)
      {
      case 0:
        timed_sound(C5, 300);
        LCDMessage(LINE1_START_ADDR, "\x23");
        LCDMessage(LINE2_START_ADDR, "\x23");
        break;
      case 1:
        timed_sound(D5, 300);
        LCDMessage(LINE1_START_ADDR + 6, "\x23");
        LCDMessage(LINE2_START_ADDR + 6, "\x23");
        break;
      case 2:
        timed_sound(E5, 300);
        LCDMessage(LINE1_START_ADDR + 13, "\x23");
        LCDMessage(LINE2_START_ADDR + 13, "\x23");
        break;
      case 3:
        timed_sound(G5S, 300);
        LCDMessage(LINE1_START_ADDR + 19, "\x23");
        LCDMessage(LINE2_START_ADDR + 19, "\x23");
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
      au8Line1[i] = '=';
    Line1 = (FroggerLine_Type){ au8Line1, LEFT, 1, LOGS };
    Line2 = (FroggerLine_Type){ au8Line2, RIGHT, 0, WATER };
    new_line(&Line2);
    lines[0] = &Line1;
    lines[1] = &Line2;
    
    u32Score = 0;
    s8Position = 10;
    u32TickLength = 500;
    u32Counter = u32TickLength;
    line_stickman = 0;
    bPositionChanged = FALSE;
    
    bFirstEntry = FALSE;
  }
  
  if (WasButtonPressed(BUTTON0)) /* Go to Running state of Frogger */
  {
    ButtonAcknowledge(BUTTON0);
    ButtonAcknowledge(BUTTON1);
    ButtonAcknowledge(BUTTON2);
    ButtonAcknowledge(BUTTON3);
    LCDMessage(LINE2_START_ADDR, lines[0]->line_ptr);
    LCDMessage(LINE2_START_ADDR + 10, "\xAB");
    LCDMessage(LINE1_START_ADDR, lines[1]->line_ptr);
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
    bPositionChanged = TRUE;
    return;
  }
  
  u32Counter--;
  if (u32Counter == 0)
  {
    u32Counter = u32TickLength;
    if (line_stickman == 1)
    {
      line_stickman = 0;
      FroggerLine_Type *temp = lines[0];
      lines[0] = lines[1];
      new_line(temp);
      lines[1] = temp;
    }
    if (line_stickman > 1)
      line_stickman--;
    if (u32Score != 0)
    {
      if (lines[(line_stickman == 0) ? 0 : 1]->direction == RIGHT)
      {
        s8Position++;
      }
      else
      {
        s8Position--;
      }
      shift_line(lines[0]);
    }
    shift_line(lines[1]);
    bPositionChanged = TRUE;
  }
  
  if (WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);
    if (line_stickman == 0)
    {
      timed_sound(C6, 50);
      u32Score++;
      bScoreChanged = TRUE;
      if (u32TickLength > 200)
      {
        u32TickLength = 150000/(300 + 3*u32Score);
      }
      bPositionChanged = TRUE;
      line_stickman = 2;
    }
  }
  if (WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    timed_sound(C6, 50);
    s8Position--;
    bPositionChanged = TRUE;
  }
  if (WasButtonPressed(BUTTON2))
  {
    ButtonAcknowledge(BUTTON2);
    timed_sound(C6, 50);
    s8Position++;
    bPositionChanged = TRUE;
  }
  
  if (bPositionChanged)
  {
    if (s8Position < 0)
    {
      s8Position = 0;
    }
    else if (s8Position >= 20)
    {
      s8Position = 19;
    }
    LCDMessage(LINE2_START_ADDR, lines[0]->line_ptr);
    LCDMessage(LINE1_START_ADDR, lines[1]->line_ptr);
    if (lines[(line_stickman == 0) ? 0 : 1]->line_ptr[s8Position] == ' ')
    {
      LCDMessage(((line_stickman == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + s8Position, "X");
      u32Score--;
      bScoreChanged = FALSE;
      Game_StateMachine = Game_GameOver;
      
    }
    else
    {
      LCDMessage(((line_stickman == 0) ? LINE2_START_ADDR : LINE1_START_ADDR) + s8Position, "\xAB");
    }
    bPositionChanged = FALSE;
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
      au8Cactuses[i] = ' ';
    }
    au8Cactuses[20] = '\0';
    u32TickLength = 250;
    u8SequenceLength = 0;
    SequenceType = GROUND;
    u32Score = 0;
    bScreenUpdate = TRUE;
    u8Jump = 0;
    
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
    bScreenUpdate = TRUE;
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
    
    if (u8Jump != 0)
    {
      u8Jump--;
    }
    else if (WasButtonPressed(BUTTON2))
    {
      u8Jump = 4;
      timed_sound(C6, 50);
    }
    ButtonAcknowledge(BUTTON2);              /* Clears button press even if button was pressed while still in air */
    
    bScreenUpdate = TRUE;
  }
  
  if (bScreenUpdate)
  {
    LCDMessage(LINE2_START_ADDR, au8Cactuses);
    
    if (u8Jump == 0)
    {
      LCDMessage(LINE1_START_ADDR + 1, " "); /* Clears stick man if in upper row */
      if (au8Cactuses[1] == ' ')
      {
        LCDMessage(LINE2_START_ADDR + 1, "\xAB");
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
      LCDMessage(LINE1_START_ADDR + 1, "\xAB");
    }
    
    bScreenUpdate = FALSE;
  }
}

static void cactus_update()
{ 
  for (u8 i = 0; i < 19; i++)               /* Shift Cactuses 1 left)  */
  {
    au8Cactuses[i] = au8Cactuses[i + 1];
  }
  if (u8SequenceLength == 0)
  {
    if (SequenceType == CACTUS)
    {
      SequenceType = GROUND;
      u8SequenceLength = 4 + randInt() % 4; /* Sequences of length 4 to 7 */
    }
    else
    {
      SequenceType = CACTUS;
      u8SequenceLength = 1 + randInt() % 3; /* Sequences of length 1 to 3 */
    }
  }
  if (SequenceType == CACTUS)
  {
    au8Cactuses[19] = 0x1D;
  }
  else
  {
    au8Cactuses[19] = ' ';
  }
  u8SequenceLength--;
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
    LFSR = (LFSR << 1) | (1u & ((LFSR >> 15) ^ (LFSR >> 14) ^ (LFSR >> 12) ^ (LFSR >> 3)));  /* [16, 15, 13, 4, 0] */
  }
  return (u8)LFSR;
}

/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

extern u32 G_u32MessagingFlags;

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserAppSM_Idle(void)
{
  if (G_u32MessagingFlags & _MESSAGING_TX_QUEUE_ALMOST_FULL)
  {
    DebugPrintf("Queue Almost Full!\n\r");
  }
  if (G_u32MessagingFlags & _MESSAGING_TX_QUEUE_FULL)
  {
    DebugPrintf("Queue Full!\n\r");
  }
  
  Game_StateMachine();
  Sound_Service();
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