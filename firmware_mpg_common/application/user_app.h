/**********************************************************************************************************************
File: user_app1.h                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
1. Follow the instructions at the top of user_app1.c
2. Use ctrl-h to find and replace all instances of "user_app1" with "yournewtaskname"
3. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
4. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
5. Add #include yournewtaskname.h" to configuration.h
6. Add/update any special configurations required in configuration.h (e.g. peripheral assignment and setup values)
7. Delete this text (between the dashed lines)
----------------------------------------------------------------------------------------------------------------------

Description:
Header file for user_app1.c

**********************************************************************************************************************/

#ifndef __USER_APP1_H
#define __USER_APP1_H

/**********************************************************************************************************************
Type Definitions
**********************************************************************************************************************/
typedef enum { RUNNER, FROGGER, MEMORY, CHAT } Game_Type;
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
#define CHAT_NUM_LINES                      20

#define BUTTON0_PIN                         17
#define BUTTON0_MSK                         (u32)(1 << BUTTON0_PIN)

#define TIMEOUT_VALUE                       (u32)5000

#define ANT_CHANNEL_USERAPP                 (u8)0
#define ANT_SERIAL_LO_USERAPP               (u8)0x54
#define ANT_SERIAL_HI_USERAPP               (u8)0x70
#define ANT_DEVICE_TYPE_USERAPP             (u8)1
#define ANT_TRANSMISSION_TYPE_USERAPP       (u8)1
#define ANT_CHANNEL_PERIOD_LO_USERAPP       (u8)0xcd
#define ANT_CHANNEL_PERIOD_HI_USERAPP       (u8)0x0c
#define ANT_FREQUENCY_USERAPP               (u8)50
#define ANT_TX_POWER_USERAPP                RADIO_TX_POWER_0DBM

#define PACKET_LENGTH                       8
#define TEXT_MESSAGE_LENGTH                 112
#define ACK_TIMEOUT                         950
#define CODE_MSK                            (u8)0xe0
#define MSG_CODE                            (u8)0xe0
#define END_CODE                            (u8)0xa0
#define ACK_CODE                            (u8)0x80
#define RQT_CODE                            (u8)0xc0
#define RSD_CODE                            (u8)0x60
#define PACKET_NUM_MSK                      (u8)0x0f
#define MSG_EO_BIT                          (u8)0x10
#define CODE_BYTE                           0


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
static void Chat_Initialize(void);
static void ServiceIncoming(void);
static bool IsPunctuation(u8 u8Char);

static void Sound_Service(void);

static void new_line(FroggerLine_Type *line);
static void shift_line(FroggerLine_Type *line);

static void cactus_update(void);

static void timed_sound(u16 note, u16 duration_ms);
static void led_score(void);
static void to_decimal(u8 *str, u32 num);
static u8 randInt(void);

/***********************************************************************************************************************
State Machine Declarations
***********************************************************************************************************************/
static void Game_MainMenu(void);
static void Game_Chat(void);
static void Game_PrintIncoming(void);
static void Game_HighScore(void);
static void Game_ConfirmExit(void);
static void Game_GameOver(void);
static void Game_ScoreBoard(void);

static void Memory_StartScreen(void);
static void Memory_Input(void);
static void Memory_Output(void);

static void Frogger_Running(void);
static void Frogger_StartScreen(void);

static void Runner_Running(void);
static void Runner_StartScreen(void);

static void ANT_Master(void);
static void ANT_SlaveWaitChannelOpen(void);
static void ANT_SlaveChannelOpen(void);
static void ANT_SlaveWaitChannelClose(void);

static void AntDecode(void);
static void AntGeneratePacket(u8 *pDataPacket);

static void ANT_Error(void);         
static void ANT_FailedInit(void);        

#endif /* __USER_APP1_H */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
