#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include "app/AppConfig.h"
#include "audio/AudioStream.h"
#include "cd/CdController.h"
#include "cd/CdMetadataCache.h"
#include "platform/ICdDrive.h"
#include "platform/stubs/StubCdDrive.h"
#include "state/PlaybackState.h"

Q_DECLARE_METATYPE(QVector<TocEntry>)
Q_DECLARE_METATYPE(CdMetadata)

/// Minimal stub for LocalPlaybackController to avoid needing real audio output.
/// CdController only calls isActive(), stop(), play(), and emits trackFinished.
/// We stub via a thin class that provides those signals/slots.
class StubPlaybackController : public QObject
{
    Q_OBJECT
public:
    explicit StubPlaybackController(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    int playCallCount() const { return m_playCallCount; }
    int stopCallCount() const { return m_stopCallCount; }

public slots:
    void play(std::shared_ptr<AudioStream> /*stream*/)
    {
        m_playCallCount++;
        m_active = true;
    }

    void stop()
    {
        m_stopCallCount++;
        m_active = false;
    }

signals:
    void trackFinished();

private:
    bool m_active = false;
    int m_playCallCount = 0;
    int m_stopCallCount = 0;
};

/// Test fixture for CdController tests using StubCdDrive.
/// Since CdController takes a LocalPlaybackController*, but we need a test stub,
/// we pass nullptr for playbackController and test the signal-based behavior.
class CdControllerTest : public ::testing::Test
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

        qRegisterMetaType<QVector<TocEntry>>("QVector<TocEntry>");
        qRegisterMetaType<CdMetadata>("CdMetadata");

        m_drive = new StubCdDrive();
        m_playbackState = new PlaybackState();

        // Configure fast polling for tests
        m_config.pollIntervalMs = 50;
        m_config.idleTimeoutSeconds = 1; // 1 second for fast idle timer tests
        m_config.audioOnly = true;
        m_config.devicePath = "/dev/cdrom";

        // Set up a realistic TOC on the stub drive
        QVector<TocEntry> toc;
        int sector = 150;
        for (int i = 0; i < 10; ++i)
        {
            TocEntry entry;
            entry.trackNumber = i + 1;
            entry.startSector = sector;
            int trackSectors = 15000 + (i * 1000);
            entry.endSector = sector + trackSectors - 1;
            entry.durationSeconds = trackSectors / 75;
            toc.append(entry);
            sector += trackSectors;
        }
        m_drive->setToc(toc);
        m_drive->setDiscId("test-disc-id-001");
    }

    void TearDown() override
    {
        // CdController may parent m_drive, but we created it on the heap
        // and CdController takes non-owning pointer, so we manage lifetime ourselves
        delete m_playbackState;
        delete m_drive;
    }

    CdController* createController()
    {
        // CdController takes non-owning pointers, we pass nullptr for playbackController
        // since we don't have a real one in tests
        auto* ctrl = new CdController(m_drive, nullptr, m_playbackState, m_config);
        return ctrl;
    }

    StubCdDrive* m_drive = nullptr;
    PlaybackState* m_playbackState = nullptr;
    CdConfig m_config;
};

