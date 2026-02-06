#include "nes/apu.hpp"

namespace nes
{
    APU::APU() { reset(); }

    // safe for start and reset
    void APU::reset()
    {
        frame_counter_step = 0;
        frame_counter_cycles = 0;
        status_enable = 0;
        frame_counter_mode = false;
        frame_irq_flag = false;
        cpu_cycle = 0;

        // clear status
        write_register($4015, 0x00);

        pulse1.reset();
        pulse2.reset();
        triangle.reset();
        noise.reset();
        dmc.reset();
    };

    // https://www.nesdev.org/wiki/APU_Frame_Counter
    // We only support NTSC timings
    // From https://www.nesdev.org/wiki/APU_Frame_Counter
    // The frame counter generates clocks for the envelope, length counter, and
    // sweep units. It can operate in two modes defined by $4017 depending how it is configured.
    // It may optionally issue an IRQ on the last tick of the 4-step sequence.
    // The following diagram illustrates the two modes, selected by bit 7 of $4017:
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
    void APU::clock_frame_counter()
    {
        frame_counter_cycles++;

        if (frame_counter_mode) // 5-step mode
        {
            switch (frame_counter_cycles)
            {
                case 3728: clock_quarter_frame(); break;
                case 7456: clock_quarter_frame(); clock_half_frame(); break;
                case 11185: clock_quarter_frame(); break;
                case 14914: break; // silent step. ssssshhh...
                case 18640: clock_quarter_frame(); clock_half_frame(); frame_counter_cycles = 0; break;
                // In this mode, the frame interrupt flag is never set.
            }
        }
        else // 4-step  mode
        {
            switch (frame_counter_cycles)
            {
                case 3728: clock_quarter_frame(); break;
                case 7456: clock_quarter_frame(); clock_half_frame(); break;
                case 11185: clock_quarter_frame(); break;
                case 14914:
                    {
                        clock_quarter_frame();
                        clock_half_frame();

                        // frame interrupt flag gets set if interrupt inhibit is clear
                        if (!frame_irq_inhibit) frame_irq_flag = true;

                        frame_counter_cycles = 0;
                        break;
                    }
            }
        }
    }

    // length counters and sweep units only on half frame
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    void APU::clock_half_frame()
    {
        pulse1.clock_length_and_sweep();
        pulse2.clock_length_and_sweep();
    }

    // envelopes and linear counters only on quarter frame
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    void APU::clock_quarter_frame()
    {
        pulse1.clock_envelope();
        pulse2.clock_envelope();
        noise.clock_envelope();
        triangle.clock_linear_counter();
    }

    // see https://www.nesdev.org/wiki/APU_Mixer
    // output = pulse_out + tnd_out
    // this is NOT efficient, but it is highly accurate and should be fast
    // on modern hardware, as required by NES-EMU specification
    // TODO: optionally consider implementing high- and low-pass filters
    float APU::get_output() const
    {
        return get_pulse_output() + get_tnd_output();
    }

    // pulse audio output
    //                          95.88
    // pulse_out = ------------------------------------
    //              (8128 / (pulse1 + pulse2)) + 100
    float APU::get_pulse_output() const
    {
        const float pulse_sum = pulse2.output() + pulse1.output(); // NOLINT(*-narrowing-conversions)
        if (pulse_sum > 0.0f)
            return 95.88f / ((8128.0f / pulse_sum) + 100.0f);
        return 0.0f;
    }

    // triangle-noise-dmc output formula from https://www.nesdev.org/wiki/APU_Mixer
    //
    //                              159.79
    // tnd_out = -------------------------------------------------------------
    //                                  1
    //           ----------------------------------------------------- + 100
    //           (triangle / 8227) + (noise / 12241) + (dmc / 22638)
    float APU:: get_tnd_output() const
    {
        const float t = triangle.output();
        const float n = noise.output();
        const float d = dmc.output();
        const float tnd_sum = (t / 8227.0f) + (n / 12241.0f) + (d /22638.0f);
        if (tnd_sum > 0.0f)
            return 159.79f / ((1.0f / tnd_sum) + 100.0f);
        return 0.0f;
    }

    // From https://www.nesdev.org/wiki/APU#Status_($4015)
    // write APU status ($4015). writing zero silences NT21; dmc finishes sample before silence
    // 4-bits: ---D NT21 	Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1)
    void APU::write_status(uint8_t value)
    {
        status_enable = value & 0x1F; // mask to keep lower 4 bits

        // clear length counter to silence NT21
        if ((status_enable & 0x01) == 0) pulse1.length_counter.clear();
        if ((status_enable & 0x02) == 0) pulse2.length_counter.clear();
        if ((status_enable & 0x04) == 0) triangle.length_counter.clear();
        if ((status_enable & 0x08) == 0) noise.length_counter.clear();

        // dmc silence uses a flag to enable/disable
        if ((status_enable & 0x10) == 0) dmc.enabled = false;

        // "Writing to this register clears the DMC interrupt flag."
        dmc.irq_pending = false;
    }

