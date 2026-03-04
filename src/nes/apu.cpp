#include "nes/apu.hpp"
#include "nes/mem.hpp"
#include <algorithm>
#include <mutex>

namespace nes {

// ============================================================================
// Envelope
// ============================================================================

void Envelope::clock()
{
    // https://www.nesdev.org/wiki/APU_Envelope
    if (start_flag)
    {
        // Restart: clear flag, reload decay to max, reload divider.
        start_flag  = false;
        decay_level = 15;
        divider     = volume_period;
        return;
    }

    // Normal tick: clock the divider.
    if (divider != 0)
    {
        divider--;
        return;
    }

    // Divider hit 0 — reload it and clock the decay counter.
    divider = volume_period;

    if (decay_level != 0)
        decay_level--;
    else if (loop_flag)
        decay_level = 15;
}

uint8_t Envelope::output() const
{
    return constant_volume_flag ? volume_period : decay_level;
}

void Envelope::reset()
{
    start_flag  = false;
    decay_level = 0;
    divider     = 0;
    // Configuration fields (loop_flag, constant_volume_flag, volume_period)
    // are owned by the parent channel and intentionally not cleared here.
}

// ============================================================================
// LengthCounter
// ============================================================================

const uint8_t LengthCounter::TABLE[32] = {
     10, 254,  20,   2,  40,   4,  80,   6,
    160,   8,  60,  10,  14,  12,  26,  14,
     12,  16,  24,  18,  48,  20,  96,  22,
    192,  24,  72,  26,  16,  28,  32,  30
};

void LengthCounter::load(uint8_t table_index)
{
    counter = TABLE[table_index & 0x1F];
}

void LengthCounter::clock()
{
    if (counter != 0 && !halt)
        counter--;
}

// ============================================================================
// SweepUnit
// ============================================================================

uint16_t SweepUnit::calculate_target_period(uint16_t current_period) const
{
    const uint16_t change = current_period >> shift;

    if (!negate)
        return current_period + change;

    // Pulse 1: ones'-complement negation (subtract change + 1)
    // Pulse 2: twos'-complement negation (subtract change)
    const uint16_t negated = is_channel1 ? (change + 1) : change;
    return (current_period >= negated) ? (current_period - negated) : 0;
}

bool SweepUnit::is_muting(uint16_t target_period)
{
    return target_period > 0x7FF;
}

bool SweepUnit::clock(uint16_t& timer_period)
{
    bool period_updated = false;

    if (divider == 0 || reload_flag)
    {
        // When divider expires with sweep active, update the period.
        if (divider == 0 && enabled && shift != 0)
        {
            const uint16_t target = calculate_target_period(timer_period);
            if (!is_muting(target))
            {
                timer_period   = target;
                period_updated = true;
            }
        }
        divider     = period;
        reload_flag = false;
    }
    else
    {
        divider--;
    }

    return period_updated;
}

void SweepUnit::reset()
{
    enabled     = false;
    period      = 0;
    negate      = false;
    shift       = 0;
    reload_flag = false;
    divider     = 0;
    // is_channel1 is structural identity — never reset.
}

// ============================================================================
// PulseChannel
// ============================================================================

const uint8_t PulseChannel::DUTY_TABLE[4][8] = {
    //  0  1  2  3  4  5  6  7
    {  0, 1, 0, 0, 0, 0, 0, 0 }, // 12.5%
    {  0, 1, 1, 0, 0, 0, 0, 0 }, // 25%
    {  0, 1, 1, 1, 1, 0, 0, 0 }, // 50%
    {  1, 0, 0, 1, 1, 1, 1, 1 }, // 75% (inverted 25%)
};

PulseChannel::PulseChannel(bool is_channel1)
{
    sweep.is_channel1 = is_channel1;
}

void PulseChannel::write_register(uint8_t reg, uint8_t value)
{
    switch (reg & 0x03)
    {
    case 0: // $4000/$4004  DDLC VVVV
    {
        duty_mode                    = (value >> 6) & 0x03;
        const bool halt_or_loop      = (value & 0x20) != 0;
        length_counter.halt          = halt_or_loop;
        envelope.loop_flag           = halt_or_loop;
        envelope.constant_volume_flag = (value & 0x10) != 0;
        envelope.volume_period       = value & 0x0F;
        break;
    }
    case 1: // $4001/$4005  EPPP NSSS
    {
        sweep.enabled     = (value & 0x80) != 0;
        sweep.period      = (value >> 4) & 0x07;
        sweep.negate      = (value & 0x08) != 0;
        sweep.shift       = value & 0x07;
        sweep.reload_flag = true;
        break;
    }
    case 2: // $4002/$4006  TTTT TTTT  (timer low 8 bits)
    {
        timer_period = (timer_period & 0x0700) | value;
        break;
    }
    case 3: // $4003/$4007  LLLL LTTT  (length load + timer high 3 bits)
    {
        timer_period = (timer_period & 0x00FF) | (static_cast<uint16_t>(value & 0x07) << 8);
        length_counter.load((value >> 3) & 0x1F);

        // Side effects: restart sequencer and envelope
        sequencer_position = 0;
        timer_counter      = timer_period;
        envelope.start_flag = true;
        break;
    }
    }
}

void PulseChannel::clock_timer()
{
    if (timer_counter == 0)
    {
        sequencer_position = (sequencer_position + 1) & 0x07;
        timer_counter = timer_period;
    }
    else
    {
        timer_counter--;
    }
}

void PulseChannel::clock_envelope()
{
    envelope.clock();
}

void PulseChannel::clock_length_and_sweep()
{
    length_counter.clock();
    sweep.clock(timer_period);
}

uint8_t PulseChannel::output() const
{
    if (!enabled)                                          return 0;
    if (length_counter.counter == 0)                       return 0;
    if (timer_period < 8)                                  return 0;
    if (!DUTY_TABLE[duty_mode][sequencer_position])        return 0;
    if (SweepUnit::is_muting(sweep.calculate_target_period(timer_period)))
                                                           return 0;
    return envelope.output();
}

void PulseChannel::reset()
{
    enabled            = false;
    timer_period       = 0;
    timer_counter      = 0;
    duty_mode          = 0;
    sequencer_position = 0;

    envelope.reset();
    length_counter.reset();
    sweep.reset();
}

// ============================================================================
// Triangle
// ============================================================================

// BUG FIX: the original table was missing values 0 and 9 in the ascending
// half, producing audible glitches.  Correct 32-step triangle sequence:
const uint8_t Triangle::SEQUENCE[32] = {
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

void Triangle::write_register(uint8_t reg, uint8_t value)
{
    switch (reg & 0x03)
    {
    case 0: // $4008  CRRR RRRR
    {
        control_flag         = (value & 0x80) != 0;
        length_counter.halt  = control_flag;
        counter_reload_value = value & 0x7F;
        break;
    }
    case 1: // $4009 — unused
        break;
    case 2: // $400A  TTTT TTTT  (timer low 8 bits)
    {
        timer_period = (timer_period & 0x0700) | value;
        break;
    }
    case 3: // $400B  LLLL LTTT  (length load + timer high 3 bits)
    {
        timer_period = (timer_period & 0x00FF) | (static_cast<uint16_t>(value & 0x07) << 8);
        length_counter.load((value >> 3) & 0x1F);
        linear_reload_flag = true;
        break;
    }
    }
}

void Triangle::clock_timer()
{
    if (timer_counter != 0)
    {
        timer_counter--;
        return;
    }

    timer_counter = timer_period;

    // Sequencer only advances when both counters are non-zero.
    // Silence is like Pause — unsilencing resumes from the last step.
    if (linear_counter > 0 && length_counter.counter > 0)
        sequencer_step = (sequencer_step + 1) & 0x1F;
}

void Triangle::clock_linear_counter()
{
    // https://www.nesdev.org/wiki/APU_Triangle#Linear_counter
    if (linear_reload_flag)
        linear_counter = counter_reload_value;
    else if (linear_counter != 0)
        linear_counter--;

    // Reload flag is only cleared when control_flag is also clear.
    if (!control_flag)
        linear_reload_flag = false;
}

uint8_t Triangle::output() const
{
    if (!enabled || length_counter.counter == 0 || linear_counter == 0)
        return 0;
    return SEQUENCE[sequencer_step];
}

void Triangle::reset()
{
    control_flag         = false;
    counter_reload_value = 0;
    linear_counter       = 0;
    linear_reload_flag   = false;
    timer_period         = 0;
    timer_counter        = 0;
    sequencer_step       = 0;
    enabled              = false;
    length_counter.reset();
}

// ============================================================================
// Noise
// ============================================================================

void Noise::write_register(uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case 0: // $400C  --LC VVVV
    {
        const bool halt_or_loop          = (value & 0x20) != 0;
        envelope.loop_flag               = halt_or_loop;
        length_counter.halt              = halt_or_loop;
        envelope.constant_volume_flag    = (value & 0x10) != 0;
        envelope.volume_period           = value & 0x0F;
        break;
    }
    // case 1: $400D — unused
    case 2: // $400E  M--- PPPP
    {
        mode_flag          = (value & 0x80) != 0;
        timer_period_index = value & 0x0F;
        break;
    }
    case 3: // $400F  LLLL L---
    {
        length_counter.load(value >> 3);
        envelope.start_flag = true;
        break;
    }
    }
}

void Noise::clock_timer()
{
    if (timer_counter == 0)
    {
        timer_counter = TIMER_PERIOD[timer_period_index];
        clock_shift_register();
    }
    else
    {
        timer_counter--;
    }
}

void Noise::clock_shift_register()
{
    const uint8_t  feedback_bit = mode_flag ? 6 : 1;
    const uint16_t feedback     = (lfsr & 1) ^ ((lfsr >> feedback_bit) & 1);
    lfsr = (lfsr >> 1) | (feedback << 14);
}

void Noise::clock_envelope()
{
    envelope.clock();
}

uint8_t Noise::output() const
{
    if ((lfsr & 1) || length_counter.counter == 0)
        return 0;
    return envelope.output();
}

void Noise::reset()
{
    mode_flag          = false;
    timer_period_index = 0;
    timer_counter      = 0;
    lfsr               = 1;
    enabled            = false;

    envelope.reset();
    length_counter.reset();
}

// ============================================================================
// DeltaModulationChannel
// ============================================================================

void DeltaModulationChannel::attach_memory(Memory* mem)
{
    memory_ = mem;
}

void DeltaModulationChannel::write_register(uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case 0: // $4010  IL-- RRRR
        irq_enabled = (value & 0x80) != 0;
        loop        = (value & 0x40) != 0;
        rate_index  = value & 0x0F;
        if (!irq_enabled)
            irq_pending = false;
        break;

    case 1: // $4011  -DDD DDDD  (direct load)
        output_level = value & 0x7F;
        break;

    case 2: // $4012  AAAA AAAA
        sample_address = value;
        break;

    case 3: // $4013  LLLL LLLL
        sample_length = value;
        break;
    }
}

