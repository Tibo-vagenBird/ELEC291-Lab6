#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>

// Timing constants (must match the transmitter's Timer 2 period).
//
// Transmitter (EFM8LB1 @ 76 MHz, TIMER2_RELOAD = 46588):
//   period = (65536 - 46588) / 76 000 000 = 18948 / 76e6 ≈ 249 µs per state
//
// Receiver (ATMega328P @ 16 MHz, Timer0 prescaler 64):
//   1 tick = 64 / 16 000 000 = 4 µs
//   ticks per state = 249 / 4 ≈ 62 ticks
#define IR_TICKS_PER_STATE  62   // one full transmitter state period
#define IR_HALF_STATE       31   // midpoint — where we sample
#define IR_TIMEOUT_TICKS   250   // ~1 ms timeout waiting for start burst

// IR receiver module output polarity (e.g. TSOP1838):
//   LOW  = 38 kHz carrier detected (burst)
//   HIGH = no carrier (silence)
#define IR_IS_BURST()  (!(PINB & (1 << PINB2)))

// FSM states — numbered to match the transmitter states directly.
typedef enum {
    RX_IDLE      = 0,   // waiting for a falling edge (start of transmission)
    RX_STATE_0   = 1,   // verify start burst
    RX_STATE_1   = 2,   // data bit 0 (LSB)
    RX_STATE_2   = 3,   // data bit 1
    RX_STATE_3   = 4,   // data bit 2
    RX_STATE_4   = 5,   // data bit 3
    RX_STATE_5   = 6,   // data bit 4
    RX_STATE_6   = 7,   // data bit 5
    RX_STATE_7   = 8,   // data bit 6
    RX_STATE_8   = 9,   // data bit 7 (MSB)
    RX_STATE_9   = 10,  // verify stop burst
    RX_STATE_10  = 11,  // verify final silence
    RX_DONE      = 12,
    RX_ERROR     = 13,
    RX_TIMEOUT   = 14
} rx_state_t;

// Blocking receive: runs the FSM until a full byte is received or an error occurs.
// Returns 1 on success  (*out holds the received byte).
// Returns 0 on timeout (no carrier seen within IR_TIMEOUT_TICKS).
// Returns -1 on framing error (start/stop burst or final silence missing).
int8_t IR_Receive(uint8_t *out);

#endif
