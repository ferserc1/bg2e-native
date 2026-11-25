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

#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/db/file.hpp>
#include "detail/ToolbarConfiguration.hpp"

#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_set>

namespace bg2e::ui {

namespace {

namespace fs = std::filesystem;

constexpr std::string_view version = "version=1";
constexpr std::size_t maximumFileSize = 64 * 1024;

struct StoredToolbarSettings {
    ToolbarConfiguration appearance;
    std::optional<std::vector<std::string>> left;
    std::optional<std::vector<std::string>> right;
};

bool isPlainIdCharacter(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-';
}

bool readId(std::string_view encoded, std::string& id)
{
    if (encoded == ToolbarLayout::Separator)
    {
        id = ToolbarLayout::Separator;
        return true;
    }

    id.clear();
    while (!encoded.empty())
    {
        if (isPlainIdCharacter(encoded.front()))
        {
            id.push_back(encoded.front());
            encoded.remove_prefix(1);
        }
        else if (encoded.front() == '%' && encoded.size() >= 3)
        {
            unsigned value = 0;
            const auto first = encoded.data() + 1;
            const auto last = first + 2;
            const auto result = std::from_chars(first, last, value, 16);
            if (result.ec != std::errc{} || result.ptr != last)
            {
                return false;
            }
            id.push_back(static_cast<char>(value));
            encoded.remove_prefix(3);
        }
        else
        {
            return false;
        }
    }
    return !id.empty();
}

std::vector<std::string> readSide(std::string_view value)
{
    std::vector<std::string> ids;
    std::string id;
    while (!value.empty())
    {
        const auto separator = value.find(',');
        if (readId(value.substr(0, separator), id))
        {
            ids.push_back(id);
        }
        if (separator == std::string_view::npos)
        {
            break;
        }
        value.remove_prefix(separator + 1);
    }
    return ids;
}

void writeSide(std::ostream& out, const std::vector<std::string>& ids)
{
    constexpr std::string_view hex = "0123456789ABCDEF";
    bool first = true;
    for (const auto& id : ids)
    {
        if (!first)
        {
            out.put(',');
        }
        first = false;
        if (id == ToolbarLayout::Separator)
        {
            out << ToolbarLayout::Separator;
            continue;
        }
        for (const unsigned char ch : id)
        {
            if (isPlainIdCharacter(static_cast<char>(ch)))
            {
                out.put(static_cast<char>(ch));
            }
            else
            {
                out << '%' << hex[ch >> 4] << hex[ch & 0x0f];
            }
        }
    }
    out.put('\n');
}

std::optional<StoredToolbarSettings> readSettings(const fs::path& path)
{
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error)
    {
        return std::nullopt;
    }

    std::ifstream input(path, std::ios::binary);
    std::string contents(maximumFileSize + 1, '\0');
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!input.eof() || input.bad() || input.gcount() > static_cast<std::streamsize>(maximumFileSize))
    {
        return std::nullopt;
    }

    contents.resize(static_cast<std::size_t>(input.gcount()));
    std::istringstream lines(std::move(contents));
    std::string line;
    const auto nextLine = [&]()
    {
        if (!std::getline(lines, line))
        {
            return false;
        }
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        return true;
    };

    if (!nextLine() || line != version)
    {
        return std::nullopt;
    }

