#pragma once
#include "pch.h"
#include <string>
#include <optional>

namespace ZipSpark
{
    /// <summary>
    /// Policy for handling file conflicts during extraction
    /// </summary>
    enum class OverwritePolicy
    {
        Prompt,      // Ask user for each conflict
        AutoRename,  // Automatically rename conflicting files
        Overwrite,   // Replace existing files
        Skip         // Skip conflicting files
    };

    /// <summary>
    /// Configuration options for archive extraction
    /// </summary>
    struct ExtractionOptions
    {
        /// <summary>
        /// Destination directory for extraction
        /// </summary>
        std::wstring destinationPath;

        /// <summary>
        /// Whether to create a subfolder for extraction
        /// </summary>
        bool createSubfolder = true;

        /// <summary>
        /// Password for encrypted archives (if needed)
        /// </summary>
        std::optional<std::wstring> password;

        /// <summary>
        /// How to handle file conflicts
        /// </summary>
        OverwritePolicy overwritePolicy = OverwritePolicy::Prompt;

        /// <summary>
        /// Number of threads to use for extraction (0 = auto)
        /// </summary>
        uint32_t threadCount = 0;

        /// <summary>
        /// Buffer size for extraction operations (bytes)
        /// </summary>
        uint32_t bufferSize = 65536; // 64 KB default

        /// <summary>
        /// Whether to cache extracted data in memory for performance
        /// </summary>
        bool cacheToMemory = false;

        /// <summary>
        /// Whether to preserve file timestamps
        /// </summary>
        bool preserveTimestamps = true;

        /// <summary>
        /// Whether to preserve file permissions (where applicable)
        /// </summary>
        bool preservePermissions = true;

        /// <summary>
        /// Whether to create a subfolder if archive has multiple root items
        /// </summary>
        bool autoCreateSubfolder = true;
    };
}
