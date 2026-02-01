#pragma once
#include <cstdint>

namespace nes {
// From https://www.nesdev.org/wiki/APU_Envelope
// In a synthesizer, an envelope is the way a sound's parameter changes over time.
// The NES APU has an envelope generator that controls the volume in one of two ways:
// it can generate a decreasing saw envelope (like a decay phase of an ADSR) with optional looping,
// or it can generate a constant volume that a more sophisticated software envelope generator
// can manipulate.
// Volume values are practically linear (see: APU Mixer).
// Each volume envelope unit contains the following: start flag, divider, and decay level counter.
// When clocked by the frame counter, one of two actions occurs:
// if the start flag is clear, the divider is clocked,
// otherwise the start flag is cleared, the decay level counter is loaded with 15, and
// the divider's period is immediately reloaded.
// When the divider is clocked while at 0, it is loaded with V and clocks the decay level counter.
// Then one of two actions occurs:
// If the counter is non-zero, it is decremented,
// otherwise if the loop flag is set, the decay level counter is loaded with 15.
// The envelope unit's volume output depends on the constant volume flag:
// if set, the envelope parameter directly sets the volume,
// otherwise the decay level is the current volume.
// The constant volume flag has no effect besides selecting the volume source;
// the decay level will still be updated when constant volume is selected.
// Each of the three envelope units' output is fed through additional gates at
// the sweep unit (pulse only),
// waveform generator (sequencer or LFSR), and
// length counter.
//                                    Loop flag
//                                         |
//                Start flag  +.   |   Constant volume
//                            |        |   |        flag
//                            v        v   v          |
// Quarter frame clock --> Divider --> Decay --> |    |
//                            ^        level     |    v
//                            |                  | Select --> Envelope output
//                            |                  |
//         Envelope parameter +> |

class Envelope {
public:
    //  Configuration (set by CPU writes to $4000/$4004) 
    bool loop_flag = false;           // Also serves as length counter halt
    bool constant_volume_flag = false; // true = use volume directly, false = use decay
    uint8_t volume_period = 0;        // 4 bits: either constant volume OR envelope period

    //  Internal state 
    bool start_flag = false;          // Set when note starts, triggers envelope restart
    uint8_t decay_level = 0;          // Current envelope volume (counts down 15->0)
    uint8_t divider = 0;              // Divider countdown (reloads from volume_period)

    // Clock the envelope (called on quarter-frame)
    void clock();

    // Get current output volume (0-15)
    uint8_t output() const;

    // Reset the envelope state
    void reset();
};


// The sweep unit automatically adjusts the channel's period (frequency) over
// time, creating pitch bends. It can sweep the pitch UP (decreasing period)
// or DOWN (increasing period).
// From https://www.nesdev.org/wiki/APU_Sweep
//    The sweep unit continuously calculates each pulse channel's target period in this way:
//    Reads the pulse channel's 11-bit raw timer period
//    Shifts timer period right by the shift count, producing the change amount.
//    If the negate flag is true, the change amount is made negative.
//    The target period is the sum of the current period and the change amount, clamped to zero if this sum is negative.
//    For example, if the negate flag is false and the shift amount is zero, the change amount equals the current
//    period, making the target period equal to twice the current period.
//    The two pulse channels have their adders' carry inputs wired differently, which produces different results
//    when each channel's change amount is made negative:
//    Pulse 1 adds the ones' complement (−c − 1). Making 20 negative produces a change amount of −21.
//    Pulse 2 adds the two's complement (−c). Making 20 negative produces a change amount of −20.
//    Whenever the current period or sweep setting changes, whether by $400x writes or by sweep updating the period,
//    the target period also changes.

class SweepUnit {
public:
    //  configuration set by CPU writes to $4001/$4005
    bool enabled = false;             // sweep on/off
    uint8_t period = 0;               // divider period (3 bits, 0-7)
    bool negate = false;              // false = add (pitch down), true = subtract (pitch up)
    uint8_t shift = 0;                // 3-bit shift amount for period adjustment SSS = 0 behaves like enabled = false

    //  internal state
    bool reload_flag = false;         // set when sweep register written
    uint8_t divider = 0;              // current divider countdown

    //  channel identity (affects negation calculation)
    bool is_pulse1 = true;            // Pulse 1 uses one's complement negation

    // calculate target period given current period
    // returns the period the channel would have after one sweep adjustment
    uint16_t calculate_target_period(uint16_t current_period) const;

    // check if channel should be muted due to sweep
    // muting occurs if period < 8 OR target period > 0x7FF
    bool is_muting(uint16_t current_period) const;

    // clock the sweep unit called on half-frame
    // returns true if timer period should be updated
    bool clock(uint16_t& timer_period);