    StoredToolbarSettings settings;
    while (nextLine())
    {
        if (line == "end")
        {
            if (lines.peek() == std::char_traits<char>::eof())
            {
                return settings;
            }
            return std::nullopt;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string_view field(line);
        const auto key = field.substr(0, separator);
        const auto value = field.substr(separator + 1);
        if (key == "presentation")
        {
            const auto first = value.data();
            const auto last = first + value.size();
            int selection = 0;
            const auto result = std::from_chars(first, last, selection);
            if (result.ec == std::errc{} && result.ptr == last &&
                selection >= static_cast<int>(ToolbarPresentation::Text) &&
                selection <= static_cast<int>(ToolbarPresentation::IconsAndText))
            {
                settings.appearance.presentation = static_cast<ToolbarPresentation>(selection);
            }
        }
        else if (key == "iconSize")
        {
            std::istringstream number{std::string(value)};
            number.imbue(std::locale::classic());
            float size = 0.0f;
            if (number >> size && number.eof() && std::isfinite(size) &&
                size >= detail::minimumToolbarIconSize && size <= detail::maximumToolbarIconSize)
            {
                settings.appearance.iconSize = size;
            }
        }
        else if (key == "left")
        {
            settings.left = readSide(value);
        }
        else if (key == "right")
        {
            settings.right = readSide(value);
        }
    }

    // A publication always writes a terminator, including for an empty layout.
    return std::nullopt;
}

struct StagedFileCleanup {
    const fs::path& file;
    const fs::path& directory;

    ~StagedFileCleanup()
    {
        std::error_code ignored;
        fs::remove(file, ignored);
        fs::remove(directory, ignored);
    }
};

}

void Toolbar::loadCustomization()
{
    if (_customizationPath.empty())
    {
        return;
    }

    _customLayout.reset();
    try
    {
        std::error_code error;
        if (!fs::exists(_customizationPath, error) && !error)
        {
            // Preserve the legacy session appearance during first-time migration.
            // Another running instance might already have published its settings.
            saveCustomization(false);
            return;
        }

        if (auto settings = readSettings(_customizationPath))
        {
            if (settings->left || settings->right)
            {
                ToolbarLayout layout{
                    settings->left ? std::move(*settings->left) : _defaultLayout.left,
                    settings->right ? std::move(*settings->right) : _defaultLayout.right
                };
                // Keep every separator and give the left side priority when an
                // action ID occurs more than once.
                std::unordered_set<std::string> used;
                const auto invalid = [&](const std::string& id)
                {
                    return id != ToolbarLayout::Separator && (!_actions.contains(id) || !used.insert(id).second);
                };
                std::erase_if(layout.left, invalid);
                std::erase_if(layout.right, invalid);
                _customLayout = std::move(layout);
            }
            _configuration = settings->appearance;
            return;
        }
    }
    catch (const std::exception&)
    {
        // Corrupt and inaccessible settings both fall back silently.
    }

    _configuration = ToolbarConfiguration{};
}

void Toolbar::saveCustomization(bool replaceExisting) const
{
    if (_customizationPath.empty())
    {
        return;
    }

    try
    {
        const auto settings = configuration();
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << version << '\n'
            << "presentation=" << static_cast<int>(settings.presentation) << '\n'
            << "iconSize=" << std::setprecision(std::numeric_limits<float>::max_digits10) << settings.iconSize << '\n';
        if (_customLayout)
        {
            out << "left=";
            writeSide(out, _customLayout->left);
            out << "right=";
            writeSide(out, _customLayout->right);
        }
        out << "end\n";
        if (!out)
        {
            return;
        }
        const auto contents = std::move(out).str();
        if (contents.size() > maximumFileSize)
        {
            return;
        }

        // Stage on the same filesystem. Atomically acquiring a directory
        // prevents concurrent writers from sharing a partially written file.
        static std::atomic<uint64_t> sequence{0};
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto directory = _customizationPath;
        directory += ".tmp-" + std::to_string(timestamp) + "-" +
            std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
        const auto staged = directory / "settings";
        std::error_code error;
        if (!fs::create_directory(directory, error))
        {
            return;
        }

        StagedFileCleanup cleanup{staged, directory};
        std::ofstream file(staged, std::ios::binary | std::ios::trunc);
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        file.close();
        if (file)
        {
            db::publishFile(staged, _customizationPath, replaceExisting);
        }
    }
    catch (const std::exception&)
    {
        // Keep the in-memory settings when storage is temporarily unavailable.
    }
}

}
