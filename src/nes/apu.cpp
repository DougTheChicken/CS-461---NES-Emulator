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
        ;
    }

    void APU::clock_half_frame()
    {
        ;
    }

    void APU::clock_quarter_frame()
    {
        ;
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
        ;
    }

    void PulseChannel::clock_length_and_sweep()
    {
        ;
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

    void Noise::reset()
    {
        ;
    }

    void DeltaModulationChannel::reset()
    {
        ;
    }
}

