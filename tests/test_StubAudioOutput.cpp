#include <gtest/gtest.h>

#include "platform/stubs/StubAudioOutput.h"

TEST(StubAudioOutput, OpenSucceeds)
{
    StubAudioOutput audio;
    EXPECT_TRUE(audio.open("test-device", 44100, 2, 16));
    EXPECT_TRUE(audio.isOpen());
}

TEST(StubAudioOutput, WriteReturnsFrameCount)
{
    StubAudioOutput audio;
    audio.open("test", 44100, 2, 16);

    int16_t buffer[256] = {};
    long written = audio.writeFrames(buffer, 128);
    EXPECT_EQ(written, 128);
}

TEST(StubAudioOutput, CloseWorks)
{
    StubAudioOutput audio;
    audio.open("test", 44100, 2, 16);
    EXPECT_TRUE(audio.isOpen());

    audio.close();
    EXPECT_FALSE(audio.isOpen());
}

TEST(StubAudioOutput, DeviceNameIsStub)
{
    StubAudioOutput audio;
    audio.open("test", 44100, 2, 16);
    EXPECT_EQ(audio.deviceName(), "stub:null");
}
