#pragma once
#include <cstdint>

namespace nes {

// APU needs memory access
class Memory;

// From https://www.nesdev.org/wiki/APU_Envelope
//
// In a synthesizer, an envelope controls how a sound's volume changes over
// time.  The NES APU envelope generator either produces a decreasing saw
// (with optional looping) or holds a constant volume for use by a software
// envelope.  Volume values are practically linear (see APU Mixer).
//
// Each envelope unit contains: start flag, divider, and decay level counter.
//
// Clocking (quarter-frame):
//   - If start flag is clear, the divider is clocked.
//   - Otherwise the start flag is cleared, decay reloads to 15, and the
//     divider period is immediately reloaded.
//   - When the divider reaches 0 it reloads from volume_period and clocks
//     the decay counter: decrement if non-zero, else reload to 15 if looping.
//
// Output: constant_volume_flag ? volume_period : decay_level
//
//                                    Loop flag
//                                         |
//                        Start flag  +.   |   Constant volume
//                            |        |   |        flag
//                            v        v   v          |
// Quarter frame clock --> Divider --> Decay --> |    |
//                            ^        level     |    v
//                            |                  | Select --> Envelope output
//                            |                  |
//                         Envelope parameter +> |
class Envelope {
public:
    void clock();
    uint8_t output() const;
    void reset();

    // Configuration — set by CPU writes to $4000/$4004/$400C
    bool loop_flag             = false; // also serves as length counter halt
    bool constant_volume_flag  = false; // true = use volume_period directly
    uint8_t volume_period      = 0;    // 4-bit: constant volume OR envelope period

    // Internal state
    bool    start_flag  = false; // triggers envelope restart on next clock
    uint8_t decay_level = 0;    // current volume (counts 15→0)
    uint8_t divider     = 0;    // countdown; reloads from volume_period
};


// From https://www.nesdev.org/wiki/APU_Sweep
//
// The sweep unit adjusts the pulse channel's period over time, creating
// pitch bends.  It continuously computes a target period from the current
// period, a shift count, and a negate flag.  Pulse 1 uses ones'-complement
// negation (−c − 1); Pulse 2 uses twos'-complement (−c).
//
// Muting: the channel is silenced when period < 8 **or** the computed
// target period exceeds $7FF.
class SweepUnit {
public:
    uint16_t calculate_target_period(uint16_t current_period) const;
    bool clock(uint16_t& timer_period);
    void reset();

    // Returns true when the target period would silence the channel.
    static bool is_muting(uint16_t target_period);

    // Configuration — set by CPU writes to $4001/$4005
    bool    enabled  = false;
    uint8_t period   = 0;    // divider period (3 bits, 0-7)
    bool    negate   = false; // true = subtract (pitch up)
    uint8_t shift    = 0;    // 3-bit shift count; 0 behaves like enabled=false

    // Internal state
    bool    reload_flag = false;
    uint8_t divider     = 0;

    // Channel identity — affects negation; set once at construction, never reset.
    bool is_channel1 = true;
};


// From https://www.nesdev.org/wiki/APU_Length_Counter
//
// The length counter provides automatic note duration.  On load it receives
// a value from a hardware lookup table indexed by a 5-bit field.  It counts
// down once per half-frame; when it reaches 0 the channel is silenced.
// Setting the halt flag pauses counting (note sustains indefinitely).
class LengthCounter {
public:
    void load(uint8_t table_index);
    void clock();

    bool is_active() const { return counter > 0; }
    void clear()           { counter = 0; }
    void reset()           { counter = 0; }

    bool    halt    = false;
    uint8_t counter = 0;

    static const uint8_t TABLE[32];
};


// From https://www.nesdev.org/wiki/APU_Pulse
//
//                  Sweep -----> Timer
//                    |            |            clocks
//                    |            v
//                    |        Sequencer   Length Counter
//                    |            |             |
//                    v            v             v
// Envelope -------> Gate -----> Gate -------> Gate ---> (to mixer)
//
// Registers ($4000-$4003 for Pulse 1, $4004-$4007 for Pulse 2):
//   $4000/$4004: DDLC VVVV  Duty, Length halt, Constant vol, Volume/Period
//   $4001/$4005: EPPP NSSS  Sweep Enable, Period, Negate, Shift
//   $4002/$4006: TTTT TTTT  Timer low 8 bits
//   $4003/$4007: LLLL LTTT  Length counter load, Timer high 3 bits
class PulseChannel {
public:
    explicit PulseChannel(bool is_channel1 = true);

    void write_register(uint8_t reg, uint8_t value);
    void clock_timer();
    void clock_envelope();
    void clock_length_and_sweep();
    uint8_t output() const;
    void reset();

    // Sub-units
    Envelope      envelope;
    SweepUnit     sweep;
    LengthCounter length_counter;

    // Timer (11-bit period + countdown)
    uint16_t timer_period   = 0;
    uint16_t timer_counter  = 0;

    // Duty cycle sequencer (8-step, 4 patterns)
    uint8_t duty_mode           = 0; // 0-3
    uint8_t sequencer_position  = 0; // 0-7

    bool enabled = false; // controlled via $4015

    static const uint8_t DUTY_TABLE[4][8];
};


// From https://www.nesdev.org/wiki/APU_Triangle
//
//      Linear Counter   Length Counter
//            |                |
//            v                v
// Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
//
// The triangle channel has no volume control; the waveform is either
// cycling or suspended.  It includes a linear counter for finer-grained
// duration control than the length counter alone.
// Timer ticks at CPU clock rate (not halved like Pulse/Noise).
class Triangle {
public:
    void write_register(uint8_t reg, uint8_t value);
    void clock_timer();
    void clock_linear_counter();
    uint8_t output() const;
    void reset();

