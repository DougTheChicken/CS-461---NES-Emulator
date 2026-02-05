#include "nes/apu.hpp"

namespace nes
{
    APU::APU() { reset(); }

    void APU::reset()
    {
        frame_counter_step = 0;
        frame_counter_cycles = 0;
        frame_counter_mode = false;
        frame_irq_flag = false;
        cycle_count = 0;

        // clear status
        write_register($4015, 0x00);

        pulse1.reset();
        pulse2.reset();
        triangle.reset();
        noise.reset();
        dmc.reset();
    };

    void APU::clock_frame_counter()
    {
        frame_counter_cycles++;

        // https://www.nesdev.org/wiki/APU_Frame_Counter
        // We only support NTSC timings
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

    void APU::step()
    {
        ;
    }

    void APU::write_register(uint16_t address, uint8_t value)
    {
        ;
    }

    PulseChannel::PulseChannel(bool is_pulse1)
    {
        ;
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

    void PulseChannel::clock_timer()
    {
        ;
    }

    uint8_t PulseChannel::output() const
    {
        return 1;
    }

    void PulseChannel::reset()
    {
        ;
    }

    void PulseChannel::write_register(uint8_t reg, uint8_t value)
    {
        ;
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

    bool SweepUnit::clock(uint16_t& timer_period)
    {
        // TODO: fix naive implementation
        return true;
    }

    uint16_t SweepUnit::calculate_target_period(uint16_t current_period) const
    {
        return 1;
    }

    bool SweepUnit::is_muting(uint16_t current_period) const
    {
        // TODO: fix naive implementation
        return true;
    }

    void SweepUnit::reset()
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

