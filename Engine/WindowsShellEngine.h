#pragma once
#include "IExtractionEngine.h"
#include <windows.h>
#include <shldisp.h>
#include <comdef.h>

namespace ZipSpark {

class WindowsShellEngine : public IExtractionEngine
{
public:
    WindowsShellEngine();
    ~WindowsShellEngine();

    // IExtractionEngine implementation
    bool CanHandle(const std::wstring& archivePath) override;
    ArchiveInfo GetArchiveInfo(const std::wstring& archivePath) override;
    void Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback) override;
    void Cancel() override;
    std::wstring GetEngineName() const override { return L"Windows Shell"; }

private:
    std::atomic<bool> m_cancelled{ false };
    
    bool InitializeCOM();
    void CleanupCOM();
    std::wstring DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options);
    bool AnalyzeArchiveStructure(const std::wstring& archivePath, int& rootItemCount);
};

} // namespace ZipSpark