    LengthCounter length_counter;

    bool    control_flag         = false; // bit 7 of $4008; also length halt
    uint8_t counter_reload_value = 0;    // bits 6-0 of $4008
    uint8_t linear_counter       = 0;
    bool    linear_reload_flag   = false;

    uint16_t timer_period   = 0;
    uint16_t timer_counter  = 0;
    uint8_t  sequencer_step = 0;

    bool enabled = false; // $4015 bit 2

    static const uint8_t SEQUENCE[32];
};


// From https://www.nesdev.org/wiki/APU_Noise
//
//       Timer --> Shift Register   Length Counter
//                       |                |
//                       v                v
//    Envelope -------> Gate ----------> Gate --> (to mixer)
//
// The noise channel generates pseudo-random 1-bit noise at 16 frequencies
// via a 15-bit LFSR.  Mode flag selects between a 32767-step sequence
// (long / bit 1) and a 93-or-31-step sequence (short / bit 6).
class Noise {
public:
    void write_register(uint8_t reg, uint8_t value);
    void clock_timer();
    void clock_envelope();
    void clock_shift_register();
    uint8_t output() const;
    void reset();

    Envelope      envelope;
    LengthCounter length_counter;

    bool    mode_flag          = false; // bit 7 of $400E
    uint8_t timer_period_index = 0;    // bits 3-0 of $400E
    uint16_t timer_counter     = 0;
    uint16_t lfsr              = 1;    // 15-bit shift register; power-on = 1

    bool enabled = false; // controlled via $4015

    // NTSC timer period lookup (values are in APU ticks = CPU cycles / 2).
    static constexpr uint16_t TIMER_PERIOD[16] = {
          2,   4,   8,  16,  32,  48,  64,  80,
        101, 127, 190, 254, 381, 508, 1017, 2034
    };
};


// From https://www.nesdev.org/wiki/APU_DMC
//
// The delta modulation channel outputs 7-bit PCM driven by a stream of
// 1-bit deltas.  Each bit adds or subtracts 2 from the output level,
// clamped to 0-127.  Samples reside at $C000-$FFFF and are fetched by
// a memory reader that stalls the CPU for ~4 cycles per byte.
class DeltaModulationChannel {
public:
    void write_register(uint8_t reg, uint8_t value);
    void clock_timer();
    void clock_memory_reader();
    uint8_t output() const;
    void reset();

    void attach_memory(Memory* mem);

    // Starts (or restarts) sample playback from the configured address/length.
    void start_sample();

    // Configuration — set by CPU writes to $4010-$4013
    bool    irq_enabled     = false;
    bool    loop            = false;
    uint8_t rate_index      = 0;
    uint8_t output_level    = 0;    // 7-bit DAC value ($4011)
    uint8_t sample_address  = 0;    // raw $4012 value; actual addr = $C000 + A*64
    uint8_t sample_length   = 0;    // raw $4013 value; actual len  = L*16 + 1

    // Status
    bool    irq_pending     = false;
    bool    enabled         = false; // $4015 bit 4

    // Memory reader state
    uint8_t  sample_buffer       = 0;
    bool     sample_buffer_full  = false;
    uint16_t current_address     = 0;
    uint16_t bytes_remaining     = 0;

    // Output unit state
    uint16_t timer_counter  = 0;
    uint8_t  shift_register = 0;
    uint8_t  bits_remaining = 0;
    bool     silence        = false;

    // CPU stall cycles from DMA sample fetches (typically 1-4 cycles)
    uint8_t  pending_stall_cycles = 0;

    // NTSC rate table (in APU ticks; already halved from NES Dev Wiki values).
    static constexpr uint16_t RATE_TABLE[16] = {
        214, 190, 170, 160, 143, 127, 113, 107,
         95,  80,  71,  64,  53,  42,  36,  27
    };

private:
    Memory* memory_ = nullptr;

    bool fetch_sample_byte();
    void clock_output_unit();


    // Compute the CPU address from the raw $4012 value.
    static uint16_t decode_address(uint8_t raw) {
        return 0xC000 | (static_cast<uint16_t>(raw) << 6);
    }

    // Compute the byte count from the raw $4013 value.
    static uint16_t decode_length(uint8_t raw) {
        return (static_cast<uint16_t>(raw) << 4) + 1;
    }
};


// The main APU class: all audio channels plus the frame counter.
// Based on https://www.nesdev.org/wiki/APU
class APU {
public:
    APU();

    void reset();

    // Advance the APU by one CPU cycle.
    void step();

    // Register interface (address range $4000-$4017)
    void    write_register(uint16_t address, uint8_t value);
    uint8_t read_status();   // $4015 read

    // Mixed audio output (0.0 – ~1.0)
    float get_output() const;

    // Channels (public for test / debug access)
    PulseChannel          pulse1{true};
    PulseChannel          pulse2{false};
    Triangle              triangle;
    Noise                 noise;
    DeltaModulationChannel dmc;

    // Frame counter state
    uint16_t frame_counter_cycles = 0;
    uint8_t  frame_counter_step   = 0;
    bool     frame_counter_mode   = false; // false = 4-step, true = 5-step
    bool     frame_irq_inhibit    = false;
    bool     frame_irq_flag       = false;

private:
    void write_status(uint8_t value);
    void write_frame_counter(uint8_t value);

    void clock_frame_counter();
    void clock_quarter_frame(); // envelopes + triangle linear counter
    void clock_half_frame();    // length counters + sweep

    float get_pulse_output() const;
    float get_tnd_output() const;

    uint64_t cpu_cycle     = 0;
    uint8_t  status_enable = 0;
};

} // namespace nes