    // From https://www.nesdev.org/wiki/APU_Frame_Counter
    // $4017 	MI-- ---- 	Mode (M, 0 = 4-step, 1 = 5-step), IRQ inhibit flag (I)
    // write APU frame counter ($4017)
    void APU::write_frame_counter(uint8_t value)
    {
        frame_irq_inhibit = (value & 0x40) != 0;

        // Sequencer mode: 0 selects 4-step sequence, 1 selects 5-step sequence
        frame_counter_mode = (value & 0x80) != 0;

        // "If interrupt inhibit flag set, the frame interrupt flag is cleared,
        // otherwise it is unaffected."
        if (frame_irq_inhibit) frame_irq_flag = false;

        // reset sequencer after clearing flag
        frame_counter_step = 0;
        frame_counter_cycles = 0;

        // side effect: if mode is set then both quarter frame and half frame generated
        if (frame_counter_mode)
        {
            clock_quarter_frame();
            clock_half_frame();
        }
    }

    // From https://www.nesdev.org/wiki/APU#Status_($4015)
    // $4015 read IF-D NT21
    uint8_t APU::read_status()
    {
        uint8_t status = 0;

        // bits 0-3: channels are considered enabled if its length counter is non-zero
        // "length counter > 0 (N/T/2/1)"
        if (pulse1.length_counter.counter > 0) status |= 0x01;
        if (pulse2.length_counter.counter > 0) status |= 0x02;
        if (triangle.length_counter.counter > 0) status |= 0x04;
        if (noise.length_counter.counter > 0) status |= 0x08;

        // bit 4: D will read as 1 if the DMC bytes remaining is more than 0.
        if (dmc.bytes_remaining > 0) status |= 0x10;

        // bit 5 is open bus so never |= 0x20.
        // Because the external bus is disconnected when reading $4015,
        // the open bus value comes from the last cycle that did not read $4015,

        // bit 6 frame interrupt (F)
        if (frame_irq_flag) status |= 0x40;

        // bit 7: the tricky bit-- DMC interrupt (I) is irq_pending (not irq_enable)
        if (dmc.irq_pending) status |= 0x80;

        // reading $4015 clear frame interrupt and the DMC interrupt
        frame_irq_flag = false;
        dmc.irq_pending = false;

        // But NES Dev says: If an interrupt flag was
        // set at the same moment of the read, it will read back as 1
        // but it will not be cleared. We do not emulate this behavior
        // which nesttest ROM says is fine.
        // TODO: determine if this level of APU/CPU sync is necessary

        return status;
    }

    // Invariant: always call write_register() before step() in CPU cycle, and only once per cycle for each
    void APU::step()
    {
        // TODO: verify order of operations herein
        cpu_cycle++;
        clock_frame_counter();

        // clock all the channel timers.
        // triangle ticks on every CPU cycle
        triangle.clock_timer();

        // and others tick on every other CPU cycle
        if (!(cpu_cycle & 1)) // if not odd
        {
            pulse1.clock_timer();
            pulse2.clock_timer();
            noise.clock_timer();
            dmc.clock_timer();
        }
    }

    // from https://www.nesdev.org/wiki/APU_registers
    void APU::write_register(uint16_t address, uint8_t value)
    {
        // don't trust inputs in exposed interfaces
        if (address < 0x4000 || address > 0x4017)
            return;

        switch (address)
        {
        case 0x4015: write_status(value); return;
        case 0x4017: write_frame_counter(value); return;

        // 0x4000 - 0x4003 pulse 1
        case 0x4000:
        case 0x4001:
        case 0x4002:
        case 0x4003: pulse1.write_register(address - 0x4000, value); return;

        // 0x4004 - 0x4007 pulse 2
        case 0x4004:
        case 0x4005:
        case 0x4006:
        case 0x4007: pulse2.write_register(address - 0x4004, value); return;

        // 0x4008 - 0x400B triangle
        case 0x4008:
        case 0x4009:    // not implemented in triangle
        case 0x400A:
        case 0x400B: triangle.write_register(address - 0x4008, value); return;

        // 0x400C - 0x400F noise
        case 0x400C:
        case 0x400D:    // not implemented in noise
        case 0x400E:
        case 0x400F: noise.write_register(address - 0x400C, value); return;

        // 0x4010 - 0x4013 dmc
        case 0x4010:
        case 0x4011:
        case 0x4012:
        case 0x4013: dmc.write_register(address - 0x4010, value); return;
        default: return;
        }
    }

