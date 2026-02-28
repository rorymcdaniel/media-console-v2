#include <QCoreApplication>

#include <gtest/gtest.h>

#include "app/AppBuilder.h"
#include "app/AppConfig.h"
#include "app/AppContext.h"

class AppBuilderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName, nullptr };
            new QCoreApplication(argc, argv);
        }
    }
};

TEST_F(AppBuilderTest, BuildReturnsNonNullContext)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    EXPECT_NE(ctx.audioOutput, nullptr);
    EXPECT_NE(ctx.cdDrive, nullptr);
    EXPECT_NE(ctx.gpioMonitor, nullptr);
    EXPECT_NE(ctx.displayControl, nullptr);
}

TEST_F(AppBuilderTest, ContextHasAudioOutput)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.audioOutput, nullptr);
}

TEST_F(AppBuilderTest, ContextHasCdDrive)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.cdDrive, nullptr);
}

TEST_F(AppBuilderTest, ContextHasGpioMonitor)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.gpioMonitor, nullptr);
}

TEST_F(AppBuilderTest, ContextHasDisplayControl)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.displayControl, nullptr);
}

TEST_F(AppBuilderTest, ContextHasReceiverState)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.receiverState, nullptr);
}

TEST_F(AppBuilderTest, ContextHasPlaybackState)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.playbackState, nullptr);
}

TEST_F(AppBuilderTest, ContextHasUIState)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.uiState, nullptr);
}

TEST_F(AppBuilderTest, ContextHasLocalPlaybackController)
{
    AppConfig config;
    AppBuilder builder;
    auto ctx = builder.build(config);

    ASSERT_NE(ctx.localPlaybackController, nullptr);
}
