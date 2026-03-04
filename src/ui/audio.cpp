#include "ui/audio.hpp"
#include "nes/console.hpp"

namespace ui {

    AudioStream::AudioStream(console& emu)
        : m_emu(emu), m_sampleRate(44100), m_lastSample(0)
    {
        m_buffer.reserve(4096);
    }

    AudioStream::~AudioStream() = default; // default destructor is fine since we don't have any manual resource management

    bool AudioStream::initialize(unsigned int sampleRate) {
        m_sampleRate = sampleRate;
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
        // Use a fixed chunk size.  735 samples = 44100 / 60, which matches
        // exactly one NES frame at 60 fps.  Keeping this small minimises
        // latency; SFML will call us again immediately if needed.
        constexpr size_t CHUNK_SIZE = 735;

        m_buffer.resize(CHUNK_SIZE);

        const size_t received = m_emu.get_apu().drain_samples(m_buffer.data(), CHUNK_SIZE);

        // Pad any shortfall with the last sample (hold), not silence (zero).
        // This avoids a sharp discontinuity when the emulation thread hasn't
        // produced enough samples yet.
        if (received > 0)
            m_lastSample = m_buffer[received - 1];

        for (size_t i = received; i < CHUNK_SIZE; ++i)
            m_buffer[i] = m_lastSample;

        data.samples     = m_buffer.data();
        data.sampleCount = CHUNK_SIZE;
        return true; // always return true — stream runs until stopped
    }

    void AudioStream::onSeek(sf::Time /*timeOffset*/) {
        // Seeking is not supported; the emulator state cannot be rewound.
    }

}
