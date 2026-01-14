#pragma once
#include "pch.h"
#include <string>
#include <atomic>
#include <chrono>

namespace ZipSpark
{
    /// <summary>
    /// Progress tracking for archive extraction operations
    /// </summary>
    class ExtractionProgress
    {
    public:
        /// <summary>
        /// Current file being extracted
        /// </summary>
        std::wstring currentFile;

        /// <summary>
        /// Number of files processed so far
        /// </summary>
        std::atomic<uint32_t> filesProcessed{ 0 };

        /// <summary>
        /// Total number of files to extract
        /// </summary>
        uint32_t totalFiles = 0;

        /// <summary>
        /// Bytes processed so far
        /// </summary>
        std::atomic<uint64_t> bytesProcessed{ 0 };

        /// <summary>
        /// Total bytes to extract
        /// </summary>
        uint64_t totalBytes = 0;

        /// <summary>
        /// Extraction start time
        /// </summary>
        std::chrono::steady_clock::time_point startTime;

        /// <summary>
        /// Whether extraction has been cancelled
        /// </summary>
        std::atomic<bool> isCancelled{ false };

        /// <summary>
        /// Whether extraction is paused
        /// </summary>
        std::atomic<bool> isPaused{ false };

        /// <summary>
        /// Get overall progress percentage (0-100)
        /// </summary>
        double GetProgressPercentage() const
        {
            if (totalBytes == 0) return 0.0;
            return (static_cast<double>(bytesProcessed) / totalBytes) * 100.0;
        }

        /// <summary>
        /// Get current extraction speed in bytes per second
        /// </summary>
        double GetSpeedBytesPerSecond() const
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            if (elapsed == 0) return 0.0;
            return static_cast<double>(bytesProcessed) / elapsed;
        }

        /// <summary>
        /// Get estimated time remaining in seconds
        /// </summary>
        uint64_t GetEstimatedTimeRemaining() const
        {
            double speed = GetSpeedBytesPerSecond();
            if (speed == 0.0) return 0;
            uint64_t remaining = totalBytes - bytesProcessed;
            return static_cast<uint64_t>(remaining / speed);
        }

        /// <summary>
        /// Format speed as human-readable string (e.g., "5.2 MB/s")
        /// </summary>
        std::wstring GetFormattedSpeed() const
        {
            double speed = GetSpeedBytesPerSecond();
            if (speed < 1024) return std::to_wstring(static_cast<int>(speed)) + L" B/s";
            if (speed < 1024 * 1024) return std::to_wstring(static_cast<int>(speed / 1024)) + L" KB/s";
            return std::to_wstring(speed / (1024 * 1024)) + L" MB/s";
        }

        /// <summary>
        /// Format ETA as human-readable string (e.g., "2m 30s")
        /// </summary>
        std::wstring GetFormattedETA() const
        {
            uint64_t seconds = GetEstimatedTimeRemaining();
            if (seconds < 60) return std::to_wstring(seconds) + L"s";
            uint64_t minutes = seconds / 60;
            uint64_t remainingSeconds = seconds % 60;
            if (minutes < 60) return std::to_wstring(minutes) + L"m " + std::to_wstring(remainingSeconds) + L"s";
            uint64_t hours = minutes / 60;
            uint64_t remainingMinutes = minutes % 60;
            return std::to_wstring(hours) + L"h " + std::to_wstring(remainingMinutes) + L"m";
        }
    };

    /// <summary>
    /// Callback interface for progress updates
    /// </summary>
    class IProgressCallback
    {
    public:
        virtual ~IProgressCallback() = default;

        /// <summary>
        /// Called when progress is updated
        /// </summary>
        virtual void OnProgressUpdate(const ExtractionProgress& progress) = 0;

        /// <summary>
        /// Called when extraction completes successfully
        /// </summary>
        virtual void OnComplete() = 0;

        /// <summary>
        /// Called when extraction fails
        /// </summary>
        virtual void OnError(const std::wstring& errorMessage) = 0;
    };
}
