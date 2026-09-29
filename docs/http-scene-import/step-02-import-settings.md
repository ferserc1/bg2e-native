# Step 02 — ImportSettings (preferences wrapper)

## Goal

Persist the two service settings (`port`, `enabled`) with the existing
preferences system, following the style of
`bg2e::render::RenderSettingsPreferences` (a typed wrapper class holding a
`bg2e::app::Preferences` member), but living at **app level**.

`Preferences("import")` maps to `preferences_import.json`; `~Preferences()`
already calls `save()`, and the class offers `load()`, `get<T>(key, default)`,
`set(key, value)`, `save()`.

## Files to create

- `apps/bg2e_composer/src/ImportSettings.hpp`
- `apps/bg2e_composer/src/ImportSettings.cpp`

(Flat in `src/`: `bundle_app()` only globs `${CMAKE_CURRENT_LIST_DIR}/src/*.cpp`.)

## Interface

```cpp
#pragma once

#include <bg2e/app/Preferences.hpp>
#include <cstdint>
#include <string>

class ImportSettings {
public:
    static constexpr uint32_t DefaultPort = 8643;
    static constexpr uint32_t MinPort = 1024;    // below: privileged
    static constexpr uint32_t MaxPort = 49151;   // above: ephemeral/dynamic range

    ImportSettings();   // constructs Preferences("import"), does NOT load

    void load();
    void save();

    uint32_t port() const;
    void setPort(uint32_t port);

    bool serviceEnabled() const;        // default: true
    void setServiceEnabled(bool enabled);

    // Numeric string typed in the UI -> validated port.
    // Returns false when empty, non-numeric, or outside [MinPort, MaxPort].
    static bool parsePort(const std::string& text, uint32_t& outPort);

private:
    bg2e::app::Preferences _prefs { "import" };
};
```

## Implementation sketch

```cpp
#include "ImportSettings.hpp"

void ImportSettings::load() { _prefs.load(); }
void ImportSettings::save() { _prefs.save(); }

uint32_t ImportSettings::port() const
{
    return _prefs.get<uint32_t>("port", DefaultPort);
}

void ImportSettings::setPort(uint32_t p)
{
    _prefs.set<uint32_t>("port", p);
}

bool ImportSettings::serviceEnabled() const
{
    return _prefs.get<bool>("serviceEnabled", true);
}

void ImportSettings::setServiceEnabled(bool enabled)
{
    _prefs.set<bool>("serviceEnabled", enabled);
}

bool ImportSettings::parsePort(const std::string& text, uint32_t& outPort)
{
    if (text.empty() ||
        !std::all_of(text.begin(), text.end(),
                     [](unsigned char c){ return std::isdigit(c); }))
    {
        return false;
    }
    try
    {
        unsigned long v = std::stoul(text);
        if (v < MinPort || v > MaxPort) return false;
        outPort = static_cast<uint32_t>(v);
        return true;
    }
    catch (...)
    {
        return false;   // out_of_range on absurdly long input
    }
}
```

## Design notes

- Port range **1024–49151** (approved): excludes privileged ports and the
  IANA ephemeral range (which collides with OS-assigned client ports).
- Save policy: **save-on-change** from the settings window (deterministic,
  no dependency on the 60 s timer used by render settings). `~Preferences()`
  still saves on destruction as a safety net.
- `load()` is called once from `AppDelegate::initWorkspace()` (step 06)
  before any UI or server access.

## Verification

- Launch Composer, change port / toggle service, quit, relaunch: values
  restored from `preferences_import.json` (path resolved by
  `Preferences::initFilePath`).
- Unit-style check of `parsePort`: `"8643"` ok; `"80"`, `"50000"`, `"abc"`,
  `""` rejected.
