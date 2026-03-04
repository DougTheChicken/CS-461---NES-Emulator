#include "ui/audio.hpp"
#include "nes/console.hpp"
#include <cmath>

namespace ui {

    AudioStream::AudioStream(console& emu): m_emu(emu), m_sampleRate(44100), m_samplesGenerated(0), m_cpuCyclesToSamples(0.0f), m_lastCpuCycle(0) {
        m_buffer.reserve(4096);
    }

    AudioStream::~AudioStream() {
        // SoundStream will be stopped automatically
        // No additional cleanup needed for now but could be added if we later manage more resources
    }

    bool AudioStream::initialize(unsigned int sampleRate) {
        m_sampleRate = sampleRate;
        
        // NES CPU runs at approximately 1,789,773 Hz
        // We want to generate audio at m_sampleRate Hz
        // samples_per_cpu_cycle = m_sampleRate / 1789773
        // this could probably be found or genrated elsewhere, but we'll calculate it here for now
        constexpr float NES_CPU_FREQUENCY = 1789773.0f;
        m_cpuCyclesToSamples = static_cast<float>(m_sampleRate) / NES_CPU_FREQUENCY;
        
        // Initialize SFML SoundStream with mono audio at our sample rate
        sf::SoundStream::initialize(1, m_sampleRate);
        
        m_lastCpuCycle = 0;
        m_samplesGenerated = 0;
        
        return true;
    }

    bool AudioStream::onGetData(Chunk& data) {
        m_buffer.clear();
        
        // Generate samples—request chunks of audio data
        // We'll generate enough samples to fill a reasonable buffer
        // This likely needs to be tuned but for now I shall leave it at this
        unsigned int samplesToGenerate = 4096;
        
        for (unsigned int i = 0; i < samplesToGenerate; ++i) {
            float sample = m_emu.get_apu().get_output();
            
            // Clamp
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            
            // Convert to int16_t range [-32768, 32767]
            int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
            m_buffer.push_back(intSample);
        }
        
        data.samples = m_buffer.data();
        data.sampleCount = m_buffer.size();
        
        return m_buffer.size() > 0; // Return true if we have data
    }

    void AudioStream::onSeek(sf::Time timeOffset) {
        // This would require seeking the emulator state, which we do not have implemented. For now, we'll just ignore seeks.
        // here is a commented out example of how we might calculate the CPU cycle to seek to based on the time offset:

        // unsigned long long targetCpuCycle = static_cast<unsigned long long>(timeOffset.asSeconds() * NES_CPU_FREQUENCY);
        // m_lastCpuCycle = targetCpuCycle;
    }

}
