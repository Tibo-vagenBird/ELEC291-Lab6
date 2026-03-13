#include <avr/io.h>
#include "ir_receiver.h"

// ---- Timer0 helpers -------------------------------------------------

static void timer0_start(void)
{
    TCCR0A = 0x00;
    TCCR0B = (1 << CS01) | (1 << CS00); // prescaler 64 -> 4 us/tick at 16 MHz
    TCNT0  = 0;
}

static void timer0_stop(void)
{
    TCCR0B = 0x00;
}

// Delay exactly <ticks> Timer0 ticks from now.
// Resets TCNT0 so each call is relative to the moment it is entered.
static void wait_ticks(uint8_t ticks)
{
    TCNT0 = 0;
    while (TCNT0 < ticks);
}

// ---- IR receive FSM -------------------------------------------------
//
// The transmitter (EFM8LB1) drives the IR LED through these states on
// every Timer 2 interrupt:
//
//   State  0  : carrier ON  (start burst)
//   State  1-8: carrier ON if bit(state-1) of byte is 1, else OFF  (LSB first)
//   State  9  : carrier ON  (stop burst)
//   State 10  : carrier OFF (final silence)
//
// The IR receiver module (e.g. TSOP1838) inverts: LOW = carrier present.
// We synchronise to the falling edge that marks the start of state 0,
// then sample PB2 at the midpoint of each subsequent state.
//
//   Timing from falling edge (t = 0):
//     State 0  midpoint : t = HALF_STATE               (31 ticks =  124 us)
//     State k  midpoint : t = HALF_STATE + k * TICKS_PER_STATE
//     State 10 midpoint : t = 31 + 10*62 = 651 ticks  (2604 us)

int8_t IR_Receive(uint8_t *out)
{
    rx_state_t  state    = RX_IDLE;
    uint8_t     rx_byte  = 0;
    uint8_t     bit_idx;

    // PB2 as input, no pull-up (IR receiver drives the line)
    DDRB  &= ~(1 << DDB2);
    PORTB &= ~(1 << PORTB2);

    timer0_start();

    while (state != RX_DONE && state != RX_ERROR && state != RX_TIMEOUT)
    {
        switch (state)
        {
            // --- RX_IDLE: wait for a falling edge (carrier starts = PB2 goes LOW) ---
            // TCNT0 is 8-bit (max 255 ticks = ~1ms at 4us/tick).
            // One IR frame is ~3ms, so we use a software iteration counter to
            // extend the total wait to ~10ms (covers >3 full frame periods),
            // giving us enough time to always catch at least one start burst.
            case RX_IDLE:
            {
                uint8_t iter = 10; // 10 x ~1ms = ~10ms total timeout
                TCNT0 = 0;
                while (!IR_IS_BURST())
                {
                    if (TCNT0 >= IR_TIMEOUT_TICKS)
                    {
                        TCNT0 = 0;          // reset for next milli-second window
                        if (--iter == 0)
                        {
                            state = RX_TIMEOUT;
                            break;
                        }
                    }
                }
                if (state == RX_IDLE)
                    state = RX_STATE_0; // falling edge detected
                break;
            }

            // --- RX_STATE_0: sample at midpoint of start burst ---
            // Transmitter always sends carrier here; anything else is noise.
            case RX_STATE_0:
                wait_ticks(IR_HALF_STATE);
                if (!IR_IS_BURST())
                    state = RX_ERROR;   // expected burst, got silence
                else
                    state = RX_STATE_1;
                break;

            // --- RX_STATE_1..8: sample each data bit ---
            // Each state: wait one full period from the previous sample,
            // then read the bit. Carrier present (LOW) = 1, silence = 0.
            case RX_STATE_1:
            case RX_STATE_2:
            case RX_STATE_3:
            case RX_STATE_4:
            case RX_STATE_5:
            case RX_STATE_6:
            case RX_STATE_7:
            case RX_STATE_8:
                wait_ticks(IR_TICKS_PER_STATE);
                bit_idx = (uint8_t)state - (uint8_t)RX_STATE_1; // 0..7
                if (IR_IS_BURST())
                    rx_byte |= (1 << bit_idx); // LSB first, matches transmitter
                state = (rx_state_t)((uint8_t)state + 1);
                break;

            // --- RX_STATE_9: sample stop burst ---
            // Transmitter always sends carrier here.
            case RX_STATE_9:
                wait_ticks(IR_TICKS_PER_STATE);
                if (!IR_IS_BURST())
                    state = RX_ERROR;   // expected burst, got silence
                else
                    state = RX_STATE_10;
                break;

            // --- RX_STATE_10: sample final silence ---
            // Transmitter turns carrier OFF here.
            case RX_STATE_10:
                wait_ticks(IR_TICKS_PER_STATE);
                if (IR_IS_BURST())
                    state = RX_ERROR;   // expected silence, got carrier
                else
                    state = RX_DONE;
                break;

            default:
                state = RX_ERROR;
                break;
        }
    }

    timer0_stop();

    if (state == RX_DONE)
    {
        *out = rx_byte;
        return 1;
    }
    if (state == RX_TIMEOUT)
        return 0;

    return -1; // framing error
}