void DeltaModulationChannel::start_sample()
{
    if (bytes_remaining == 0)
    {
        current_address = decode_address(sample_address);
        bytes_remaining = decode_length(sample_length);
    }
}

bool DeltaModulationChannel::fetch_sample_byte()
{
    if (memory_ == nullptr)
        return false;

    sample_buffer      = memory_->read(current_address);
    sample_buffer_full = true;

    // Advance address with wrap at $FFFF → $8000
    current_address++;
    if (current_address == 0)
        current_address = 0x8000;

    bytes_remaining--;
    if (bytes_remaining == 0)
    {
        if (loop)
        {
            current_address = decode_address(sample_address);
            bytes_remaining = decode_length(sample_length);
        }
        else if (irq_enabled)
        {
            irq_pending = true;
        }
    }

    // DMA fetch stalls the CPU for ~4 cycles due to bus contention.
    pending_stall_cycles = 4;
    return true;
}

void DeltaModulationChannel::clock_memory_reader()
{
    if (!sample_buffer_full && bytes_remaining > 0)
        fetch_sample_byte();
}

void DeltaModulationChannel::clock_output_unit()
{
    // End of output cycle: reload shift register from sample buffer.
    if (bits_remaining == 0)
    {
        bits_remaining = 8;

        if (sample_buffer_full)
        {
            silence            = false;
            shift_register     = sample_buffer;
            sample_buffer_full = false;
        }
        else
        {
            silence = true;
        }
    }

    // Apply the current delta bit to the output level.
    if (!silence)
    {
        if (shift_register & 1)
        {
            if (output_level <= 125) output_level += 2;
        }
        else
        {
            if (output_level >= 2)   output_level -= 2;
        }
        shift_register >>= 1;
    }

    bits_remaining--;
}