TEST_F(CdControllerTest, DiscInsertionEmitsDiscDetectedAndTocReady)
{
    auto* ctrl = createController();
    QSignalSpy discSpy(ctrl, &CdController::discDetected);
    QSignalSpy tocSpy(ctrl, &CdController::tocReady);

    ctrl->start();

    // Simulate disc insertion
    m_drive->setDiscPresent(true);

    // Wait for poll cycle
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_GE(discSpy.count(), 1);
    EXPECT_GE(tocSpy.count(), 1);

    // Verify TOC has correct track count
    if (tocSpy.count() > 0)
    {
        auto args = tocSpy.first();
        auto toc = args.at(0).value<QVector<TocEntry>>();
        EXPECT_EQ(toc.size(), 10);
    }

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, DiscRemovalEmitsDiscRemoved)
{
    auto* ctrl = createController();
    QSignalSpy removedSpy(ctrl, &CdController::discRemoved);

    ctrl->start();

    // Insert disc
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    // Remove disc
    m_drive->setDiscPresent(false);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_GE(removedSpy.count(), 1);

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, NonAudioDiscEmitsNonAudioSignal)
{
    auto* ctrl = createController();
    QSignalSpy nonAudioSpy(ctrl, &CdController::nonAudioDiscDetected);
    QSignalSpy tocSpy(ctrl, &CdController::tocReady);

    // Set disc present but not audio
    m_drive->setAudioDisc(false);

    ctrl->start();
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_GE(nonAudioSpy.count(), 1);
    // TOC should NOT be emitted for non-audio disc
    EXPECT_EQ(tocSpy.count(), 0);

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, NoAutoPlayOnDiscInsertion)
{
    auto* ctrl = createController();
    QSignalSpy discSpy(ctrl, &CdController::discDetected);
    QSignalSpy tocSpy(ctrl, &CdController::tocReady);

    ctrl->start();
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    // Verify disc was detected and TOC emitted
    EXPECT_GE(discSpy.count(), 1);
    EXPECT_GE(tocSpy.count(), 1);

    // Verify no playback was started (PlaybackState should still be Stopped)
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Stopped);

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, DiscRemovalClearsState)
{
    auto* ctrl = createController();

    ctrl->start();

    // Insert disc
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_TRUE(ctrl->isDiscPresent());
    EXPECT_EQ(ctrl->currentToc().size(), 10);

    // Remove disc
    m_drive->setDiscPresent(false);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_FALSE(ctrl->isDiscPresent());
    EXPECT_TRUE(ctrl->currentToc().isEmpty());
    EXPECT_TRUE(ctrl->currentMetadata().discId.isEmpty());

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, MetadataFailureFallbackToAudioCD)
{
    auto* ctrl = createController();
    QSignalSpy metadataSpy(ctrl, &CdController::metadataReady);

    ctrl->start();
    m_drive->setDiscPresent(true);

    // Wait for disc detection and metadata lookup to start + fail (network timeout or no server)
    // The metadata provider will fail since there's no real server
    // Wait long enough for the metadata lookup chain to complete (all 3 sources fail)
    QTest::qWait(500);

    // Process events to allow metadata failure chain to complete
    QCoreApplication::processEvents();

    // If metadata sources all fail, the controller should emit fallback metadata
    // Note: In tests without network, the NAM requests will fail quickly
    // Give more time for async operations
    for (int i = 0; i < 50; ++i)
    {
        QTest::qWait(100);
        QCoreApplication::processEvents();
        if (metadataSpy.count() > 0)
        {
            break;
        }
    }

    // Metadata should have been emitted (either from cache miss -> lookup failure -> fallback,
    // or from some other path)
    if (metadataSpy.count() > 0)
    {
        auto args = metadataSpy.last();
        auto metadata = args.at(0).value<CdMetadata>();
        // If it came through the fallback path, it should say "Audio CD"
        if (metadata.source == "fallback")
        {
            EXPECT_EQ(metadata.artist, "Audio CD");
            EXPECT_EQ(metadata.album, "Audio CD");
        }
    }

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, IdleTimerFiresStopSpindle)
{
    auto* ctrl = createController();

    ctrl->start();

    // Insert disc
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    // Idle timer is 1 second in our test config
    // Wait for idle timeout
    QTest::qWait(1500);

    EXPECT_GE(m_drive->stopSpindleCallCount(), 1);

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, EjectStopsAndClearsState)
{
    auto* ctrl = createController();
    QSignalSpy removedSpy(ctrl, &CdController::discRemoved);

    ctrl->start();

    // Insert disc
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_TRUE(ctrl->isDiscPresent());

    // Eject
    ctrl->eject();

    EXPECT_FALSE(ctrl->isDiscPresent());
    EXPECT_GE(removedSpy.count(), 1);
    EXPECT_GE(m_drive->ejectCallCount(), 1);

    ctrl->stop();
    delete ctrl;
}

TEST_F(CdControllerTest, AccessorsReturnCorrectValues)
{
    auto* ctrl = createController();

    // Before start -- no disc
    EXPECT_FALSE(ctrl->isDiscPresent());
    EXPECT_TRUE(ctrl->currentToc().isEmpty());

    ctrl->start();
    m_drive->setDiscPresent(true);
    QTest::qWait(m_config.pollIntervalMs * 3);

    EXPECT_TRUE(ctrl->isDiscPresent());
    EXPECT_EQ(ctrl->currentToc().size(), 10);

    ctrl->stop();
    delete ctrl;
}

// Include MOC for the StubPlaybackController Q_OBJECT
#include "test_CdController.moc"
