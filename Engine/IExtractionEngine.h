#pragma once
#include "pch.h"
#include "../Core/ArchiveInfo.h"
#include "../Core/ExtractionOptions.h"
#include "../Core/ExtractionProgress.h"
#include <memory>

namespace ZipSpark::Engine
{
    /// <summary>
    /// Abstract interface for archive extraction engines
    /// Implementations handle specific archive formats or use different libraries
    /// </summary>
    class IExtractionEngine
    {
    public:
        virtual ~IExtractionEngine() = default;

        /// <summary>
        /// Check if this engine can handle the specified archive
        /// </summary>
        /// <param name="archivePath">Full path to the archive file</param>
        /// <returns>True if this engine supports the archive format</returns>
        virtual bool CanHandle(const std::wstring& archivePath) = 0;

        /// <summary>
        /// Get detailed information about an archive without extracting
        /// </summary>
        /// <param name="archivePath">Full path to the archive file</param>
        /// <returns>Archive metadata and information</returns>
        virtual Core::ArchiveInfo GetArchiveInfo(const std::wstring& archivePath) = 0;

        /// <summary>
        /// Extract an archive with the specified options
        /// </summary>
        /// <param name="info">Archive information (from GetArchiveInfo)</param>
        /// <param name="options">Extraction configuration</param>
        /// <param name="callback">Progress callback for UI updates</param>
        virtual void Extract(
            const Core::ArchiveInfo& info,
            const Core::ExtractionOptions& options,
            Core::IProgressCallback* callback) = 0;

        /// <summary>
        /// Cancel an ongoing extraction operation
        /// </summary>
        virtual void Cancel() = 0;

        /// <summary>
        /// Get the name of this extraction engine
        /// </summary>
        virtual std::wstring GetEngineName() const = 0;
    };
}
