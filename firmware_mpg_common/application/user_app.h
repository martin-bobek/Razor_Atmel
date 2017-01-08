/**********************************************************************************************************************
File: user_app.h                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
1. Follow the instructions at the top of user_app.c
2. Use ctrl-h to find and replace all instances of "user_app" with "yournewtaskname"
3. Use ctrl-h to find and replace all instances of "UserApp" with "YourNewTaskName"
4. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
5. Add #include yournewtaskname.h" to configuration.h
6. Add/update any special configurations required in configuration.h (e.g. peripheral assignment and setup values)
7. Delete this text (between the dashed lines)
----------------------------------------------------------------------------------------------------------------------

Description:
Header file for yournewtaskname.c

**********************************************************************************************************************/

#ifndef __USER_APP_H
#define __USER_APP_H

/**********************************************************************************************************************
Type Definitions
**********************************************************************************************************************/
typedef enum { RUNNER, FROGGER, MEMORY } Game_Type;
typedef enum { CACTUS, GROUND } RunnerSequence_Type;
typedef enum { WATER, LOGS } FroggerSequence_Type;
typedef enum { LEFT, RIGHT } FroggerDirection_Type;
typedef struct { 
  u8 *line_ptr;
  FroggerDirection_Type direction;
  u8 sequence_length;
  FroggerSequence_Type sequence_type; } FroggerLine_Type;
typedef enum { PAUSE, DISPLAY } MemoryOutput_Type;


/**********************************************************************************************************************
Constants / Definitions
**********************************************************************************************************************/


/**********************************************************************************************************************
Function Declarations
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/
void UserAppInitialize(void);
void UserAppRunActiveState(void);


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static void Game_MainMenu();
static void Game_HighScore();
static void Game_ConfirmExit();

static void Memory_StartScreen();
static void Memory_Input();
static void Memory_Output();

static void Frogger_Running();
static void Frogger_StartScreen();

static void new_line(FroggerLine_Type *line);
static void shift_line(FroggerLine_Type *line);

static void Runner_Running();
static void Runner_StartScreen();

static void Game_GameOver();
static void Game_ScoreBoard();

static void cactus_update();
static void led_score();
static void to_decimal(u8 *str, u32 num);
static u8 randInt();

/***********************************************************************************************************************
State Machine Declarations
***********************************************************************************************************************/
static void UserAppSM_Idle(void);    

static void UserAppSM_Error(void);         
static void UserAppSM_FailedInit(void);        


#endif /* __USER_APP_H */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