    // reset all sweep state
    void reset();
};

// From https://www.nesdev.org/wiki/APU_Length_Counter
// The length counter controls how long a note plays. When a note starts,
// the counter is loaded with a value from a lookup table. The counter then
// counts down once per frame-counter half-frame. When it reaches 0, the
// channel is silenced.
//
// This allows the game to play a note of a specific duration without needing
// to manually turn it off - useful for sound effects and music timing.
//
// The length counter can be "halted" (paused) by setting the halt flag.
// When halted, it stops counting and the note sustains indefinitely.
// This is controlled by the same bit as the envelope loop flag.
//
// LENGTH COUNTER LOOKUP TABLE:
// The table contains values designed for musical timing. The game writes
// a 5-bit index (0-31) and gets a specific counter value.
//
// Index: Value pairs (values represent frame counter half-frames):
//   0x00: 10    0x01: 254   0x02: 20    0x03: 2
//   0x04: 40    0x05: 4     0x06: 80    0x07: 6
//   ... and so on (see implementation for full table)

class LengthCounter {
public:
    bool halt = false;                // when true, counter doesn't decrement

    uint8_t counter = 0;              // current counter value

    // load counter from lookup table called when $4003/$4007 written
    void load(uint8_t index);

    // clock the counter called on half-frame
    void clock();

    // check if counter is non-zero aka channel active
    bool is_active() const { return counter > 0; }

    // force counter to 0 when channel disabled via $4015
    void clear() { counter = 0; }

    // reset all state
    void reset();

    // static lookup table for length counter values
    static const uint8_t LENGTH_TABLE[32];
};


// From https://www.nesdev.org/wiki/APU_Pulse
// The complete pulse channel combining all the above components:
//
//                  Sweep -----> Timer
//                    |            |
//                    |            | clocks
//                    |            v
//                    |        Sequencer   Length Counter
//                    |            |             |
//                    |            | 0 or 1      |
//                    v            v             v
// Envelope -------> Gate -----> Gate -------> Gate --->(to mixer)
// multiplies by
// volume 0-15
//
//   Gate output is 0 if any is true:
//    Sequencer output is 0
//    Length counter is 0
//    Sweep unit is muting
//
//   Timer runs at CPU clock / 2 (894.886 kHz on NTSC)
//   Period of 0 = timer runs every 2 CPU cycles
//   Audible range: ~54 Hz (period 0x7FF) to ~12.4 kHz (period 8)
//   Periods < 8 are muted (ultrasonic, would alias badly)
//
// Registers:
//   $4000/$4004: DDLC VVVV  Duty, Length halt, Constant vol, Volume/Period
//   $4001/$4005: EPPP NSSS  Sweep Enable, Period, Negate, Shift
//   $4002/$4006: TTTT TTTT  Timer low 8 bits
//   $4003/$4007: LLLL LTTT  Length counter load, Timer high 3 bits
//
class PulseChannel {
public:
    Envelope envelope;
    SweepUnit sweep;
    LengthCounter length_counter;

    // Timer controls frequency
    // The timer is an 11-bit value (0-2047). The actual period in CPU cycle is (timer_period + 1) * 2 where
    // lower values = higher frequency.
    uint16_t timer_period = 0;        // 11-bit period value from registers $4002 + lower 3 bits of $4003 for pulse 1
                                      // and $4006 (low) + $4007's lower 3 bits (high) for pulse 2

    // countdown timer that reloads from table and clocks the shift register on 0
    uint16_t timer_counter = 0;

    //  Duty cycle sequencer 
    // (8-step sequence, 4 different duty patterns)
    //
    //   12.5%: 0 1 0 0 0 0 0 0  (1 high, 7 low)
    //   25%:   0 1 1 0 0 0 0 0  (2 high, 6 low)
    //   50%:   0 1 1 1 1 0 0 0  (4 high, 4 low)
    //   75%:   1 0 0 1 1 1 1 1  (6 high, 2 low)
    // The sequencer steps through 8 positions, outputting 0 or 1 based on
    // the selected duty cycle pattern.
    uint8_t duty_mode = 0;            // 0-3: selects one of 4 duty patterns
    uint8_t sequencer_position = 0;   // 0-7: current position in sequence

    //  channel state
    bool enabled = false;             // set via $4015 status register

    // Duty cycle lookup table
    // Each row is a duty pattern, each column is a sequencer position.
    // The sequencer counts down and loops (7,6,5,4,3,2,1,0,7,6,5...)
    static const uint8_t DUTY_TABLE[4][8];

    // specify if this is pulse 1 (affects sweep negation)
    explicit PulseChannel(bool is_pulse1 = true);

    // write to channel registers
    void write_register(uint8_t reg, uint8_t value);

    // clock the timer called every APU cycle = every 2 CPU cycles
    void clock_timer();

