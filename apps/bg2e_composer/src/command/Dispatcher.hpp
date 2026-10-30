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

#pragma once

#include <bg2e/app/CommandQueue.hpp>
#include <unordered_map>

namespace bg2e_composer::command {

enum class Boundary { CpuValue, RetainedResource, Conservative };

struct Effects {
    bool lightmapSources = false;
    bool renderHistories = false;
    bool sceneReferences = false;
    bool tools = false;
    bool selection = false;
};

// Composer owns synchronization and effects. The app queue owns scheduling,
// history and policy mechanics. The step 3 commands contain CPU state only and
// survive renderer re-creation without retaining native realizations. Resource
// commands must supply their renderer/retirement lifecycle adapter before use.
class Dispatcher {
  public:
    using Scheduler = std::function<void(Boundary, const bg2e::app::CommandContext*,
        bg2e::app::CommandQueue::Task, bg2e::app::CommandQueue::Task)>;
    using EffectHandler = std::function<void(const Effects&)>;
    using Completion = bg2e::app::CommandQueue::Completion;

    Dispatcher(uint64_t generation, bool saved, Scheduler scheduler,
        EffectHandler effects, Completion completion);
    ~Dispatcher();

    bg2e::app::CommandResult submit(std::unique_ptr<bg2e::app::Command> command,
        Boundary boundary, Effects effects, std::function<bool()> valid, Completion completion = {},
        bg2e::app::CommandOrigin origin = bg2e::app::CommandOrigin::Editor);
    // CPU-only supervised interactions begin at the input/graphics boundary.
    // The transaction owns validation, effects and recovery; it retains no native
    // resources. Pending commands prevent admission. History blocks execution of
    // other commands until this interaction is settled.
    bg2e::app::CommandResult beginCpuInteraction(std::unique_ptr<bg2e::app::CommandTransaction> command);

    void setSelectionHistoryEnabled(bool enabled);
    bool selectionHistoryEnabled() const;

    void undo();
    void redo();
    void pump();
    size_t pending() const;
    bg2e::app::CommandResult settle(bool commit);
    bg2e::app::CommandResult shutdown();
    // Call only after render extraction/submissions no longer use retired data.
    bool retiredResourcesPending() const;
    void releaseRetired();

    bg2e::app::CommandHistory& history();
    const bg2e::app::CommandHistory& history() const;

  private:
    bool _selectionHistory = false;
    struct Retirement;
    std::shared_ptr<Retirement> _retirement;
    std::unique_ptr<bg2e::app::CommandHistory> _history;
    std::unique_ptr<bg2e::app::CommandQueue> _queue;
    std::unordered_map<std::string, Boundary> _boundaries;
    Scheduler _scheduler;
    EffectHandler _effects;
    Completion _completion;
};

}
