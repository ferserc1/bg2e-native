/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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

