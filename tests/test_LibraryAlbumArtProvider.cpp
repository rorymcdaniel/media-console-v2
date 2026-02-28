#include <gtest/gtest.h>

#ifdef HAS_SNDFILE

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "library/LibraryAlbumArtProvider.h"

class LibraryAlbumArtProviderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char arg0[] = "test";
            static char* argv[] = { arg0 };
            new QCoreApplication(argc, argv);
        }

        m_tempDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_tempDir->isValid());
        m_cacheDir = m_tempDir->path() + "/art-cache";
    }

    void TearDown() override { m_tempDir.reset(); }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_cacheDir;
};

TEST_F(LibraryAlbumArtProviderTest, ConstructionCreatesCacheDir)
{
    LibraryAlbumArtProvider provider(m_cacheDir);
    EXPECT_TRUE(QDir(m_cacheDir).exists());
}

TEST_F(LibraryAlbumArtProviderTest, ComputeHashIsConsistent)
{
    LibraryAlbumArtProvider provider(m_cacheDir);

    QString hash1 = provider.computeHash("Artist", "Album");
    QString hash2 = provider.computeHash("Artist", "Album");
    EXPECT_EQ(hash1, hash2);
    EXPECT_FALSE(hash1.isEmpty());
}

TEST_F(LibraryAlbumArtProviderTest, ComputeHashDiffersForDifferentInput)
{
    LibraryAlbumArtProvider provider(m_cacheDir);

    QString hash1 = provider.computeHash("Artist A", "Album 1");
    QString hash2 = provider.computeHash("Artist B", "Album 2");
    EXPECT_NE(hash1, hash2);
}

TEST_F(LibraryAlbumArtProviderTest, ComputeHashIsSha1Hex)
{
    LibraryAlbumArtProvider provider(m_cacheDir);

    QString hash = provider.computeHash("Test Artist", "Test Album");
    // SHA-1 hex is 40 characters
    EXPECT_EQ(hash.length(), 40);

    // Verify it matches manual SHA-1 computation
    QByteArray data = QString("Test ArtistTest Album").toUtf8();
    QString expected = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    EXPECT_EQ(hash, expected);
}

TEST_F(LibraryAlbumArtProviderTest, HasCachedArtReturnsFalseForUncached)
{
    LibraryAlbumArtProvider provider(m_cacheDir);
    EXPECT_FALSE(provider.hasCachedArt("Unknown Artist", "Unknown Album"));
}

TEST_F(LibraryAlbumArtProviderTest, GetCachedArtReturnsEmptyForUncached)
{
    LibraryAlbumArtProvider provider(m_cacheDir);
    auto art = provider.getCachedArt("Unknown Artist", "Unknown Album");
    EXPECT_TRUE(art.frontPath.isEmpty());
    EXPECT_TRUE(art.backPath.isEmpty());
}

TEST_F(LibraryAlbumArtProviderTest, FileFallbackFindsCoverJpg)
{
    // Create a mock album directory with cover.jpg
    QString albumDir = m_tempDir->path() + "/music/artist/album";
    QDir().mkpath(albumDir);

    // Write a fake JPEG (starts with JFIF-like data, not PNG)
    QFile coverFile(albumDir + "/cover.jpg");
    ASSERT_TRUE(coverFile.open(QIODevice::WriteOnly));
    coverFile.write(QByteArray(100, '\xFF'));
    coverFile.close();

    // Create a dummy FLAC path (won't exist, but we need it for albumDir detection)
    QString flacPath = albumDir + "/01.flac";

    LibraryAlbumArtProvider provider(m_cacheDir);
    auto art = provider.extractArt(flacPath, "TestArtist", "TestAlbum");

    // Should have found cover.jpg as front art via fallback
    EXPECT_FALSE(art.frontPath.isEmpty());
    EXPECT_TRUE(QFile::exists(art.frontPath));

    // Should be in cache dir, not in album dir (never modify music library)
    EXPECT_TRUE(art.frontPath.startsWith(m_cacheDir));
}

TEST_F(LibraryAlbumArtProviderTest, HasCachedArtReturnsTrueAfterExtraction)
{
    // Create a mock album directory with cover.jpg
    QString albumDir = m_tempDir->path() + "/music/artist2/album2";
    QDir().mkpath(albumDir);

    QFile coverFile(albumDir + "/cover.jpg");
    ASSERT_TRUE(coverFile.open(QIODevice::WriteOnly));
    coverFile.write(QByteArray(100, '\xFF'));
    coverFile.close();

    QString flacPath = albumDir + "/01.flac";

    LibraryAlbumArtProvider provider(m_cacheDir);
    provider.extractArt(flacPath, "Artist2", "Album2");

    EXPECT_TRUE(provider.hasCachedArt("Artist2", "Album2"));
}

TEST_F(LibraryAlbumArtProviderTest, DetectsExtensionAsPngForPngMagic)
{
    // Create a mock album directory with a PNG file named cover.jpg (wrong extension)
    QString albumDir = m_tempDir->path() + "/music/artist3/album3";
    QDir().mkpath(albumDir);

    // Write PNG magic bytes
    QByteArray pngData;
    pngData.append("\x89PNG");
    pngData.append(QByteArray(96, '\x00'));

    QFile coverFile(albumDir + "/cover.png");
    ASSERT_TRUE(coverFile.open(QIODevice::WriteOnly));
    coverFile.write(pngData);
    coverFile.close();

    QString flacPath = albumDir + "/01.flac";

    LibraryAlbumArtProvider provider(m_cacheDir);
    auto art = provider.extractArt(flacPath, "Artist3", "Album3");

    // Should detect as PNG and save with .png extension
    EXPECT_FALSE(art.frontPath.isEmpty());
    EXPECT_TRUE(art.frontPath.endsWith(".png"));
}

#endif // HAS_SNDFILE