void DeltaModulationChannel::clock_timer()
{
    if (timer_counter == 0)
    {
        timer_counter = RATE_TABLE[rate_index];
        clock_output_unit();
    }
    else
    {
        timer_counter--;
    }
}

uint8_t DeltaModulationChannel::output() const
{
    return output_level;
}

void DeltaModulationChannel::reset()
{
    irq_enabled         = false;
    irq_pending         = false;
    enabled             = false;
    loop                = false;
    rate_index          = 0;
    output_level        = 0;
    sample_address      = 0;
    sample_length       = 0;
    sample_buffer       = 0;
    sample_buffer_full  = false;
    current_address     = 0;
    bytes_remaining     = 0;
    timer_counter       = 0;
    shift_register      = 0;
    bits_remaining      = 0;
    silence             = true; // silent until CPU provides a sample
    pending_stall_cycles = 0;
}

// ============================================================================
// APU
// ============================================================================

APU::APU() { reset(); }

void APU::reset()
{
    frame_counter_step   = 0;
    frame_counter_cycles = 0;
    frame_counter_mode   = false;
    frame_irq_inhibit    = false;
    frame_irq_flag       = false;
    status_enable        = 0;
    cpu_cycle            = 0;

    write_status(0x00);

    pulse1.reset();
    pulse2.reset();
    triangle.reset();
    noise.reset();
    dmc.reset();

    // Flush any stale audio samples so the stream starts clean
    // must be done after resetting the channels since they may have pending samples in the queue
    // mutex is needed to avoid race conditions with the audio callback thread that may be draining samples at the same time
    {
        std::lock_guard<std::mutex> lock(m_sample_mutex);
        m_sample_queue.clear();
    }
    m_sample_accumulator = 0.0;
}

