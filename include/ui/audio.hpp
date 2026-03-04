#pragma once

#include <SFML/Audio.hpp>
#include <vector>
#include <cstdint>

class console;

namespace ui {

class AudioStream : public sf::SoundStream {
    public:
        AudioStream(console& emu);
        ~AudioStream();

        bool initialize(unsigned int sampleRate = 44100);

    private:
        bool onGetData(Chunk& data) override;
        void onSeek(sf::Time timeOffset) override;

        console& m_emu;
        std::vector<int16_t> m_buffer;
        unsigned int m_sampleRate;
        unsigned int m_samplesGenerated;
        
        // Timing: NES runs at ~1.789773 MHz, we need to generate audio samples
        // at a fixed rate (e.g., 44.1 kHz)
        float m_cpuCyclesToSamples; // conversion factor (perhaps this should be calculated once based on sample rate?)
        unsigned long long m_lastCpuCycle;
    };
}
