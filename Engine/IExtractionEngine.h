#pragma once
#include "../Core/ArchiveInfo.h"
#include "../Core/ExtractionOptions.h"
#include "../Core/ExtractionProgress.h"
#include "../Utils/ErrorHandler.h"
#include <string>

namespace ZipSpark {

// Abstract interface for archive extraction engines
class IExtractionEngine
{
public:
    virtual ~IExtractionEngine() = default;

    // Check if this engine can handle the given archive
    virtual bool CanHandle(const std::wstring& archivePath) = 0;

    // Get information about the archive
    virtual ArchiveInfo GetArchiveInfo(const std::wstring& archivePath) = 0;

    // Extract the archive
    virtual void Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback) = 0;

    // Cancel ongoing extraction
    virtual void Cancel() = 0;

    // Get engine name for logging
    virtual std::wstring GetEngineName() const = 0;
};

} // namespace ZipSpark
