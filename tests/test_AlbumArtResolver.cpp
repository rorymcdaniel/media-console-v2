#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "orchestration/AlbumArtResolver.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"

class AlbumArtResolverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        int argc = 0;
        if (!QCoreApplication::instance())
        {
            m_app = std::make_unique<QCoreApplication>(argc, nullptr);
        }
        m_playbackState = std::make_unique<PlaybackState>();
        m_receiverState = std::make_unique<ReceiverState>();
    }

    std::unique_ptr<QCoreApplication> m_app;
    std::unique_ptr<PlaybackState> m_playbackState;
    std::unique_ptr<ReceiverState> m_receiverState;
};

// Test 1: Streaming source returns ReceiverState albumArtUrl
TEST_F(AlbumArtResolverTest, StreamingSourceUsesReceiverArt)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_receiverState->setAlbumArtUrl("http://receiver/art.jpg");
    m_playbackState->setActiveSource(MediaSource::Streaming);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), "http://receiver/art.jpg");
}

// Test 2: Bluetooth source returns ReceiverState albumArtUrl
TEST_F(AlbumArtResolverTest, BluetoothSourceUsesReceiverArt)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_receiverState->setAlbumArtUrl("http://receiver/bt-art.jpg");
    m_playbackState->setActiveSource(MediaSource::Bluetooth);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), "http://receiver/bt-art.jpg");
}

// Test 3: CD source returns PlaybackState albumArtUrl
TEST_F(AlbumArtResolverTest, CdSourceUsesPlaybackArt)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setAlbumArtUrl("/cache/cd/art.jpg");
    m_playbackState->setActiveSource(MediaSource::CD);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), "/cache/cd/art.jpg");
}

// Test 4: Library source returns PlaybackState albumArtUrl
TEST_F(AlbumArtResolverTest, LibrarySourceUsesPlaybackArt)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setAlbumArtUrl("/music/album/cover.jpg");
    m_playbackState->setActiveSource(MediaSource::Library);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), "/music/album/cover.jpg");
}

// Test 5: Phono source returns empty string
TEST_F(AlbumArtResolverTest, PhonoSourceReturnsEmpty)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setActiveSource(MediaSource::Phono);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), QString());
}

// Test 6: Computer source returns empty string
TEST_F(AlbumArtResolverTest, ComputerSourceReturnsEmpty)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setActiveSource(MediaSource::Computer);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), QString());
}

// Test 7: None source returns empty string
TEST_F(AlbumArtResolverTest, NoneSourceReturnsEmpty)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    // Both states have art URLs set, but None source should return empty
    m_playbackState->setAlbumArtUrl("/some/art.jpg");
    m_receiverState->setAlbumArtUrl("http://receiver/art.jpg");
    m_playbackState->setActiveSource(MediaSource::None);

    QCoreApplication::processEvents();

    EXPECT_EQ(resolver.albumArtUrl(), QString());
}

// Test 8: Source change triggers resolve and emits albumArtUrlChanged
TEST_F(AlbumArtResolverTest, SourceChangeEmitsSignal)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());
    QSignalSpy spy(&resolver, &AlbumArtResolver::albumArtUrlChanged);

    m_playbackState->setAlbumArtUrl("/cache/cd/art.jpg");
    m_playbackState->setActiveSource(MediaSource::CD);

    QCoreApplication::processEvents();

    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(spy.last().at(0).toString(), "/cache/cd/art.jpg");
}

// Test 9: ReceiverState art change during Streaming triggers re-resolve
TEST_F(AlbumArtResolverTest, ReceiverArtChangeTriggersResolve)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setActiveSource(MediaSource::Streaming);
    m_receiverState->setAlbumArtUrl("http://receiver/art1.jpg");

    QCoreApplication::processEvents();

    QSignalSpy spy(&resolver, &AlbumArtResolver::albumArtUrlChanged);

    m_receiverState->setAlbumArtUrl("http://receiver/art2.jpg");
    QCoreApplication::processEvents();

    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(resolver.albumArtUrl(), "http://receiver/art2.jpg");
}

// Test 10: PlaybackState art change during CD triggers re-resolve
TEST_F(AlbumArtResolverTest, PlaybackArtChangeTriggersResolve)
{
    AlbumArtResolver resolver(m_playbackState.get(), m_receiverState.get());

    m_playbackState->setActiveSource(MediaSource::CD);
    m_playbackState->setAlbumArtUrl("/cache/cd/art1.jpg");

    QCoreApplication::processEvents();

    QSignalSpy spy(&resolver, &AlbumArtResolver::albumArtUrlChanged);

    m_playbackState->setAlbumArtUrl("/cache/cd/art2.jpg");
    QCoreApplication::processEvents();

    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(resolver.albumArtUrl(), "/cache/cd/art2.jpg");
}
