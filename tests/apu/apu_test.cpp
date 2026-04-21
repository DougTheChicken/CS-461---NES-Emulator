#include <gtest/gtest.h>
#include "nes/apu.hpp"
#include "nes/mem.hpp"
#include "nes/ppu.hpp"
#include <cmath>

// Test Envelope functionality
TEST(APUTest, EnvelopeBasics) {
    nes::Envelope env;
    
    // Test constant volume mode
    env.constant_volume_flag = true;
    env.volume_period = 12;
    EXPECT_EQ(env.output(), 12);
    
    // Test decay mode
    env.constant_volume_flag = false;
    env.decay_level = 8;
    EXPECT_EQ(env.output(), 8);
}

TEST(APUTest, EnvelopeClock) {
    nes::Envelope env;
    env.constant_volume_flag = false;
    env.loop_flag = false;
    env.volume_period = 3;
    env.decay_level = 15;
    env.divider = 3;
    env.start_flag = false;
    
    // Clock the envelope and check decay behavior
    env.clock();
    EXPECT_LE(env.decay_level, 15); // Should decay or stay at current level
}

TEST(APUTest, EnvelopeStartFlag) {
    nes::Envelope env;
    env.constant_volume_flag = false;
    env.start_flag = true;
    env.volume_period = 5;
    
    // When start flag is set, decay should reload to 15
    env.clock();
    EXPECT_EQ(env.decay_level, 15);
    EXPECT_FALSE(env.start_flag); // Start flag should clear
}

// Test Length Counter
TEST(APUTest, LengthCounterLoad) {
    nes::LengthCounter lc;
    
    // Test loading various indices
    lc.load(0);
    EXPECT_EQ(lc.counter, 10);
    
    lc.load(1);
    EXPECT_EQ(lc.counter, 254);
    
    lc.load(31);
    EXPECT_EQ(lc.counter, 30);
}

TEST(APUTest, LengthCounterClock) {
    nes::LengthCounter lc;
    lc.counter = 5;
    lc.halt = false;
    
    lc.clock();
    EXPECT_EQ(lc.counter, 4);
    
    // Test halt flag prevents counting
    lc.halt = true;
    lc.clock();
    EXPECT_EQ(lc.counter, 4); // Should not decrement
    
    // Test counter doesn't go below zero
    lc.halt = false;
    lc.counter = 1;
    lc.clock();
    EXPECT_EQ(lc.counter, 0);
    lc.clock();
    EXPECT_EQ(lc.counter, 0); // Should stay at 0
}

// Test Sweep Unit
TEST(APUTest, SweepUnitTargetPeriod) {
    nes::SweepUnit sweep;
    sweep.negate = false;
    sweep.shift = 1;
    sweep.is_channel1 = true;
    
    uint16_t current = 64;
    uint16_t target = sweep.calculate_target_period(current);
    
    // Non-negated: target = current + (current >> shift)
    EXPECT_EQ(target, 64 + 32);
}

TEST(APUTest, SweepUnitMuting) {
    // is_muting checks if target period > $7FF
    // (Note: period < 8 is checked separately in PulseChannel::output)
    EXPECT_FALSE(nes::SweepUnit::is_muting(7));
    EXPECT_FALSE(nes::SweepUnit::is_muting(8));
    
    // Period > $7FF should mute
    EXPECT_TRUE(nes::SweepUnit::is_muting(0x800));
    EXPECT_FALSE(nes::SweepUnit::is_muting(0x7FF));
}

// Test Pulse Channel
TEST(APUTest, PulseChannelRegisterWrite) {
    nes::PulseChannel pulse(true);
    
    // Write to register 0: DDLC VVVV
    pulse.write_register(0, 0b10110101);
    
    EXPECT_EQ(pulse.duty_mode, 2);              // DD = 10
    EXPECT_TRUE(pulse.envelope.loop_flag);       // L = 1
    EXPECT_TRUE(pulse.envelope.constant_volume_flag); // C = 1
    EXPECT_EQ(pulse.envelope.volume_period, 5); // VVVV = 0101
}

