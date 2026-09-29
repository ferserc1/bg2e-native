# Step 01 — UI wrapper: disabled text input

## Goal

The Import Settings window needs a port text field that is **disabled while the
service is running**. The engine wrapper `bg2e::ui::Value::text()` has no
`disabled` support. Per project rules, add it **only** to the function this
feature uses — no wide refactor of `bg2e::ui`. ImGui must never be exposed in
public headers and never called from app code.

This is the only engine-side change in the whole feature (pre-authorized).

## Files to modify

- `lib/include/bg2e/ui/Value.hpp`
- `lib/src/bg2e/ui/Value.cpp`
- `doc/api/ui/Value.md`

## Interface change

`Value.hpp` — extend only the labeled overload (default argument keeps every
existing call site source-compatible):

```cpp
// Non-numeric value editors (text fields, color picker and combo boxes)
class BG2E_API Value {
public:
    static bool text(
        const std::string& label,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false,
        bool disabled = false        // NEW
    );
    // ... rest unchanged
};
```

The id-only overload with `readOnly` (`Value::text(id, value, readOnly, ...)`)
is **not** touched — readOnly already covers its use cases and the new window
uses the labeled overload.

## Implementation

`Value.cpp` — mirror the exact pattern already used in
`lib/src/bg2e/ui/Button.cpp` (`button`, `checkBox`, `radioButton`):

```cpp
bool Value::text(const std::string& label, std::string& value, int maxLength,
                 bool sameLine, bool disabled)
{
    char * stringValue = new char[maxLength];
    strcpy(stringValue, value.c_str());
    if (sameLine)
    {
        ImGui::SameLine();
    }
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    bool changed = ImGui::InputText(label.c_str(), stringValue, maxLength);
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    if (changed)
    {
        value = stringValue;
        delete [] stringValue;
        return true;
    }
    delete [] stringValue;
    return false;
}
```

Notes:

- `BeginDisabled/EndDisabled` wraps **only** the `InputText` call (SameLine
  stays outside, exactly like `Button.cpp` keeps layout calls outside).
- Keep the existing buffer semantics (fixed `char[maxLength]`, write-back only
  on change).

## Documentation update

`doc/api/ui/Value.md`:

- Update the class signature block with the new parameter.
- Add to the "Text fields" section:

```markdown
- `text()` accepts `disabled = true` to render the field greyed out and
  non-editable (ImGui `BeginDisabled/EndDisabled`), e.g. a setting that can
  only be changed while a service is stopped.
```

## Verification

- Engine and apps rebuild; existing call sites (e.g. `UISettingsWindow`,
  `DrawableEditor`) compile unchanged thanks to the default argument.
- Visual check happens in step 05 (settings window): the port field greys out
  while the service is running.
