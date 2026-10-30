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

#include "ImportCommand.hpp"

namespace bg2e_composer::command::scene {

ApplyImport::ApplyImport(std::unique_ptr<EditNodes> placement, std::shared_ptr<ImportRecord> record,
    std::shared_ptr<bg2e::scene::Node> candidate, std::function<bool()> valid)
    : _placement(std::move(placement)), _valid(std::move(valid)), _record(std::move(record)),
      _before(_record ? _record->node : std::weak_ptr<bg2e::scene::Node>{}), _after(candidate)
{
    _info.name = _placement->info().name;
}

const bg2e::app::CommandInfo& ApplyImport::info() const noexcept
{
    return _info;
}

bg2e::app::CommandMemory ApplyImport::memory() const
{
    auto result = _placement->memory();
    result.localBytes += sizeof(*this) + 65536;
    if (_record)
    {
        result.resources.push_back({_record, sizeof(ImportRecord)});
    }
    return result;
}

bg2e::app::CommandResult ApplyImport::execute() noexcept
{
    try
    {
        if (!_valid())
        {
            return {bg2e::app::CommandStatus::Rejected, "Prepared import source revision or job changed"};
        }
    }
    catch (...)
    {
        return {bg2e::app::CommandStatus::Rejected, "Prepared import validation failed"};
    }
    auto result = _placement->execute();
    if (result.status == bg2e::app::CommandStatus::Applied && _record)
    {
        _record->node = _after;
    }
    return result;
}

bg2e::app::CommandResult ApplyImport::undo() noexcept
{
    auto result = _placement->undo();
    if (result.status == bg2e::app::CommandStatus::Applied && _record)
    {
        _record->node = _before;
    }
    return result;
}

bg2e::app::CommandResult ApplyImport::redo() noexcept
{
    auto result = _placement->redo();
    if (result.status == bg2e::app::CommandStatus::Applied && _record)
    {
        _record->node = _after;
    }
    return result;
}

}