// https://www.nesdev.org/wiki/APU_Frame_Counter
//
// 4-step mode (mode = 0):               5-step mode (mode = 1):
//   Step 1: quarter                        Step 1: quarter
//   Step 2: quarter + half                 Step 2: quarter + half
//   Step 3: quarter                        Step 3: quarter
//   Step 4: quarter + half + IRQ           Step 4: (nothing)
//                                          Step 5: quarter + half
void APU::clock_frame_counter()
{
    frame_counter_cycles++;

    if (frame_counter_mode) // 5-step
    {
        switch (frame_counter_cycles)
        {
        case 3728:  clock_quarter_frame(); break;
        case 7456:  clock_quarter_frame(); clock_half_frame(); break;
        case 11185: clock_quarter_frame(); break;
        case 14914: break; // silent step
        case 18640:
            clock_quarter_frame();
            clock_half_frame();
            frame_counter_cycles = 0;
            break;
        }
    }
    else // 4-step
    {
        switch (frame_counter_cycles)
        {
        case 3728:  clock_quarter_frame(); break;
        case 7456:  clock_quarter_frame(); clock_half_frame(); break;
        case 11185: clock_quarter_frame(); break;
        case 14914:
            clock_quarter_frame();
            clock_half_frame();
            if (!frame_irq_inhibit)
                frame_irq_flag = true;
            frame_counter_cycles = 0;
            break;
        }
    }
}

void APU::clock_quarter_frame()
{
    pulse1.clock_envelope();
    pulse2.clock_envelope();
    noise.clock_envelope();
    triangle.clock_linear_counter();
}

void APU::clock_half_frame()
{
    pulse1.clock_length_and_sweep();
    pulse2.clock_length_and_sweep();
    noise.length_counter.clock();
    triangle.length_counter.clock();
}

void APU::step()
{
    cpu_cycle++;
    clock_frame_counter();

    // Triangle ticks every CPU cycle.
    triangle.clock_timer();

    // Pulse, noise, and DMC tick every other CPU cycle (APU rate).
    if ((cpu_cycle & 1) == 0)
    {
        pulse1.clock_timer();
        pulse2.clock_timer();
        noise.clock_timer();
        dmc.clock_timer();
        dmc.clock_memory_reader();
    }

    // Collect a PCM sample when the accumulator crosses the threshold.
    // This runs at ~44100 Hz relative to the CPU clock so that the audio
    // stream always has freshly-generated samples to drain.
    // (likely needs to be tuned if the emulator runs at a different CPU frequency or audio sample rate compared to what I was testing with)
    m_sample_accumulator += 1.0;
    if (m_sample_accumulator >= CYCLES_PER_SAMPLE)
    {
        m_sample_accumulator -= CYCLES_PER_SAMPLE;

        float s = get_output();
        if (s >  1.0f) s =  1.0f;
        if (s < -1.0f) s = -1.0f;
        const int16_t sample = static_cast<int16_t>(s * 32767.0f);

        std::lock_guard<std::mutex> lock(m_sample_mutex);
        if (m_sample_queue.size() < MAX_QUEUED_SAMPLES)
            m_sample_queue.push_back(sample);
    }
}

// ── Register interface ──────────────────────────────────────────────────