    // see https://www.nesdev.org/wiki/APU_Pulse
    PulseChannel::PulseChannel(bool is_pulse1) { sweep.is_pulse1 = is_pulse1; }

    void PulseChannel::clock_envelope() { envelope.clock(); }

    void PulseChannel::clock_length_and_sweep()
    {
        length_counter.clock();
        sweep.clock(timer_period);
    }

    void PulseChannel::clock_timer()
    {
        // decrementing until we're done means playing the current note for the timer_period
        // then advancing to the next step in the waveform sequence and playing that note
        // for the timer period
        if (timer_counter == 0)
        {
            // sequencer wraps 0..7
            sequencer_position = (sequencer_position + 1) & 0x07;
            timer_counter = timer_period;
        }
        else
        {
            timer_counter--;
        }
    }

    // structured for our incrementing sequencer
    const uint8_t PulseChannel::DUTY_TABLE[4][8] =
    {
        // 0  1  2  3  4  5  6  7   <- sequencer_position
        { 0, 1, 0, 0, 0, 0, 0, 0 }, // 12.5%
        { 0, 1, 1, 0, 0, 0, 0, 0 }, // 25%
        { 0, 1, 1, 1, 1, 0, 0, 0 }, // 50%
        { 1, 0, 0, 1, 1, 1, 1, 1 }  // 75%
    };

    // See https://www.nesdev.org/wiki/APU_Pulse#Pulse_channel_output_to_mixer
    // relies on envelope output unless silenced for one of several reasons
    uint8_t PulseChannel::output() const
    {
        // there are several ways to mute
        if (!enabled
            || length_counter.counter == 0
            || timer_period < 8
            || !DUTY_TABLE[duty_mode][sequencer_position]
            || sweep.is_muting(sweep.calculate_target_period(timer_period))) // TODO: expensive? optimize?
            return 0;
        return envelope.output();
    }

    // safe for start and reset
    void PulseChannel::reset()
    {
        enabled = false;
        timer_counter = 0;
        timer_period = 0;
        duty_mode = 0;
        sequencer_position = 0;

        envelope.reset();
        length_counter.reset();
        sweep.reset();
    }


    // From https://www.nesdev.org/wiki/APU_Pulse#Registers
    void PulseChannel::write_register(uint8_t reg, uint8_t value)
    {
        // we always pass "address - $4000" or "address - $4003" or similar
        // so normalizes reg to 0..3
        switch (reg & 0x03)
        {
        // $4000/$4004
        case 0:
            {
                // bits 3-0 envelope period
                envelope.volume_period = value & 0x0F;

                // bit 4 constant volume (1) and envelope decay (0)
                envelope.constant_volume_flag = (value & 0x10) != 0;

                // see https://www.nesdev.org/wiki/APU_Envelope
                // bit 5 --L- ---- 	APU Length Counter halt flag/envelope loop flag
                bool halt_or_loop = (value & 0x20) != 0;
                length_counter.halt = halt_or_loop;
                envelope.loop_flag = halt_or_loop;

                // bits 7-6 duty_mode 0..3
                // shift and isolate the 2 bits into duty_mode 0..3 values
                duty_mode = (value >> 6) & 0x03;

                // side effects duty cycle is changed but sequencer position unchanged
                break;
            }

        // $4001/$4005
        case 1:
            {
                // see https://www.nesdev.org/wiki/APU_Sweep
                // EPPP.NSSS
                // bit 7 	E--- ---- 	Enabled flag
                sweep.enabled = (value & 0x80) != 0;

                // bits 6-4 	-PPP ---- 	The divider's period is P + 1 half-frames
                sweep.period  = (value >> 4) & 0x07;

                // bit 3 	---- N--- 	Negate flag
                // 0: add to period, sweeping toward lower frequencies
                // 1: subtract from period, sweeping toward higher frequencies
                sweep.negate  = (value & 0x08) != 0;

                // bits 2-0 	---- -SSS 	Shift count (number of bits).
                // If SSS is 0, then behaves like E=0.
                sweep.shift   = value & 0x07;

                // Side effects 	Sets the reload flag
                sweep.reload_flag = true;

                break;
            }

        // $4002/$4006
        case 2:
            {
                // timer low 8 bits
                // clear
                timer_period &= 0x0700;

                // set
                timer_period |= value;

                break;
            }
        // $4003/$4007
        case 3:
            {
                // bits 2-0 timer high 3 bits
                // clear high bits
                timer_period &= 0x00FF;

                // set high 3 bits
                timer_period |= (value & 0x07) << 8;

                // bits 7-3 length index 0..31
                // see https://www.nesdev.org/wiki/APU_Length_Counter
                // If the enabled flag is set, the length counter is loaded with entry L of the length table
                // LLLL L--- >> 3 -> ---L LLLL &  0x1F -> 0..31 so
                if (enabled)
                    length_counter.load((value >> 3) & 0x1F);

                // side effects - sequencer and envelope restarted, phase is reset
                envelope.start_flag = true;
                sequencer_position = 0;
                timer_counter = timer_period; // TODO: test thoroughly

                break;
            }
        }
    }

