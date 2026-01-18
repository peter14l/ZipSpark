#pragma once
#include "IExtractionEngine.h"
#include "../Core/ArchiveInfo.h"
#include "../Core/ExtractionOptions.h"
#include <atomic>

namespace ZipSpark {

/// <summary>
/// Extraction engine using libarchive for multi-format support
/// Supports: 7z, RAR, TAR, GZ, XZ, TAR.GZ, TAR.XZ
/// </summary>
class LibArchiveEngine : public IExtractionEngine
{
public:
    LibArchiveEngine();
    ~LibArchiveEngine();

    bool CanHandle(const std::wstring& archivePath) override;
    ArchiveInfo GetArchiveInfo(const std::wstring& archivePath) override;
    void Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback) override;
    
private:
    void ExtractInternal(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback);
    void Cancel() override;
    std::wstring GetEngineName() const override { return L"libarchive"; }

private:
    std::atomic<bool> m_cancelled{false};
    
    // Helper methods
    std::wstring DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options);
    bool IsSupportedFormat(const std::wstring& extension);
};

} // namespace ZipSpark
