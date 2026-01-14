#pragma once
#include "pch.h"
#include <string>
#include <Windows.h>
#include <winrt/Windows.Foundation.h>

namespace ZipSpark
{
    /// <summary>
    /// Error codes for ZipSpark operations
    /// </summary>
    enum class ErrorCode
    {
        Success = 0,
        FileNotFound,
        ArchiveNotFound,
        DestinationNotFound,
        AccessDenied,
        ArchiveCorrupted,
        UnsupportedFormat,
        InsufficientSpace,
        PasswordRequired,
        IncorrectPassword,
        ExtractionFailed,
        CancellationRequested,
        UnknownError
    };

    /// <summary>
    /// Centralized error handling for ZipSpark
    /// </summary>
    class ErrorHandler
    {
    public:
        /// <summary>
        /// Get user-friendly error message for an error code
        /// </summary>
        static std::wstring GetErrorMessage(ErrorCode code)
        {
            switch (code)
            {
            case ErrorCode::Success:
                return L"Operation completed successfully.";
            case ErrorCode::FileNotFound:
                return L"The archive file could not be found.";
            case ErrorCode::ArchiveNotFound:
                return L"The archive could not be opened or accessed.";
            case ErrorCode::DestinationNotFound:
                return L"The destination folder could not be accessed.";
            case ErrorCode::AccessDenied:
                return L"Access denied. Please check file permissions.";
            case ErrorCode::ArchiveCorrupted:
                return L"The archive appears to be corrupted or incomplete.";
            case ErrorCode::UnsupportedFormat:
                return L"This archive format is not supported.";
            case ErrorCode::InsufficientSpace:
                return L"Insufficient disk space to extract the archive.";
            case ErrorCode::PasswordRequired:
                return L"This archive is password-protected. Please provide a password.";
            case ErrorCode::IncorrectPassword:
                return L"The password provided is incorrect.";
            case ErrorCode::ExtractionFailed:
                return L"Extraction failed. Please check the log for details.";
            case ErrorCode::CancellationRequested:
                return L"Extraction was cancelled by the user.";
            default:
                return L"An unknown error occurred.";
            }
        }

        /// <summary>
        /// Get detailed error message with context
        /// </summary>
        static std::wstring GetDetailedErrorMessage(ErrorCode code, const std::wstring& context)
        {
            std::wstring baseMessage = GetErrorMessage(code);
            if (!context.empty())
            {
                return baseMessage + L"\n\nDetails: " + context;
            }
            return baseMessage;
        }

        /// <summary>
        /// Map Windows error code to ZipSpark error code
        /// </summary>
        static ErrorCode FromWindowsError(DWORD windowsError)
        {
            switch (windowsError)
            {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return ErrorCode::FileNotFound;
            case ERROR_ACCESS_DENIED:
                return ErrorCode::AccessDenied;
            case ERROR_DISK_FULL:
            case ERROR_HANDLE_DISK_FULL:
                return ErrorCode::InsufficientSpace;
            default:
                return ErrorCode::UnknownError;
            }
        }

        /// <summary>
        /// Map HRESULT to ZipSpark error code
        /// </summary>
        static ErrorCode FromHResult(HRESULT hr)
        {
            if (SUCCEEDED(hr))
                return ErrorCode::Success;

            // Map common HRESULTs
            switch (hr)
            {
            case E_ACCESSDENIED:
                return ErrorCode::AccessDenied;
            case E_OUTOFMEMORY:
                return ErrorCode::InsufficientSpace;
            default:
                return ErrorCode::UnknownError;
            }
        }
    };
}
