#pragma once

#include <cstdint>

/// Buffer statistics for audio playback monitoring.
/// Tracks xrun events, per-read latency, and read errors.
/// Logged to media.audio category only — not exposed in UI.
struct AudioBufferStats
{
    uint32_t xrunCount = 0;
    double readLatencyAvgUs = 0.0;
    double readLatencyMaxUs = 0.0;
    uint32_t errorCount = 0;
    uint64_t totalReads = 0;
    double readLatencySumUs = 0.0; // Internal: for computing running average

    void recordReadLatency(double microseconds)
    {
        ++totalReads;
        readLatencySumUs += microseconds;
        readLatencyAvgUs = readLatencySumUs / static_cast<double>(totalReads);
        if (microseconds > readLatencyMaxUs)
        {
            readLatencyMaxUs = microseconds;
        }
    }

    void recordXrun() { ++xrunCount; }

    void recordError() { ++errorCount; }

    void reset()
    {
        xrunCount = 0;
        readLatencyAvgUs = 0.0;
        readLatencyMaxUs = 0.0;
        errorCount = 0;
        totalReads = 0;
        readLatencySumUs = 0.0;
    }
};
