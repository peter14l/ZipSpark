#pragma once
#include "pch.h"
#include <string>
#include <cstdint>

namespace ZipSpark
{
    /// <summary>
    /// Supported archive format types
    /// </summary>
    enum class ArchiveFormat
    {
        Unknown,
        Zip,
        SevenZip,
        Rar,
        Tar,
        Gzip,
        TarGz,
        TarXz,
        Xz
    };

    /// <summary>
    /// Metadata and information about an archive file
    /// </summary>
    struct ArchiveInfo
    {
        /// <summary>
        /// Full path to the archive file
        /// </summary>
        std::wstring archivePath;

        /// <summary>
        /// Detected format of the archive
        /// </summary>
        ArchiveFormat format = ArchiveFormat::Unknown;

        /// <summary>
        /// Total uncompressed size in bytes
        /// </summary>
        uint64_t totalSize = 0;

        /// <summary>
        /// Number of files in the archive
        /// </summary>
        uint32_t fileCount = 0;

        /// <summary>
        /// Number of directories in the archive
        /// </summary>
        uint32_t directoryCount = 0;

        /// <summary>
        /// Whether the archive is encrypted/password protected
        /// </summary>
        bool isEncrypted = false;

        /// <summary>
        /// Destination path for extraction (context-aware)
        /// </summary>
        std::wstring destinationPath;

        /// <summary>
        /// Whether the archive has a single root folder
        /// (if true, extract directly; if false, create subfolder)
        /// </summary>
        bool hasSingleRoot = false;

        /// <summary>
        /// Get format as string for display
        /// </summary>
        std::wstring GetFormatString() const
        {
            switch (format)
            {
            case ArchiveFormat::Zip: return L"ZIP";
            case ArchiveFormat::SevenZip: return L"7z";
            case ArchiveFormat::Rar: return L"RAR";
            case ArchiveFormat::Tar: return L"TAR";
            case ArchiveFormat::Gzip: return L"GZ";
            case ArchiveFormat::TarGz: return L"TAR.GZ";
            case ArchiveFormat::TarXz: return L"TAR.XZ";
            case ArchiveFormat::Xz: return L"XZ";
            default: return L"Unknown";
            }
        }
    };
}
