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

#include <bg2e/db/file.hpp>
#include <stdexcept>
#include <system_error>
#include <cerrno>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <stdio.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace bg2e::db {
void publishFile(const std::filesystem::path& staged, const std::filesystem::path& destination,
    bool replaceExisting)
{
    if (!std::filesystem::is_regular_file(staged) || std::filesystem::is_symlink(staged) || staged == destination ||
        (std::filesystem::exists(destination) && std::filesystem::equivalent(staged,destination)))
    {
        throw std::invalid_argument("Atomic publication requires a distinct regular staged file");
    }
#ifdef _WIN32
    const DWORD flags = MOVEFILE_WRITE_THROUGH | (replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0);
    if (!MoveFileExW(staged.c_str(),destination.c_str(),flags))
    {
        throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"Publish file");
    }
#else
    if (replaceExisting)
    {
        std::filesystem::rename(staged,destination);
        return;
    }
#ifdef __APPLE__
    if (::renamex_np(staged.c_str(),destination.c_str(),RENAME_EXCL) != 0)
    {
        throw std::system_error(errno,std::generic_category(),"Publish file without replacement");
    }
#elif defined(__linux__) && defined(SYS_renameat2)
    if (::syscall(SYS_renameat2,AT_FDCWD,staged.c_str(),AT_FDCWD,destination.c_str(),RENAME_NOREPLACE) != 0)
    {
        throw std::system_error(errno,std::generic_category(),"Publish file without replacement");
    }
#else
    // Portable fallback has OS-enforced no-clobber semantics but needs hard links.
    std::filesystem::create_hard_link(staged,destination);
    std::error_code ignored;
    std::filesystem::remove(staged,ignored);
#endif
#endif
}
}
