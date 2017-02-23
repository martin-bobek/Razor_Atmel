#ifndef KEYBOARD_H
#define KEYBOARD_H

#define PS2_CLOCK_PIN         (u32)15
#define PS2_DATA_PIN          (u32)13
#define PS2_CLOCK_MSK         (u32)(1 << PS2_CLOCK_PIN)
#define PS2_DATA_MSK          (u32)(1 << PS2_DATA_PIN)

#define MOD_CAPS_LOCK         (u8)(1 << 0)
#define MOD_NUM_LOCK          (u8)(1 << 1)
#define MOD_SCROLL_LOCK       (u8)(1 << 2)
#define MOD_LSHIFT            (u8)(1 << 3)
#define MOD_RSHIFT            (u8)(1 << 4)
#define MOD_S_UPPER           (u8)(1 << 5)
#define MOD_SCL_UPPER         (u8)(1 << 6)


#define PS2_BUFFER_SIZE       4
#define CHAR_BUFFER_SIZE      16

/* Protected Globals */
extern volatile fnCode_type PS2_Handler;

/* Private Types */
typedef enum { NOMOD, BRK, ALT0, ALT1, ALT0BRK } PS2Alt_t;

/* Public Functions */
u8 KeyboardData(void);

/* Protected Functions */
void KeyboardInitialize(void);
void KeyboardRunActiveState(void);

void PS2Start_ReceiveState(void);
void PS2Data_ReceiveState(void);
void PS2Parity_ReceiveState(void);
void PS2Stop_ReceiveState(void);

/* Private Functions */
static void PushChar(u8 c);
static void ComputeUpper(void);

#endif