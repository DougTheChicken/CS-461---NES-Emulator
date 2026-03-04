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

        // When the APU queue runs dry we repeat the last sample rather than
        // emitting a click.  This avoids a harsh pop on underrun.
        int16_t m_lastSample = 0;
    };
}
