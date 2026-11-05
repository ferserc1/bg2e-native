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

#include "TextCommands.hpp"
#include <cstdio>
#include <set>

namespace bg2e_composer::command::component {
using namespace bg2e::app;
namespace {
size_t valueBytes(const TextAuthoringValue& value)
{
    size_t bytes = sizeof(value);
    if (const auto* text = std::get_if<TextValues>(&value))
    {
        bytes += text->paragraph.content.capacity() + text->paragraph.fontIdentifier.capacity();
    }
    else
    {
        const auto& fonts = std::get<FontDeclarations>(value);
        bytes += fonts.capacity() * sizeof(FontDeclarations::value_type);
        for (const auto& font : fonts)
        {
            bytes += font.identifier.capacity() + font.path.native().capacity() * sizeof(std::filesystem::path::value_type);
            bytes += font.notices.capacity() * sizeof(std::filesystem::path);
            for (const auto& notice : font.notices)
            {
                bytes += notice.native().capacity() * sizeof(std::filesystem::path::value_type);
            }
        }
    }
    return bytes;
}
}


EditText::EditText(std::weak_ptr<StageScene> stage, std::weak_ptr<bg2e::scene::Component> target,
    std::function<void()> effects) : _stage(std::move(stage)), _target(std::move(target)), _effects(std::move(effects))
{
}


TextAuthoringValue EditText::capture(bg2e::scene::Component& target)
{
    if (auto* text = dynamic_cast<bg2e::scene::TextComponent*>(&target))
    {
        return TextValues{text->text().parameters(), text->authoredSize(), text->pivot(), text->preserveAspect()};
    }
    return static_cast<bg2e::scene::TextFontComponent&>(target).declarations();
}


bool EditText::valid() const
{
    auto stage = _stage.lock();
    auto target = _target.lock();
    return stage && target && stage->contains(target->ownerNode()) &&
        (dynamic_cast<bg2e::scene::TextComponent*>(target.get()) ||
         dynamic_cast<bg2e::scene::TextFontComponent*>(target.get()));
}


bool EditText::write(bg2e::scene::Component& target, const TextAuthoringValue& value)
{
    if (auto* text = dynamic_cast<bg2e::scene::TextComponent*>(&target))
    {
        const auto* values = std::get_if<TextValues>(&value);
        if (!values)
        {
            return false;
        }
        auto normalized = text->text();
        normalized.setParameters(values->paragraph);
        text->setText(normalized);
        if (!(text->text() == normalized))
        {
            return false; // Nonthrowing CPU allocation failure, not success.
        }
        text->setSize(values->size);
        text->setPreserveAspect(values->preserveAspect);
        text->setPivot(values->pivot);
        return true;
    }
    auto* fonts = dynamic_cast<bg2e::scene::TextFontComponent*>(&target);
    const auto* declarations = std::get_if<FontDeclarations>(&value);
    if (!fonts || !declarations)
    {
        return false;
    }
    auto normalized = std::static_pointer_cast<bg2e::scene::TextFontComponent>(fonts->clone());
    if (!normalized)
    {
        return false;
    }
    normalized->setDeclarations(*declarations);
    fonts->setDeclarations(normalized->declarations());
    return fonts->declarations() == normalized->declarations();
}


const CommandInfo& EditText::info() const noexcept
{
    return _info;
}


CommandMemory EditText::memory() const
{
    auto target = _target.lock();
    const size_t current = target && valid() ? valueBytes(capture(*target)) : 0;
    return {sizeof(*this) + 65536 + 4 * (current + valueBytes(_before) + valueBytes(_after)), {}};
}


CommandMemory EditText::memoryForUpdate(const std::any& value) const
{
    auto result = memory();
    if (const auto* proposed = std::any_cast<TextAuthoringValue>(&value))
    {
        result.localBytes += 4 * valueBytes(*proposed);
    }
    return result;
}


CommandResult EditText::begin() noexcept
try
{
    if (!valid())
    {
        return {CommandStatus::Rejected, "Text/font target is no longer in the document"};
    }
    _before = capture(*_target.lock());
    _after = _before;
    _captured = true;
    return {};
}
catch (...)
{
    std::fprintf(stderr, "Cannot capture text/font edit; previous state retained.\n");
    return {CommandStatus::Rejected, "Cannot capture text/font edit"};
}


CommandResult EditText::update(const std::any& value) noexcept
{
    if (!_captured || !valid())
    {
        return {CommandStatus::Rejected, "Text/font interaction is stale"};
    }
    bool writing = false;
    try
    {
        const auto& proposed = std::any_cast<const TextAuthoringValue&>(value);
        if (const auto* declarations = std::get_if<FontDeclarations>(&proposed))
        {
            std::set<std::string> identifiers;
            bool validDeclarations = declarations->size() <= bg2e::scene::TextFontComponent::MaxDeclarations;
            for (auto declaration : *declarations)
            {
                // Face repair belongs to the CPU resource API. Incomplete
                // identifier/path drafts must not delete an existing row.
                declaration.faceIndex = 0;
                validDeclarations = bg2e::scene::TextFontComponent::validateDeclaration(declaration) &&
                    identifiers.insert(declaration.identifier).second && validDeclarations;
            }
            if (!validDeclarations)
            {
                std::fprintf(stderr, "Incomplete/duplicate font declaration draft; previous declarations retained.\n");
                return {CommandStatus::NoOp, "Correct the identifier/path before completing the edit"};
            }
        }
        writing = true;
        if (!write(*_target.lock(), proposed))
        {
            return restore(_after).accepted() ? CommandResult{CommandStatus::Rejected, "Text/font edit unavailable"} :
                CommandResult{CommandStatus::Faulted, "Text/font preview recovery failed"};
        }
        auto next = capture(*_target.lock());
        if (next == _after)
        {
            return {CommandStatus::NoOp, {}};
        }
        if (_effects)
        {
            _effects();
        }
        _changed = next != _before;
        _after = std::move(next);
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        std::fprintf(stderr, "Cannot apply text/font preview; recovering previous state.\n");
        if (writing && !restore(_after).accepted())
        {
            return {CommandStatus::Faulted, "Text/font preview recovery failed"};
        }
        return {CommandStatus::Rejected, "Text/font edit unavailable"};
    }
}


bool EditText::changed() const noexcept
{
    return _changed;
}


CommandResult EditText::execute() noexcept
{
    return _captured ? redo() : begin();
}


CommandResult EditText::restore(const TextAuthoringValue& value) noexcept
{
    std::optional<TextAuthoringValue> recovery;
    bool writing = false;
    try
    {
        if (!_captured || !valid())
        {
            return {CommandStatus::Rejected, "Text/font snapshot is stale"};
        }
        recovery.emplace(capture(*_target.lock()));
        writing = true;
        if (write(*_target.lock(), value))
        {
            if (_effects)
            {
                _effects();
            }
            return {CommandStatus::Applied, {}};
        }
    }
    catch (...)
    {
        // Restore the preceding preview if publication/effects were incomplete.
    }
    std::fprintf(stderr, "Cannot restore text/font values; recovering preceding state.\n");
    if (writing)
    {
        try
        {
            if (!write(*_target.lock(), *recovery))
            {
                return {CommandStatus::Faulted, "Text/font replay recovery failed"};
            }
            if (_effects)
            {
                _effects();
            }
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Text/font replay recovery failed"};
        }
    }
    return {CommandStatus::Failed, "Text/font restoration unavailable"};
}


CommandResult EditText::undo() noexcept
{
    return restore(_before);
}


CommandResult EditText::redo() noexcept
{
    return restore(_after);
}

}