    // clock envelope called on frame counter quarter-frame see clock_quarter_frame() in APU class
    void clock_envelope();

    // clock length counter and sweep called on frame counter half-frame
    void clock_length_and_sweep();

    // get current output sample (0-15)
    uint8_t output() const;

    // reset all state
    void reset();
};

// From https://www.nesdev.org/wiki/APU_Triangle
//  The NES APU triangle channel generates a pseudo-triangle wave. It has no volume control;
//  the waveform is either cycling or suspended. It includes a linear counter,
//  an extra duration timer of higher accuracy than the length counter.
// The triangle channel contains the following:
// timer,
// length counter,
// linear counter,
// linear counter reload flag,
// control flag,
// sequencer.
//
//      Linear Counter   Length Counter
//            |                |
//            v                v
//Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
//
// Unlike the pulse channels, the triangle channel supports frequencies up to the
// maximum frequency the timer will allow, meaning frequencies up to fCPU/32
// (about 55.9 kHz for NTSC) are possible - far above the audible range
// f = fCPU/(32*(tval + 1))
// tval = fCPU/(32*f) - 1

class Triangle {
public:
    LengthCounter length_counter;

    //  set by bit 7 of $4008
    // This bit is also the length counter halt flag
    bool control_flag = false;

    // set by low 6-0 bits of $4008
    // linear counter is reloaded with this value if control_flag is set
    uint8_t counter_reload_value = 0;

    // Timer controls frequency
    // The timer is an 11-bit value (0-2047). Unlike Pulse, Triangle timer ticks at the rate of the CPU clock.
    uint16_t timer_period = 0;        // 11-bit period value from lower 3 bits of $400B (high) + $400A (low)

    // countdown timer that reloads from table and clocks the shift register on 0
    uint16_t timer_counter = 0;

    // The sequencer is clocked by the timer as long as both the linear counter and the length counter are nonzero.
    // The sequencer sends the following looping 32-step sequence of values to the mixer:
    // 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0
    // 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    static const uint8_t SEQUENCER_TABLE[2][16];

    // write to channel registers
    void write_register(uint8_t reg, uint8_t value);

    // clock the timer called every APU cycle = every CPU cycle (Triangle only!)
    void clock_timer();

    // get current output sample (0-15)
    uint8_t output() const;

    // reset all triangle state
    void reset();

};

// From: https://www.nesdev.org/wiki/APU_Noise
// The NES APU noise channel generates pseudo-random 1-bit noise at 16 different frequencies.
//
// The noise channel contains the following: envelope generator, timer, Linear Feedback Shift Register, length counter.
//
//       Timer --> Shift Register   Length Counter
//                       |                |
//                       v                v
//    Envelope -------> Gate ----------> Gate --> (to mixer)
class Noise
{
public:
    Envelope envelope;
    LengthCounter length_counter;

    // length counter halt flag $400C bit 4
    bool length_counter_halt = false;

    // constant volume $400C bit 5
    bool constant_volume_flag = false;

    // bits 3-0 of $400C
    uint8_t volume_envelope_divider_period = 0;

    //  mode is set by bit 7 of $400E
    bool mode_flag = false;

    // bits 3-0 of $400E
    uint8_t timer_period_index = 0;

    // countdown timer that reloads from table and clocks the shift register on 0
    uint16_t timer_counter = 0;

    // The timer period is set to the entry timer_period_index of a lookup table
    // which represents how many APU cycles happen between shift register clocks
    // (half the CPU-cycle values shown on NES Dev Wiki).
    // We only implement NTSC periods and ignore the 16 possible PAL values, so
    // a 1-dimension table suffices. The index corresponds to $400E bits 3–0
    // of timer_period_index. We inline to reduce confusion.
    inline static constexpr uint16_t TIMER_PERIOD_IN_APU_TICKS[16] = {
        2,    // $0:  4  CPU cycles
        4,    // $1:  8
        8,    // $2:  16
       16,    // $3:  32
       32,    // $4:  64
       48,    // $5:  96
       64,    // $6:  128
       80,    // $7:  160
      101,    // $8:  202
      127,    // $9:  254
      190,    // $A:  380
      254,    // $B:  508
      381,    // $C:  762
      508,    // $D:  1016
     1017,    // $E:  2034
     2034     // $F:  4068 (old hardware listed 2046 which makes rumbly sounds sound kinda wrong)
   };