    void Envelope::clock()
    {
        ;
    }

    uint8_t Envelope::output() const
    {
        return 1;
    }

    void Envelope::reset()
    {

    }

    void LengthCounter::clock()
    {
        ;
    }

    void LengthCounter::load(uint8_t index)
    {
        ;
    }

    void LengthCounter::reset()
    {
        ;
    }

    // see https://www.nesdev.org/wiki/APU_Sweep
    bool SweepUnit::clock(uint16_t& timer_period)
    {
        if (divider != 0)
        {
            divider--;
        }
        else
        {
            // https://www.nesdev.org/wiki/APU_Sweep#Updating_the_period
            // reload the divider's period as normal
            divider = period;

            if (enabled == true && shift != 0)
            {
                uint16_t target_period = calculate_target_period(timer_period);

                // case 1.1. pulse's period is set to the target period
                if (!is_muting(target_period))
                {
                    timer_period = target_period;
                    return true;
                }

            }
        }
        return false; // case 1.2. is already fully addressed so the period was not changed
    }

    // from https://www.nesdev.org/wiki/APU_Sweep#Calculating_the_target_period
    // The sweep unit continuously calculates each pulse channel's target period
    uint16_t SweepUnit::calculate_target_period(uint16_t current_period) const
    {
        // A barrel shifter shifts the pulse channel's 11-bit raw timer period right by the shift count,
        // producing the change amount.
        // If the negate flag is true, the change amount is made negative, so pitch sounds higher
        if (negate) {
            // one's-complement quirk
            if (is_pulse1) return current_period - (current_period >> shift) - 1;
            // pulse 2 is merely the difference
            else return current_period - (current_period >> shift);
        }
        // negate is false so change amount get added -- period gets longer, so pitch sounds lower
        else return current_period + (current_period >> shift);
    }

    // from https://www.nesdev.org/wiki/APU_Sweep#Muting
    // Two conditions cause the sweep unit to mute the channel until the condition ends:
    // If the current period is less than 8, the sweep unit mutes the channel. (We check this in PulseChannel.output()).
    // If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
    bool SweepUnit::is_muting(uint16_t current_period) const
    {
        return current_period > 0x7FF;
    }

    void SweepUnit::reset()
    {
        enabled = false;
        period = 0;
        negate = false;
        shift = 0;
        reload_flag = false;
        divider = 0;
        // don't reset is_pulse1
    }

    void Triangle::reset()
    {
        ;
    }

    void Triangle::clock_linear_counter()
    {
        // TODO: fix naive implementation stub
        linear_counter--;
    }

    void Triangle::clock_timer()
    {
        ;
    }

    uint8_t Triangle::output() const
    {
        return 1;
    }

    void Triangle::write_register(uint8_t reg, uint8_t value)
    {
        ;
    }

    void Noise::clock_envelope()
    {
        envelope.clock();
    }

    void Noise::clock_timer()
    {
        ;
    }

    uint8_t Noise::output() const
    {
        return 1;
    }

    void Noise::write_register(uint8_t reg, uint8_t value)
    {
        ;
    }

    void Noise::clock_shift_register()
    {
        ;
    }

    void Noise::reset()
    {
        ;
    }

    uint8_t DeltaModulationChannel::output() const
    {
        return 1;
    }

    void DeltaModulationChannel::clock_timer()
    {
        ;
    }

    void DeltaModulationChannel::write_register(uint8_t reg, uint8_t value)
    {
        ;
    }

    void DeltaModulationChannel::attach_memory(Memory* m)
    {
        ;
    }

    void DeltaModulationChannel::clock_memory_unit()
    {
        ;
    }

    void DeltaModulationChannel::clock_output_unit()
    {
        ;
    }

    bool DeltaModulationChannel::fetch_sample_byte()
    {
        // TODO: fix naive implementation
        return true;
    }

    void DeltaModulationChannel::play_sample()
    {
        ;
    }

    void DeltaModulationChannel::reset()
    {
        ;
    }
}