TEST(APUTest, PulseChannelOutput) {
    nes::PulseChannel pulse(true);
    
    // Setup pulse with constant volume
    pulse.enabled = true;
    pulse.envelope.constant_volume_flag = true;
    pulse.envelope.volume_period = 8;
    pulse.length_counter.counter = 10;
    pulse.timer_period = 100;
    
    uint8_t output = pulse.output();
    // Output should be either 0 or the volume (8), depending on duty cycle
    EXPECT_TRUE(output == 0 || output == 8);
}

// Test Triangle Channel
TEST(APUTest, TriangleSequence) {
    nes::Triangle tri;
    
    // Triangle wave should have values from 0 to 15
    for (int i = 0; i < 32; ++i) {
        EXPECT_GE(nes::Triangle::SEQUENCE[i], 0);
        EXPECT_LE(nes::Triangle::SEQUENCE[i], 15);
    }
    
    // First value should be 15 (peak)
    EXPECT_EQ(nes::Triangle::SEQUENCE[0], 15);
    
    // Middle value (index 16) should be 0 (trough)
    EXPECT_EQ(nes::Triangle::SEQUENCE[16], 0);
}

TEST(APUTest, TriangleOutput) {
    nes::Triangle tri;
    tri.enabled = true;
    tri.length_counter.counter = 10;
    tri.linear_counter = 5;
    
    uint8_t output = tri.output();
    // Output should be 0-15 from the sequence
    EXPECT_GE(output, 0);
    EXPECT_LE(output, 15);
}

// Test Noise Channel
TEST(APUTest, NoiseTimerPeriods) {
    // Ensure noise timer periods are properly defined
    EXPECT_EQ(nes::Noise::TIMER_PERIOD[0], 2);
    EXPECT_EQ(nes::Noise::TIMER_PERIOD[15], 2034);
}

TEST(APUTest, NoiseOutput) {
    nes::Noise noise;
    noise.enabled = true;
    noise.length_counter.counter = 10;
    noise.envelope.constant_volume_flag = true;
    noise.envelope.volume_period = 6;
    noise.lfsr = 1;
    
    uint8_t output = noise.output();
    // Output should be 0 or the volume
    EXPECT_TRUE(output == 0 || output == 6);
}

// Test APU Integration
TEST(APUTest, APUInitialization) {
    nes::APU apu;
    
    // After construction, APU should be in a valid state
    EXPECT_FALSE(apu.pulse1.enabled);
    EXPECT_FALSE(apu.pulse2.enabled);
    EXPECT_FALSE(apu.triangle.enabled);
    EXPECT_FALSE(apu.noise.enabled);
}

TEST(APUTest, APUReset) {
    nes::APU apu;
    
    // Enable channels and set some state
    apu.write_register(0x4015, 0x0F); // Enable all channels
    apu.pulse1.enabled = true;
    apu.pulse2.enabled = true;
    
    // Reset should clear everything
    apu.reset();
    
    EXPECT_FALSE(apu.pulse1.enabled);
    EXPECT_FALSE(apu.pulse2.enabled);
    EXPECT_FALSE(apu.triangle.enabled);
    EXPECT_FALSE(apu.noise.enabled);
}

TEST(APUTest, APUStep) {
    nes::APU apu;
    
    // APU step should not crash
    for (int i = 0; i < 100; ++i) {
        apu.step();
    }
}

TEST(APUTest, APUOutputRange) {
    nes::APU apu;
    
    // Enable pulse channel with constant volume
    apu.write_register(0x4000, 0b00110111); // Pulse 1: constant volume 7
    apu.write_register(0x4003, 0b00001000); // Load length counter
    apu.write_register(0x4015, 0x01); // Enable pulse 1
    
    // Step APU and check output
    for (int i = 0; i < 1000; ++i) {
        apu.step();
        float output = apu.get_output();
        
        // Output should be finite and in a reasonable range
        EXPECT_TRUE(std::isfinite(output));
        EXPECT_GE(output, -1.0f);
        EXPECT_LE(output, 1.0f);
    }
}

