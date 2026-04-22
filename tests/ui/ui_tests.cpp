#include "ui/ui.hpp"
#include "nes/console.hpp"

#include <gtest/gtest.h>

namespace {

TEST(UiInputTest, PressAndReleaseAUpdatesControllerBit) {
    console emu;

    sf::Event press{};
    press.type = sf::Event::KeyPressed;
    press.key.code = sf::Keyboard::Z;
    ui::process_input(press, emu);
    EXPECT_NE(emu.get_mem().get_controller1() & ui::A, 0);

    sf::Event release{};
    release.type = sf::Event::KeyReleased;
    release.key.code = sf::Keyboard::Z;
    ui::process_input(release, emu);
    EXPECT_EQ(emu.get_mem().get_controller1() & ui::A, 0);
}

TEST(UiInputTest, DirectionalAndStartButtonsMapCorrectly) {
    console emu;

    sf::Event up_press{};
    up_press.type = sf::Event::KeyPressed;
    up_press.key.code = sf::Keyboard::Up;
    ui::process_input(up_press, emu);

    sf::Event start_press{};
    start_press.type = sf::Event::KeyPressed;
    start_press.key.code = sf::Keyboard::Enter;
    ui::process_input(start_press, emu);

    const uint8_t state = emu.get_mem().get_controller1();
    EXPECT_NE(state & ui::UP, 0);
    EXPECT_NE(state & ui::START, 0);

    sf::Event up_release{};
    up_release.type = sf::Event::KeyReleased;
    up_release.key.code = sf::Keyboard::Up;
    ui::process_input(up_release, emu);

    EXPECT_EQ(emu.get_mem().get_controller1() & ui::UP, 0);
    EXPECT_NE(emu.get_mem().get_controller1() & ui::START, 0);
}

TEST(UiInputTest, NonGameplayKeysAreIgnored) {
    console emu;
    emu.get_mem().set_controller1(0x5A);

    sf::Event press{};
    press.type = sf::Event::KeyPressed;
    press.key.code = sf::Keyboard::Q;
    ui::process_input(press, emu);

    EXPECT_EQ(emu.get_mem().get_controller1(), 0x5A);
}

} // namespace
