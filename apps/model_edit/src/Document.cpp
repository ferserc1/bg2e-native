//

#include <Document.hpp>

#include <AppDelegate.hpp>

Document::Document(AppDelegate* del)
    :_appDelegate(del)
{

}

void Document::setPath(const std::filesystem::path& path)
{
    _path = path;
    updateStatus();
}

void Document::setUnsavedChanges(bool s)
{
    _unsavedChanges = s;
    updateStatus();
}

bool Document::unsavedChanges() const
{
    return _unsavedChanges;
}

void Document::setStatus(const std::filesystem::path& path, bool unsavedChanges)
{
    _path = path;
    _unsavedChanges = unsavedChanges;
    updateStatus();
}

void Document::updateStatus()
{
    if (_path.empty())
    {
        _appDelegate->fileStatus()->setText("There are no open files.");
    }
    else
    {
        _appDelegate->fileStatus()->setText("File: " + _path.string());
    }

    if (_unsavedChanges)
    {
        _appDelegate->saveStatus()->setText("Unsaved Changes");
    }
    else
    {
        _appDelegate->saveStatus()->setText("All changes saved");
    }
}