TEST(APUTest, APUMultiChannelMixing) {
    nes::APU apu;
    
    // Enable multiple channels
    apu.write_register(0x4000, 0b00110101); // Pulse 1
    apu.write_register(0x4003, 0b00001000);
    apu.write_register(0x4004, 0b00110101); // Pulse 2
    apu.write_register(0x4007, 0b00001000);
    apu.write_register(0x4015, 0x03); // Enable pulse 1 & 2
    
    // Step and check mixed output
    for (int i = 0; i < 100; ++i) {
        apu.step();
    }
    
    float output = apu.get_output();
    EXPECT_TRUE(std::isfinite(output));
}

TEST(APUTest, APUStatusRegisterRead) {
    nes::PPU ppu;
    nes::APU apu;
    nes::Memory mem(ppu, apu);
    
    // Enable pulse 1 and set length counter
    mem.write(0x4015, 0x01); // Enable pulse 1
    mem.write(0x4003, 0b10000000); // Load length counter with high value
    
    // Read status
    uint8_t status = mem.read(0x4015);
    
    // Bit 0 should be set once pulse 1 has a loaded length counter.
    EXPECT_NE(status & 0x01, 0);
}

TEST(APUTest, APUFrameCounter) {
    nes::APU apu;
    
    // Write to frame counter register
    apu.write_register(0x4017, 0x00); // 4-step mode
    EXPECT_FALSE(apu.frame_counter_mode);
    
    apu.write_register(0x4017, 0x80); // 5-step mode
    EXPECT_TRUE(apu.frame_counter_mode);
}

// Test Audio Stream Integration (basic)
TEST(AudioTest, APUGeneratesValidSamples) {
    nes::APU apu;
    
    // Configure a simple tone on pulse 1
    // DDLC VVVV: Duty 50% (10), Length halt (1), Constant volume (1), Volume 15
    apu.write_register(0x4000, 0b10111111); // Duty 50%, length halt, constant volume 15
    apu.write_register(0x4001, 0b00001000); // Sweep: disabled (bit 7=0), shift=0
    apu.write_register(0x4002, 0x00);       // Period low = 0 (very short period for fast cycling)
    apu.write_register(0x4003, 0b11110001); // Period high = 1, length (max length)
    apu.write_register(0x4015, 0x01);       // Enable pulse 1
    
    // Generate samples - need to step enough for sequencer to cycle
    std::vector<float> samples;
    for (int i = 0; i < 10000; ++i) {
        apu.step();
        samples.push_back(apu.get_output());
    }
    
    // Check at least some samples are non-zero
    bool has_nonzero = false;
    for (float s : samples) {
        if (s != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero) << "APU should generate non-zero samples";
    
    // All samples should be in valid range
    for (float s : samples) {
        EXPECT_GE(s, -1.0f) << "Sample below valid range";
        EXPECT_LE(s, 1.0f) << "Sample above valid range";
        EXPECT_TRUE(std::isfinite(s)) << "Sample is not finite";
    }
}

TEST(AudioTest, APUSilenceWhenDisabled) {
    nes::APU apu;
    
    // Don't enable any channels
    apu.write_register(0x4015, 0x00);
    
    // Step and check output is near zero
    for (int i = 0; i < 100; ++i) {
        apu.step();
    }
    
    float output = apu.get_output();
    EXPECT_NEAR(output, 0.0f, 0.001f);
}

TEST(APUTest, PulseHighRegisterWriteDoesNotReloadTimerCounter) {
    nes::PulseChannel pulse(true);

    pulse.timer_period = 0x0123;
    pulse.timer_counter = 5;
    pulse.length_counter.counter = 0;
    pulse.envelope.start_flag = false;
    pulse.sequencer_position = 4;

    pulse.write_register(3, 0b00001001); // length index + high timer bits = 001

    EXPECT_EQ(pulse.timer_period, 0x0123); // low byte 0x23, high bits 0x1
    EXPECT_EQ(pulse.timer_counter, 5);     // should NOT be reloaded
    EXPECT_EQ(pulse.sequencer_position, 0);
    EXPECT_TRUE(pulse.envelope.start_flag);
    EXPECT_GT(pulse.length_counter.counter, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
