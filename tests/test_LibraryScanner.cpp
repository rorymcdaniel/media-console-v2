#include <gtest/gtest.h>

#ifdef HAS_SNDFILE

#include <QCoreApplication>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "library/LibraryDatabase.h"
#include "library/LibraryScanner.h"

class LibraryScannerTest : public ::testing::Test
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
    }
};

TEST_F(LibraryScannerTest, ConstructionAndInitialState)
{
    LibraryScanner scanner;
    EXPECT_FALSE(scanner.isScanning());
}

TEST_F(LibraryScannerTest, ScanEmptyDirectoryCompletesImmediately)
{
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    LibraryScanner scanner;
    QSignalSpy completeSpy(&scanner, &LibraryScanner::scanComplete);

    QMap<QString, qint64> emptyMtimes;
    scanner.startScan(tempDir.path(), emptyMtimes);

    // Wait for async scan to complete
    ASSERT_TRUE(completeSpy.wait(5000));
    ASSERT_EQ(completeSpy.count(), 1);

    // No files processed, none skipped
    EXPECT_EQ(completeSpy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(completeSpy.at(0).at(1).toInt(), 0);
}

TEST_F(LibraryScannerTest, ScanNonexistentDirectoryCompletesWithZero)
{
    LibraryScanner scanner;
    QSignalSpy completeSpy(&scanner, &LibraryScanner::scanComplete);

    QMap<QString, qint64> emptyMtimes;
    scanner.startScan("/nonexistent/path/12345", emptyMtimes);

    ASSERT_TRUE(completeSpy.wait(5000));
    ASSERT_EQ(completeSpy.count(), 1);

    EXPECT_EQ(completeSpy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(completeSpy.at(0).at(1).toInt(), 0);
}

TEST_F(LibraryScannerTest, CancelStopsScan)
{
    LibraryScanner scanner;
    scanner.cancel();
    // Should not crash or hang
    EXPECT_FALSE(scanner.isScanning());
}

#endif // HAS_SNDFILE
