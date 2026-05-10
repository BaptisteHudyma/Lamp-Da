#include <cstdint>
#include <gtest/gtest.h>

#include "src/system/utils/elk_decoder.h"

#include <array>

namespace lampda::utils::ELK {

// Test some invalid message lenght
TEST(test_elf_decoder, invalid_message_lenght)
{
  Package package;

  std::array<uint8_t, 0> message0;
  EXPECT_FALSE(decode_ELK_message(message0.data(), message0.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 1> message1;
  EXPECT_FALSE(decode_ELK_message(message1.data(), message1.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 2> message2;
  EXPECT_FALSE(decode_ELK_message(message2.data(), message2.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 3> message3;
  EXPECT_FALSE(decode_ELK_message(message3.data(), message3.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 4> message4;
  EXPECT_FALSE(decode_ELK_message(message4.data(), message4.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 5> message5;
  EXPECT_FALSE(decode_ELK_message(message5.data(), message5.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 6> message6;
  EXPECT_FALSE(decode_ELK_message(message6.data(), message6.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 7> message7;
  EXPECT_FALSE(decode_ELK_message(message7.data(), message7.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  std::array<uint8_t, 8> message8;
  EXPECT_FALSE(decode_ELK_message(message8.data(), message8.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);

  // And now too big

  std::array<uint8_t, 10> message10;
  EXPECT_FALSE(decode_ELK_message(message10.data(), message10.size(), package));
  EXPECT_EQ(package.type, Type::INVALID);
  EXPECT_EQ(package.dataSize, 0);
}

// test invalid message header
TEST(test_elf_decoder, invalid_message_HEADER)
{
  std::array<uint8_t, 9> messageWrong = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  for (uint8_t i = 0; i < UINT8_MAX; i++)
  {
    if (i != 0x7E)
    {
      Package package;
      messageWrong[0] = i;
      EXPECT_FALSE(decode_ELK_message(messageWrong.data(), messageWrong.size(), package));
      EXPECT_EQ(package.type, Type::INVALID);
      EXPECT_EQ(package.dataSize, 0);
    }
  }
}

// test invalid message end flag
TEST(test_elf_decoder, invalid_message_END)
{
  std::array<uint8_t, 9> messageWrong = {0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  for (uint8_t i = 0; i < UINT8_MAX; i++)
  {
    if (i != 0xEF)
    {
      Package package;
      messageWrong[8] = i;
      EXPECT_FALSE(decode_ELK_message(messageWrong.data(), messageWrong.size(), package));
      EXPECT_EQ(package.type, Type::INVALID);
      EXPECT_EQ(package.dataSize, 0);
    }
  }
}

// Test all valid brightness commands
TEST(test_elf_decoder, brightness_messages_valid)
{
  for (uint8_t brigthness = 0; brigthness < 100; brigthness++)
  {
    std::array<uint8_t, 9> messageBrightness = {0x7E, 0x00, 0x01, 0, 0x00, 0x00, 0x00, 0x00, 0xEF};
    messageBrightness[3] = brigthness;

    Package package;
    EXPECT_TRUE(decode_ELK_message(messageBrightness.data(), messageBrightness.size(), package));
    EXPECT_EQ(package.type, Type::BRIGHTNESS);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], brigthness);
  }
}

TEST(test_elf_decoder, brightness_messages_invalid)
{
  // invalid values
  for (uint8_t brigthness = 101; brigthness < UINT8_MAX; brigthness++)
  {
    std::array<uint8_t, 9> messageBrightness = {0x7E, 0x00, 0x01, 0, 0x00, 0x00, 0x00, 0x00, 0xEF};
    messageBrightness[3] = brigthness;

    Package package;
    EXPECT_FALSE(decode_ELK_message(messageBrightness.data(), messageBrightness.size(), package));
    EXPECT_EQ(package.type, Type::INVALID);
    EXPECT_EQ(package.dataSize, 0);
  }

  // ignored other values
  for (uint8_t header = 0; header < 255; header++)
  {
    std::array<uint8_t, 9> messageBrightness = {0x7E, header, 0x01, 0, header, header, header, header, 0xEF};

    Package package;
    EXPECT_TRUE(decode_ELK_message(messageBrightness.data(), messageBrightness.size(), package));
    EXPECT_EQ(package.type, Type::BRIGHTNESS);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], 0);
  }
}

// Test all valid on off commands
TEST(test_elf_decoder, onoff_messages_valid)
{
  std::array<uint8_t, 9> message = {0x7E, 0x00, 0x04, 0, 0x00, 0x00, 0x00, 0x00, 0xEF};
  message[3] = 0;

  Package package;
  EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
  EXPECT_EQ(package.type, Type::ONOFF);
  EXPECT_EQ(package.dataSize, 1);
  EXPECT_EQ(package.data[0], 0);

  message[3] = 1;
  EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
  EXPECT_EQ(package.type, Type::ONOFF);
  EXPECT_EQ(package.dataSize, 1);
  EXPECT_EQ(package.data[0], 1);
}

TEST(test_elf_decoder, onoff_messages_invalid)
{
  // invalid values
  for (uint8_t onoff = 2; onoff < UINT8_MAX; onoff++)
  {
    std::array<uint8_t, 9> message = {0x7E, 0x00, 0x04, 0, 0x00, 0x00, 0x00, 0x00, 0xEF};
    message[3] = onoff;

    Package package;
    EXPECT_FALSE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::INVALID);
    EXPECT_EQ(package.dataSize, 0);
  }

  // ignore other values
  for (uint8_t header = 0; header < 255; header++)
  {
    std::array<uint8_t, 9> message = {0x7E, header, 0x04, 0, header, header, header, header, 0xEF};

    Package package;
    EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::ONOFF);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], 0);
  }
}

// test all color chanels
TEST(test_elf_decoder, color_messages_valid)
{
  for (uint8_t colorRed = 0; colorRed < UINT8_MAX; colorRed++)
  {
    std::array<uint8_t, 9> messageColor = {0x7E, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0xEF};
    messageColor[4] = colorRed;

    Package package;
    EXPECT_TRUE(decode_ELK_message(messageColor.data(), messageColor.size(), package));
    EXPECT_EQ(package.type, Type::COLOR_SELECT);
    EXPECT_EQ(package.dataSize, 3);
    EXPECT_EQ(package.data[0], colorRed);
    EXPECT_EQ(package.data[1], 0);
    EXPECT_EQ(package.data[2], 0);
  }

  for (uint8_t colorGreen = 0; colorGreen < UINT8_MAX; colorGreen++)
  {
    std::array<uint8_t, 9> messageColor = {0x7E, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0xEF};
    messageColor[5] = colorGreen;

    Package package;
    EXPECT_TRUE(decode_ELK_message(messageColor.data(), messageColor.size(), package));
    EXPECT_EQ(package.type, Type::COLOR_SELECT);
    EXPECT_EQ(package.dataSize, 3);
    EXPECT_EQ(package.data[0], 0);
    EXPECT_EQ(package.data[1], colorGreen);
    EXPECT_EQ(package.data[2], 0);
  }

  for (uint8_t colorBlue = 0; colorBlue < UINT8_MAX; colorBlue++)
  {
    std::array<uint8_t, 9> messageColor = {0x7E, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0xEF};
    messageColor[5] = colorBlue;

    Package package;
    EXPECT_TRUE(decode_ELK_message(messageColor.data(), messageColor.size(), package));
    EXPECT_EQ(package.type, Type::COLOR_SELECT);
    EXPECT_EQ(package.dataSize, 3);
    EXPECT_EQ(package.data[0], 0);
    EXPECT_EQ(package.data[1], colorBlue);
    EXPECT_EQ(package.data[2], 0);
  }
}

// Test all valid pattern commands
TEST(test_elf_decoder, pattern_messages_valid)
{
  for (uint8_t pattern = 0; pattern <= 28; pattern++)
  {
    std::array<uint8_t, 9> message = {0x7E, 0x00, 0x03, pattern, 0x03, 0x00, 0x00, 0x00, 0xEF};
    message[3] = pattern;

    Package package;
    EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::PATTERN_SELECT);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], pattern);
  }

  for (uint8_t pattern = 128; pattern <= 128 + 28; pattern++)
  {
    std::array<uint8_t, 9> message = {0x7E, 0x00, 0x03, pattern, 0x03, 0x00, 0x00, 0x00, 0xEF};
    message[3] = pattern;

    Package package;
    EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::PATTERN_SELECT);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], pattern - 128);
  }
}

TEST(test_elf_decoder, pattern_messages_invalid)
{
  // invalid values
  for (uint8_t pattern = 29; pattern < 128; pattern++)
  {
    std::array<uint8_t, 9> message = {0x7E, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0xEF};
    message[3] = pattern;

    Package package;
    EXPECT_FALSE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::INVALID);
    EXPECT_EQ(package.dataSize, 0);
  }
  for (uint8_t pattern = 128 + 29; pattern < UINT8_MAX; pattern++)
  {
    std::array<uint8_t, 9> message = {0x7E, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0xEF};
    message[3] = pattern;

    Package package;
    EXPECT_FALSE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::INVALID);
    EXPECT_EQ(package.dataSize, 0);
  }

  // ignore other values
  for (uint8_t pattern = 0; pattern < UINT8_MAX; pattern++)
  {
    std::array<uint8_t, 9> message = {0x7E, pattern, 0x03, 0x00, 0x03, pattern, pattern, pattern, 0xEF};

    Package package;
    EXPECT_TRUE(decode_ELK_message(message.data(), message.size(), package));
    EXPECT_EQ(package.type, Type::PATTERN_SELECT);
    EXPECT_EQ(package.dataSize, 1);
    EXPECT_EQ(package.data[0], 0);
  }
}

} // namespace lampda::utils::ELK