void APU::write_register(uint16_t address, uint8_t value)
{
    if (address < 0x4000 || address > 0x4017)
        return;

    switch (address)
    {
    case 0x4015: write_status(value);        return;
    case 0x4017: write_frame_counter(value); return;

    case 0x4000: case 0x4001: case 0x4002: case 0x4003:
        pulse1.write_register(address - 0x4000, value); return;

    case 0x4004: case 0x4005: case 0x4006: case 0x4007:
        pulse2.write_register(address - 0x4004, value); return;

    case 0x4008: case 0x4009: case 0x400A: case 0x400B:
        triangle.write_register(address - 0x4008, value); return;

    case 0x400C: case 0x400D: case 0x400E: case 0x400F:
        noise.write_register(address - 0x400C, value); return;

    case 0x4010: case 0x4011: case 0x4012: case 0x4013:
        dmc.write_register(address - 0x4010, value); return;

    default: return;
    }
}

// $4015 write  ---D NT21
void APU::write_status(uint8_t value)
{
    status_enable = value & 0x1F;

    pulse1.enabled = (status_enable & 0x01) != 0;
    if (!pulse1.enabled) pulse1.length_counter.clear();

    pulse2.enabled = (status_enable & 0x02) != 0;
    if (!pulse2.enabled) pulse2.length_counter.clear();

    triangle.enabled = (status_enable & 0x04) != 0;
    if (!triangle.enabled) triangle.length_counter.clear();

    noise.enabled = (status_enable & 0x08) != 0;
    if (!noise.enabled) noise.length_counter.clear();

    dmc.enabled = (status_enable & 0x10) != 0;

    // "Writing to this register clears the DMC interrupt flag."
    dmc.irq_pending = false;
}

// $4017 write  MI-- ----
void APU::write_frame_counter(uint8_t value)
{
    frame_irq_inhibit  = (value & 0x40) != 0;
    frame_counter_mode = (value & 0x80) != 0;

    if (frame_irq_inhibit)
        frame_irq_flag = false;

    frame_counter_step   = 0;
    frame_counter_cycles = 0;

    // 5-step mode immediately generates quarter + half frame clocks.
    if (frame_counter_mode)
    {
        clock_quarter_frame();
        clock_half_frame();
    }
}

// $4015 read  IF-D NT21
uint8_t APU::read_status()
{
    uint8_t status = 0;

    if (pulse1.length_counter.is_active())   status |= 0x01;
    if (pulse2.length_counter.is_active())   status |= 0x02;
    if (triangle.length_counter.is_active()) status |= 0x04;
    if (noise.length_counter.is_active())    status |= 0x08;
    if (dmc.bytes_remaining > 0)             status |= 0x10;
    if (frame_irq_flag)                      status |= 0x40;
    if (dmc.irq_pending)                     status |= 0x80;

    // Reading $4015 clears the frame interrupt flag.
    frame_irq_flag  = false;
    // Note: NES Dev documents a same-cycle race condition where the flag
    // reads as 1 but isn't cleared; we don't emulate that edge case.

    return status;
}

// ── Mixer (lookup-free approximation) ───────────────────────────────────
// https://www.nesdev.org/wiki/APU_Mixer

float APU::get_output() const
{
    return get_pulse_output() + get_tnd_output();
}

size_t APU::drain_samples(int16_t* out, size_t count)
{
    std::lock_guard<std::mutex> lock(m_sample_mutex);
    const size_t available = std::min(m_sample_queue.size(), count);
    for (size_t i = 0; i < available; ++i)
    {
        out[i] = m_sample_queue.front();
        m_sample_queue.pop_front();
    }
    return available;
}

//                     95.88
// pulse_out = ──────────────────────────
//             (8128 / (p1 + p2)) + 100
float APU::get_pulse_output() const
{
    const float sum = static_cast<float>(pulse1.output() + pulse2.output());
    if (sum == 0.0f) return 0.0f;
    return 95.88f / ((8128.0f / sum) + 100.0f);
}

//                         159.79
// tnd_out = ─────────────────────────────────────────
//           1/(t/8227 + n/12241 + d/22638) + 100
float APU::get_tnd_output() const
{
    const float t = static_cast<float>(triangle.output());
    const float n = static_cast<float>(noise.output());
    const float d = static_cast<float>(dmc.output());

    const float denom = (t / 8227.0f) + (n / 12241.0f) + (d / 22638.0f);
    if (denom == 0.0f) return 0.0f;
    return 159.79f / ((1.0f / denom) + 100.0f);
}

} // namespace nes
