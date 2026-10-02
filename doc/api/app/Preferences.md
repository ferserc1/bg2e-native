# Preferences and PreferencesStore

**Headers:** `<bg2e/app/Preferences.hpp>`, `<bg2e/app/PreferencesStore.hpp>`  
**Namespace:** `bg2e::app`

`Preferences` stores typed values in an application-specific JSON file.
`PreferencesStore` is the process-wide registry of global and named preference
contexts.

## Scoped preferences

```cpp
auto& prefs = PreferencesStore::instance().preferences("editor");

bool enabled = prefs.get("enabled", true);
double rate = prefs.get("rate", 1.0);

prefs.set("enabled", false);
prefs.set("rate", 0.5);
```

`preferences()` uses the global context. `preferences("name")` uses a named
file such as `preferences_name.json`. Instances load on creation and save dirty
data on destruction. `saveAll()` explicitly saves every registered context.

## Supported values

The typed API supports booleans, signed and unsigned integer widths, `float`,
`double`, strings, fixed float arrays, GLM vectors/matrices, and `base::Color`.
`get(key, fallback)` returns the fallback when the key is absent or has an
incompatible JSON type.

## Persisting background scheduling

The limiter itself is runtime state. Applications that want persistence can
store it explicitly:

```cpp
auto* loop = MainLoop::current();
auto& prefs = PreferencesStore::instance().preferences("app");

double fps = prefs.get("backgroundMaxFrameRate", 1.0);
if (std::isfinite(fps) && fps > 0.0) {
    loop->setBackgroundMaxFrameRate(fps);
}

loop->setBackgroundFrameRateLimitEnabled(
    prefs.get("backgroundFrameRateLimitEnabled", false));
```

Validate persisted frame rates before passing them to
`setBackgroundMaxFrameRate()`, because invalid values are rejected.

