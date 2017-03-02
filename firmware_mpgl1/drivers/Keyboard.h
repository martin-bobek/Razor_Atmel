#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Special Chars */
typedef enum {
      ENT_ = 0x80,  // Enter
      BKS_       ,  // Backspace
      SCR_       ,  // Scroll Lock
      NUM_       ,  // Num Lock
      CPS_       ,  // Caps Lock
      LSH_       ,  // Left Shift
      RSH_       ,  // Right Shift
      LCT_       ,  // Left Control
      LAL_       ,  // Left Alt
      ESC_       ,  // Escape
      F01_       ,  // F1
      F02_       ,  // F2
      F03_       ,  // F3
      F04_       ,  // F4
      F05_       ,  // F5
      F06_       ,  // F6
      F07_       ,  // F7
      F08_       ,  // F8
      F09_       ,  // F9
      F10_       ,  // F10
      F11_       ,  // F11
      F12_       ,  // F12
      KEN_ = ENT_,  // Keypad Enter
      KMU_ = '*' ,  // Keypad *
      KSU_ = '-' ,  // Keypad -
      KAD_ = '+' ,  // Keypad +
      KPE_ = '.' ,  // Keypad .
      KSL_ = '/' ,  // Keypad /
      KP0_ = '0' ,  // Keypad 0
      KP1_ = '1' ,  // Keypad 1
      KP2_ = '2' ,  // Keypad 2
      KP3_ = '3' ,  // Keypad 3
      KP4_ = '4' ,  // Keypad 4
      KP5_ = '5' ,  // Keypad 5
      KP6_ = '6' ,  // Keypad 6
      KP7_ = '7' ,  // Keypad 7
      KP8_ = '8' ,  // Keypad 8
      KP9_ = '9' ,  // Keypad 9
    } SpecialChar_t;

/* Private Macros */
#define PS2_CLOCK_PIN         (u32)15
#define PS2_DATA_PIN          (u32)13
#define PS2_CLOCK_MSK         (u32)(1 << PS2_CLOCK_PIN)
#define PS2_DATA_MSK          (u32)(1 << PS2_DATA_PIN)

#define TR_RESEND             (u8)(1 << 0)
#define TR_LOCK_UPDATE        (u8)(1 << 1)

#define MOD_SCROLL_LOCK       (u8)(1 << 0)
#define MOD_NUM_LOCK          (u8)(1 << 1)
#define MOD_CAPS_LOCK         (u8)(1 << 2)
#define MOD_LSHIFT            (u8)(1 << 3)
#define MOD_RSHIFT            (u8)(1 << 4)
#define MOD_SHIFT             (u8)(1 << 5)

#define PS2_RBUFFER_SIZE      4
#define PS2_TBUFFER_SIZE      4
#define CHAR_BUFFER_SIZE      16

/* Protected Globals */
extern volatile fnCode_type PS2_Handler;

/* Private Types */
typedef enum { NOMOD, BRK, ALT0, ALT1, ALT0BRK } PS2Alt_t;
typedef enum { NO_UPDATE, CMD_PEND, CMD_SENT, DATA_SENT, ABORT } LockUpdateState_t;
//typedef enum { NO_RESEND, REQ_PEND } DResendState_t;

/* Interrupt Handlers */
void TC0_IrqHandler(void);

/* Public Functions */
u8 KeyboardData(void);

/* Protected Functions */
void KeyboardInitialize(void);
void KeyboardRunActiveState(void);

void PS2Start_ReceiveState(void);
void PS2Data_ReceiveState(void);
void PS2Parity_ReceiveState(void);
void PS2Stop_ReceiveState(void);

void PS2Inhibit_TransmitState(void);
void PS2Start_TransmitState(void);
void PS2Data_TransmitState(void);
void PS2Parity_TransmitState(void);
void PS2Stop_TransmitState(void);
void PS2Acknowledge_TransmitState(void);

void PS2Empty_State(void);

/* Private Functions */
static void PushChar(u8 c);
//static void PushLockUpdate(void);
static void ComputeUpdate(void);

#endif