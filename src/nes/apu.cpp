#include "nes/apu.hpp"

namespace nes
{
    APU::APU() { reset(); }
    void APU::reset()
    {
        frame_counter_step = 0;
        frame_counter_step = 0;
        frame_counter_mode = false;
        frame_irq_flag = false;
        cycle_count = 0;
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

    float APU::get_output() const
    {
        return 1.1;
    }

    uint8_t APU::read_status()
    {
        return 1;
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

