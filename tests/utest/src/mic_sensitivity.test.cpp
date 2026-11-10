#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "src/system/common/elk_decoder.h"
#include "src/system/component/sound.h"
#include "src/system/logic/inputs_bluetooth.h"
#include "src/system/logic/user_command_queue.h"

namespace lampda {

namespace {

/// Restore the default sensitivity, shared by all tests
struct mic_sensitivity : public ::testing::Test
{
  void TearDown() override { component::microphone::set_sensitivity(component::microphone::defaultSensitivity); }
};

common::elk::Package make_sensitivity_package(const uint8_t sensitivity)
{
  const std::array<uint8_t, 9> message = {0x7E, 0x00, 0x06, sensitivity, 0x00, 0x00, 0x00, 0x00, 0xEF};
  common::elk::Package package;
  EXPECT_TRUE(common::elk::decode_ELK_message(message.data(), message.size(), package));
  return package;
}

} // namespace

TEST_F(mic_sensitivity, default_is_unity_gain)
{
  EXPECT_EQ(component::microphone::get_sensitivity(), component::microphone::defaultSensitivity);
  EXPECT_FLOAT_EQ(component::microphone::sensitivity_to_gain(component::microphone::defaultSensitivity), 1.0f);
}

TEST_F(mic_sensitivity, gain_bounds)
{
  EXPECT_FLOAT_EQ(component::microphone::sensitivity_to_gain(0), component::microphone::minSensitivityGain);
  EXPECT_FLOAT_EQ(component::microphone::sensitivity_to_gain(component::microphone::maxSensitivity),
                  component::microphone::maxSensitivityGain);
  // out of range values are constrained
  EXPECT_FLOAT_EQ(component::microphone::sensitivity_to_gain(UINT8_MAX), component::microphone::maxSensitivityGain);
}

TEST_F(mic_sensitivity, gain_is_increasing)
{
  float lastGain = 0.0f;
  for (uint8_t sensitivity = 0; sensitivity <= component::microphone::maxSensitivity; ++sensitivity)
  {
    const float gain = component::microphone::sensitivity_to_gain(sensitivity);
    EXPECT_GT(gain, lastGain) << "sensitivity " << static_cast<int>(sensitivity);
    lastGain = gain;
  }
}

TEST_F(mic_sensitivity, set_is_constrained)
{
  component::microphone::set_sensitivity(20);
  EXPECT_EQ(component::microphone::get_sensitivity(), 20);

  component::microphone::set_sensitivity(component::microphone::maxSensitivity + 1);
  EXPECT_EQ(component::microphone::get_sensitivity(), component::microphone::maxSensitivity);
}

TEST_F(mic_sensitivity, elk_command_sets_sensitivity)
{
  logic::inputs_bluetooth::handle_BLE_ELK_command(make_sensitivity_package(0));
  logic::user_command_queue::loop();
  EXPECT_EQ(component::microphone::get_sensitivity(), 0);

  logic::inputs_bluetooth::handle_BLE_ELK_command(make_sensitivity_package(75));
  logic::user_command_queue::loop();
  EXPECT_EQ(component::microphone::get_sensitivity(), 75);

  logic::inputs_bluetooth::handle_BLE_ELK_command(make_sensitivity_package(100));
  logic::user_command_queue::loop();
  EXPECT_EQ(component::microphone::get_sensitivity(), 100);
}

TEST_F(mic_sensitivity, elk_command_waits_for_the_main_loop)
{
  component::microphone::set_sensitivity(20);

  // the bluetooth task only posts the commands: nothing changes before the main loop runs them
  logic::inputs_bluetooth::handle_BLE_ELK_command(make_sensitivity_package(0));
  logic::inputs_bluetooth::handle_BLE_ELK_command(make_sensitivity_package(75));
  EXPECT_EQ(component::microphone::get_sensitivity(), 20);

  // commands run in order
  logic::user_command_queue::loop();
  EXPECT_EQ(component::microphone::get_sensitivity(), 75);
}

} // namespace lampda
