#pragma once
#include "IExtractionEngine.h"
#include <memory>

namespace ZipSpark {

class EngineFactory
{
public:
    static std::unique_ptr<IExtractionEngine> CreateEngine(const std::wstring& archivePath);
    static ArchiveFormat DetectFormat(const std::wstring& archivePath);
};

} // namespace ZipSpark
