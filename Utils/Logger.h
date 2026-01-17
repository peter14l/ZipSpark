#pragma once
#include "pch.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <shlobj.h> // For SHGetKnownFolderPath

namespace ZipSpark
{
    /// <summary>
    /// Log severity levels
    /// </summary>
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    };

    /// <summary>
    /// Simple structured logger for ZipSpark
    /// Creates per-run log files with timestamps
    /// </summary>
    class Logger
    {
    public:
        /// <summary>
        /// Get the singleton logger instance
        /// </summary>
        static Logger& GetInstance()
        {
            static Logger instance;
            return instance;
        }

        /// <summary>
        /// Initialize the logger with a log file path
        /// </summary>
        void Initialize(const std::wstring& logDirectory)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (m_isInitialized) return; // Already initialized
            
            try
            {
                // Create log directory if it doesn't exist
                std::error_code ec;
                std::filesystem::create_directories(logDirectory, ec);
                
                if (ec)
                {
                    // Directory creation failed, try temp directory
                    wchar_t tempPath[MAX_PATH];
                    GetTempPathW(MAX_PATH, tempPath);
                    std::wstring tempDir = std::wstring(tempPath) + L"ZipSpark\\Logs";
                    std::filesystem::create_directories(tempDir, ec);
                    
                    if (ec)
                    {
                        // Can't create any directory, logging disabled
                        m_isInitialized = false;
                        return;
                    }
                    
                    // Use temp directory instead
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    std::tm tm;
                    localtime_s(&tm, &time);

                    std::wstringstream filename;
                    filename << tempDir << L"\\ZipSpark_"
                        << std::put_time(&tm, L"%Y%m%d_%H%M%S")
                        << L".log";

                    m_logFilePath = filename.str();
                    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
                    
                    if (m_logFile.is_open())
                    {
                        m_isInitialized = true;
                        // Write initial log entry
                        auto logNow = std::chrono::system_clock::now();
                        auto logTime = std::chrono::system_clock::to_time_t(logNow);
                        std::tm logTm;
                        localtime_s(&logTm, &logTime);
                        m_logFile << L"[" << std::put_time(&logTm, L"%Y-%m-%d %H:%M:%S") << L"] "
                                  << L"[WARN] Logger initialized at temp location (AppData failed): " << m_logFilePath << L"\n";
                        m_logFile.flush();
                    }
                    return;
                }

                // Generate timestamped log file name
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
                localtime_s(&tm, &time);

                std::wstringstream filename;
                filename << logDirectory << L"\\ZipSpark_"
                    << std::put_time(&tm, L"%Y%m%d_%H%M%S")
                    << L".log";

                m_logFilePath = filename.str();
                m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
                
                if (m_logFile.is_open())
                {
                    m_isInitialized = true;
                    // Write initial log entry
                    auto logNow = std::chrono::system_clock::now();
                    auto logTime = std::chrono::system_clock::to_time_t(logNow);
                    std::tm logTm;
                    localtime_s(&logTm, &logTime);
                    m_logFile << L"[" << std::put_time(&logTm, L"%Y-%m-%d %H:%M:%S") << L"] "
                              << L"[INFO] Logger initialized at: " << m_logFilePath << L"\n";
                    m_logFile.flush();
                }
            }
            catch (...)
            {
                // Complete failure - logging disabled
                m_isInitialized = false;
            }
        }

        /// <summary>
        /// Get the path to the current log file
        /// </summary>
        std::wstring GetLogFilePath() const
        {
            return m_logFilePath;
        }

        /// <summary>
        /// Log a message with specified severity
        /// </summary>
        void Log(LogLevel level, const std::wstring& message)
        {
            // Auto-initialize if not initialized (without holding the lock)
            if (!m_isInitialized)
            {
                // Get AppData local path
                wchar_t* localAppData = nullptr;
                if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData) == S_OK)
                {
                    std::wstring logDir = std::wstring(localAppData) + L"\\ZipSpark\\Logs";
                    CoTaskMemFree(localAppData);
                    Initialize(logDir);
                }
                else
                {
                    // Fallback to temp
                    wchar_t tempPath[MAX_PATH];
                    GetTempPathW(MAX_PATH, tempPath);
                    std::wstring logDir = std::wstring(tempPath) + L"ZipSpark\\Logs";
                    Initialize(logDir);
                }
                
                // If still not initialized after trying, give up
                if (!m_isInitialized)
                {
                    return;
                }
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &time);

            std::wstringstream logEntry;
            logEntry << L"[" << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S") << L"] "
                << L"[" << GetLevelString(level) << L"] "
                << message << L"\n";

            if (m_logFile.is_open())
            {
                m_logFile << logEntry.str();
                m_logFile.flush(); // Immediate flush to ensure logs are written even if app crashes
            }
        }

        /// <summary>
        /// Log debug message
        /// </summary>
        void Debug(const std::wstring& message)
        {
            Log(LogLevel::Debug, message);
        }

        /// <summary>
        /// Log info message
        /// </summary>
        void Info(const std::wstring& message)
        {
            Log(LogLevel::Info, message);
        }

        /// <summary>
        /// Log warning message
        /// </summary>
        void Warning(const std::wstring& message)
        {
            Log(LogLevel::Warning, message);
        }

        /// <summary>
        /// Log error message
        /// </summary>
        void Error(const std::wstring& message)
        {
            Log(LogLevel::Error, message);
        }

        ~Logger()
        {
            if (m_logFile.is_open())
            {
                Log(LogLevel::Info, L"Logger shutting down");
                m_logFile.close();
            }
        }

    private:
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::wstring GetLevelString(LogLevel level) const
        {
            switch (level)
            {
            case LogLevel::Debug: return L"DEBUG";
            case LogLevel::Info: return L"INFO";
            case LogLevel::Warning: return L"WARN";
            case LogLevel::Error: return L"ERROR";
            default: return L"UNKNOWN";
            }
        }

        std::wofstream m_logFile;
        std::wstring m_logFilePath;
        std::mutex m_mutex;
        bool m_isInitialized = false;
    };
}

// Convenience macros for logging
#define LOG_DEBUG(msg) ZipSpark::Logger::GetInstance().Debug(msg)
#define LOG_INFO(msg) ZipSpark::Logger::GetInstance().Info(msg)
#define LOG_WARNING(msg) ZipSpark::Logger::GetInstance().Warning(msg)
#define LOG_ERROR(msg) ZipSpark::Logger::GetInstance().Error(msg)