    // The shift register is 15 bits wide, with bits numbered
    // 14 - 13 - 12 - 11 - 10 - 9 - 8 - 7 - 6 - 5 - 4 - 3 - 2 - 1 - 0
    // When the timer clocks the shift register, the following actions occur in order:
    // Fedback is calculated as the exclusive-OR of bit 0 and one other bit:
    // bit 6 if Mode flag is set, otherwise bit 1.
    // The shift register is shifted right by one bit.
    // Bit 14, the leftmost bit, is set to the feedback calculated earlier.
    // This results in a pseudo-random bit sequence, 32767 steps long when Mode flag is clear,
    // and randomly 93 or 31 steps long otherwise (aka long mode and short mode in some descriptions).
    // The particular 31- or 93-step sequence depends on where in the 32767-step sequence
    // the shift register was when Mode flag was set.
    // The mixer receives the current envelope volume except when
    // Bit 0 of the shift register is set, or
    // The length counter is zero
    // On power-up, the shift register is loaded with the value 1.
    // We store teh 15-bit LFSR state in a uint16_t and updated with bit operations.
    uint16_t shift_register_state = 1;

    // clock the timer called every APU cycle = every 2 CPU cycles
    void clock_timer();

    // clock the shift register
    void clock_shift_register();

    // get current output sample (0-15)
    uint8_t output() const;

    // reset all noise state
    void reset();
};

class DeltaModulationChannel
{
public:


    // clock the timer called every APU cycle = every 2 CPU cycles
    void clock_timer();

    // clock the shift register
    void clock_shift_register();

    // get current output sample (0-15)
    uint8_t output() const;

    // reset all noise state
    void reset();
};

// The main APU class contains all audio channels and the frame counter.
// Based on https://www.nesdev.org/wiki/APU#Pulse_($4000–$4007)
//
// From https://www.nesdev.org/wiki/APU_Frame_Counter
// The frame counter generates clocks for the envelope, length counter, and
// sweep units. It can operate in two modes defined by $4017 depending how it is configured. It may optionally issue an IRQ on the last tick of the 4-step sequence.
//    The following diagram illustrates the two modes, selected by bit 7 of $4017:
//    mode 0:    mode 1:       function
//    -  ---  -
//     - - - f    - - - - -    IRQ (if bit 6 is clear)
//     - l - l    - l - - l    Length counter and sweep
//     e e e e    e e e - e    Envelope and linear counter
//
// 4-step mode (mode bit = 0):
//   Step 1: Clock envelope
//   Step 2: Clock envelope + length/sweep
//   Step 3: Clock envelope
//   Step 4: Clock envelope + length/sweep, set IRQ flag
//   (Then loops back to step 1)
//
// 5-step mode (mode bit = 1):
//   Step 1: Clock envelope
//   Step 2: Clock envelope + length/sweep
//   Step 3: Clock envelope
//   Step 4: Clock envelope + length/sweep
//   Step 5: (nothing)
//   (Then loops back to step 1)
//   In this mode, the frame interrupt flag is never set.
//
// Note that the frame counter is not exactly synchronized with the PPU NMI;
// it runs independently at a consistent rate which is approximately 240Hz (NTSC) in 4-step mode.
// Some games (e.g. The Legend of Zelda, Super Mario Bros.) manually synchronize it by
// writing $C0 or $FF to $4017 once per frame.

class APU {
public:

    APU();

    void reset();
    void step();

    PulseChannel pulse1{true};   // true = is pulse 1 (affects sweep)
    PulseChannel pulse2{false};  // false = is pulse 2
    Triangle triangle;
    Noise noise;

    // TODO: Mixer, and DMC

    //  frame counter
    uint16_t frame_counter_cycles = 0;  // Cycle counter for frame sequencer
    uint8_t frame_counter_step = 0;     // Current step (0-3 or 0-4)
    bool frame_counter_mode = false;    // false = 4-step, true = 5-step
    bool frame_irq_inhibit = false;     // Inhibit frame counter IRQ
    bool frame_irq_flag = false;        // Frame IRQ pending

    //  register interface
    // writes to these should go through write_register()
    uint8_t $4000 = 0, $4001 = 0, $4002 = 0, $4003 = 0;  // Pulse 1
    uint8_t $4004 = 0, $4005 = 0, $4006 = 0, $4007 = 0;  // Pulse 2
    uint8_t $4008 = 0, $4009 = 0, $400A = 0, $400B = 0;  // Triangle
    uint8_t $400C = 0, $400D = 0, $400E = 0, $400F = 0;  // Noise
    uint8_t $4010 = 0, $4011 = 0, $4012 = 0, $4013 = 0;  // DMC
    uint8_t $4015 = 0;  // Status
    uint8_t $4017 = 0;  // Frame counter

    // write to APU register
    void write_register(uint16_t address, uint8_t value);

    // read APU status ($4015)
    uint8_t read_status();

    // mixed audio output
    float get_output() const;

private:
    // clock frame counter components
    void clock_frame_counter();
    void clock_quarter_frame();  // envelope
    void clock_half_frame();     // length counter + sweep

    // APU cycle counter for timing frame counterew
    uint64_t cycle_count = 0;
};

} // namespace nes
