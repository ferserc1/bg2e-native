//
#pragma once

#include <string>
#include <filesystem>

class AppDelegate;

class Document
{
public:
    Document(AppDelegate * del);

    void setPath(const std::filesystem::path& path);
    inline const std::filesystem::path& path() const { return _path; }

    void setUnsavedChanges(bool s);
    bool unsavedChanges() const;

    void setStatus(const std::filesystem::path& path, bool unsavedChanges);

    void updateStatus();

protected:
    std::filesystem::path _path;
    bool _unsavedChanges = false;

    AppDelegate * _appDelegate;
};
