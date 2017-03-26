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
#define CODE_MSK                            (u8)0xe0
#define MSG_CODE                            (u8)0xe0
#define END_CODE                            (u8)0xa0
#define ACK_CODE                            (u8)0x80
#define PACKET_NUM_MSK                      (u8)0x0f
#define MSG_EO_BIT                          (u8)0x10


/**********************************************************************************************************************
Function Declarations
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/
void UserApp1Initialize(void);
void UserApp1RunActiveState(void);


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/
static void MessageInitialize(void);
static void MessageService(void);

static bool IsPunctuation(u8 u8Char);

/***********************************************************************************************************************
State Machine Declarations
***********************************************************************************************************************/
static void UserApp1SM_Master(void);

static void UserApp1SM_SlaveIdle(void);
static void UserApp1SM_SlaveWaitChannelOpen(void);
static void UserApp1SM_SlaveChannelOpen(void);
static void UserApp1SM_SlaveWaitChannelClose(void);

static void AntParse(void);

static void UserApp1SM_Error(void);         
static void UserApp1SM_FailedInit(void);        


#endif /* __USER_APP1_H */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
