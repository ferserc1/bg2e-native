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

#include "../../../apps/bg2e_composer/src/Document.hpp"

#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <array>
#include <stdexcept>

namespace {
using namespace bg2e::app;
using bg2e_composer::Document;
using bg2e_composer::command::Dispatcher;

void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Recoverable document rejection contract failed");
    }
}

class Preview final : public CommandTransaction {
  public:
    const CommandInfo& info() const noexcept override
    {
        return _info;
    }

    CommandMemory memory() const override
    {
        return {};
    }

    CommandMemory memoryForUpdate(const std::any&) const override
    {
        return {};
    }

    CommandResult begin() noexcept override
    {
        return {};
    }

    CommandResult update(const std::any&) noexcept override
    {
        _changed = true;
        return {CommandStatus::Applied, {}};
    }

    bool changed() const noexcept override
    {
        return _changed;
    }

    CommandResult execute() noexcept override
    {
        return {CommandStatus::Rejected, "Preview requires adoption"};
    }

    CommandResult undo() noexcept override
    {
        _changed = false;
        return {CommandStatus::Applied, {}};
    }

    CommandResult redo() noexcept override
    {
        _changed = true;
        return {CommandStatus::Applied, {}};
    }

  private:
    CommandInfo _info{"Document rejection preview", "value"};
    bool _changed = false;
};

std::shared_ptr<Dispatcher> dispatcher()
{
    return std::make_shared<Dispatcher>(1, true,
        [](auto, auto*, auto run, auto) { run(); },
        [](const auto&) {}, [](auto) {});
}

void rejectedNotificationPreservesDirtyState()
{
    Document document;
    document.commands = dispatcher();
    auto& history = document.commands->history();
    require(history.beginTransaction(std::make_unique<Preview>()).accepted());
    // A legacy camera move must not crash even with an unchanged preview.
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Rejected);
    require(document.dirty && !history.dirty());
    document.synchronizeDirty();
    require(document.dirty && history.transactionActive());
    require(history.cancelTransaction().accepted());
    document.synchronizeDirty();
    require(document.dirty && history.dirty());
    require(document.pendingCompatibility == CommandCompatibility::Unspecified);
    require(history.undoCount() == 0 && history.redoCount() == 0);
    require(history.markSaved().accepted());
    document.synchronizeDirty();
    require(!document.dirty);
    require(document.markModified(CommandCompatibility::Unspecified).status == CommandStatus::Rejected);
    require(!document.dirty);
}

void pendingCompatibilityAndFaults()
{
    Document document;
    document.commands = dispatcher();
    auto& history = document.commands->history();
    require(history.beginTransaction(std::make_unique<Preview>()).accepted());
    require(history.updateTransaction(1).accepted());
    require(document.markModified(CommandCompatibility::ResetHistory).status == CommandStatus::Rejected);
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Rejected);
    require(history.commitTransaction().accepted());
    require(history.undoCount() == 1);
    document.synchronizeDirty();
    require(history.undoCount() == 0 && document.dirty);
    require(history.reportExternalFault().status == CommandStatus::Faulted);
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Faulted);
    document.synchronizeDirty();
    require(document.dirty && history.faulted());
}

void idleOrbitPreservesSavedState()
{
    Document document;
    document.commands = dispatcher();
    document.root = std::make_shared<bg2e::scene::Node>("saved camera");
    document.root->addComponent(new bg2e::scene::TransformComponent());
    bg2e::scene::OrbitCameraComponent source;
    source.setRotation({17.0f, 16.0f});
    source.setCenter({1.6639076471328735f, 0.5820321440696716f, 0.3408327784478593f});
    source.setDistance(9.630512365703176f);
    auto orbit = std::make_shared<bg2e::scene::OrbitCameraComponent>();
    orbit->deserialize(source.serializeReferences({}), {});
    document.root->addComponent(orbit);
    auto& history = document.commands->history();
    const auto checkIdle = [&]
    {
        const auto rotation = orbit->rotation();
        const auto center = orbit->center();
        const auto distance = orbit->distance();
        const auto saved = history.savedState();
        for (int frame = 0; frame < 600; ++frame)
        {
            const std::array<float, 6> deltas{0.0f, 1.0f / 30, 1.0f / 60,
                1.0f / 120, 1.0f / 144, 0.023f};
            orbit->update(deltas[frame % deltas.size()]);
            // Match Composer's exact navigation dirty-state comparison.
            if (orbit->rotation() != rotation || orbit->center() != center || orbit->distance() != distance)
            {
                document.markModified(CommandCompatibility::Preserve);
            }
            document.synchronizeDirty();
            require(!document.dirty && history.documentState() == saved);
        }
    };
    checkIdle(); // Freshly deserialized scene must remain saved.
    orbit->setCenter({2.0f, 3.0f, 4.0f});
    require(document.markModified(CommandCompatibility::Preserve).accepted());
    require(document.dirty);
    orbit->update(0.0f); // The same zero-delta update used during saving.
    require(history.markSaved().accepted());
    document.synchronizeDirty();
    require(!document.dirty);
    checkIdle(); // Saving must not be undone by subsequent idle frames.
}

void rejectionCallbackIsIsolated()
{
    auto commands = dispatcher();
    int completed = 0;
    const auto result = commands->submit({}, bg2e_composer::command::Boundary::CpuValue, {},
        [] { return true; }, [&](auto rejection)
        {
            require(rejection.status == CommandStatus::Rejected);
            ++completed;
            throw std::runtime_error("Injected notification failure");
        });
    require(result.status == CommandStatus::Rejected && completed == 1);
    require(!commands->history().faulted() && commands->pending() == 0);
}
}

int main()
{
    rejectedNotificationPreservesDirtyState();
    pendingCompatibilityAndFaults();
    rejectionCallbackIsIsolated();
    idleOrbitPreservesSavedState();
}
