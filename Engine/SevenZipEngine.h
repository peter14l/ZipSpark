#pragma once
#include "IExtractionEngine.h"
#include <string>
#include <atomic>

namespace ZipSpark {

/// <summary>
/// Robust Extraction Engine that runs 7z.exe as a subprocess.
/// Provides process isolation so crashes in 7-Zip do not affect the main app.
/// </summary>
class SevenZipEngine : public IExtractionEngine
{
public:
    SevenZipEngine();
    ~SevenZipEngine();

    bool CanHandle(const std::wstring& archivePath) override;
    ArchiveInfo GetArchiveInfo(const std::wstring& archivePath) override;
    void Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback) override;
    void CreateArchive(const std::wstring& destinationPath, const std::vector<std::wstring>& sourceFiles, const std::wstring& format, IProgressCallback* callback) override;
    void Cancel() override;
    std::wstring GetEngineName() const override { return L"7-Zip (Process)"; }
    
private:
    std::atomic<bool> m_cancelled{false};
    void* m_hSubProcess = nullptr; // HANDLE to the process

    // Helpers
    std::wstring DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options);
    std::wstring Get7zExePath();
    bool IsSupportedFormat(const std::wstring& extension);
};

} // namespace ZipSpark
