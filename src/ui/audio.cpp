#include "ui/audio.hpp"
#include "nes/console.hpp"
#include <cmath>
#include <algorithm>

namespace ui {

    AudioStream::AudioStream(console& emu)
        : m_emu(emu), m_sampleRate(44100), m_lastSample(0), m_chunkSamples(0)
    {
        // Reserve a modest default to avoid reallocations before initialize()
        m_chunkBuffer.reserve(4096);
    }

    AudioStream::~AudioStream() = default; // default destructor is fine since we don't have any manual resource management

    bool AudioStream::initialize(unsigned int sampleRate) {
        m_sampleRate = sampleRate;

        // Calculate chunk size based on PPU timing so we emit an integral number of samples per emulated frame.
        // This reduces systematic drift when the NES timing isn't exactly 60.0 Hz.
        const double frame_rate = PPU_HZ / static_cast<double>(PPU_CYCLES_PER_FRAME);
        const size_t CHUNK_SIZE = static_cast<size_t>(std::lround(static_cast<double>(m_sampleRate) / frame_rate));

        m_chunkSamples = CHUNK_SIZE; // number of samples (mono)
        m_chunkBuffer.resize(m_chunkSamples);

        // Mono audio at the requested sample rate.
        sf::SoundStream::initialize(1, m_sampleRate);
        return true;
    }

    // Called by the SFML audio thread each time it needs more PCM data.
    // We drain whatever samples the APU has accumulated since the last call.
    // If the queue runs dry (emulation is slower than real-time, or the
    // buffer size is large relative to one emulation frame), we hold the
    // last known sample to avoid a harsh click/pop.
    bool AudioStream::onGetData(Chunk& data) {
        if (m_chunkSamples == 0) {
            // Not initialized; fall back to a safe default.
            m_chunkSamples = 2048;
            m_chunkBuffer.resize(m_chunkSamples);
        }

        const size_t received = m_emu.get_apu().drain_samples(m_chunkBuffer.data(), m_chunkSamples);

        // Pad any shortfall with the last sample (hold), not silence (zero).
        // This avoids a sharp discontinuity when the emulation thread hasn't
        // produced enough samples yet.
        if (received > 0)
            m_lastSample = m_chunkBuffer[received - 1];

        if (received < m_chunkSamples) {
            std::fill(m_chunkBuffer.begin() + received, m_chunkBuffer.end(), m_lastSample);
        }

        data.samples     = m_chunkBuffer.data();
        data.sampleCount = static_cast<unsigned int>(m_chunkBuffer.size());
        return true; // always return true cuz we want stream to run until stopped later
    }

    void AudioStream::onSeek(sf::Time /*timeOffset*/) {
        // Seeking is not supported; the emulator state cannot be rewound.
    }

}
