#include <bg2e/draw/SettingsCatalog.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>
#include <filesystem>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <set>
#include <stdexcept>

namespace bg2e::draw {
namespace {
// Unique across catalogs too: an import cannot accidentally address another instance.
std::atomic<uint64_t> nextGeneration{0};
template <typename V>
void requireType(const std::any& value)
{
    // The pointer overload checks the stored type without copying the value.
    // A stored false, empty string or empty path still has a non-null pointer.
    if (std::any_cast<V>(&value) == nullptr)
    {
        throw std::bad_any_cast();
    }
}
void number(const reflection::PropertyInfo& p, double value)
{
    if (!std::isfinite(value) || (p.metadata.min && value < *p.metadata.min) || (p.metadata.max && value > *p.metadata.max))
    {
        throw std::invalid_argument("Nonfinite or out-of-range setting: " + p.name);
    }
}
void identity(const std::string& id)
{
    if (id.empty() || id.find_first_of("\r\n\t") != std::string::npos)
    {
        throw std::invalid_argument("Settings identities must be nonempty and contain no control whitespace");
    }
}
}
bool SettingsResult::valid() const
{
    return std::none_of(issues.begin(), issues.end(), [](const auto& issue)
    {
        return issue.error;
    });
}
void SettingsCatalog::reset(std::string rendererId)
{
    _generation = ++nextGeneration;
    _targets.clear();
    _renderer = std::move(rendererId);
}
void SettingsCatalog::add(SettingsTarget target)
{
    identity(target.id);
    identity(target.type.typeName);
    if (!target.instance || _renderer.empty() || std::any_of(_targets.begin(), _targets.end(), [&](const auto& other)
        {
            return other.id == target.id;
        }))
    {
        throw std::invalid_argument("Settings target requires an instance, renderer identity and unique component identity");
    }
    std::set<std::string> names;
    for (const auto& property : target.type.properties)
    {
        identity(property.name);
        if (!property.getter || !names.insert(property.name).second)
        {
            throw std::invalid_argument("Scalar settings properties need unique names and getters");
        }
    }
    _targets.push_back(std::move(target));
    _generation = ++nextGeneration;
}
uint64_t SettingsCatalog::generation() const
{
    return _generation;
}
const std::string& SettingsCatalog::rendererId() const
{
    return _renderer;
}
const SettingsTarget& SettingsCatalog::target(const SettingHandle& handle) const
{
    if (handle.generation != _generation)
    {
        throw std::logic_error("Stale settings catalog generation");
    }
    const auto found = std::find_if(_targets.begin(), _targets.end(), [&](const auto& t)
    {
        return t.id == handle.component;
    });
    if (found == _targets.end() || !found->type.property(handle.property))
    {
        throw std::out_of_range("Unknown settings component/property");
    }
    return *found;
}
SettingHandle SettingsCatalog::handle(const std::string& component, const std::string& property) const
{
    SettingHandle result{_generation, component, property};
    target(result);
    return result;
}
SettingDescriptor SettingsCatalog::describe(const SettingHandle& handle) const
{
    const auto& t = target(handle);
    return {handle, t.type.typeName, *t.type.property(handle.property), !t.available || t.available(handle.property)};
}
std::vector<SettingDescriptor> SettingsCatalog::properties(bool includeUnavailable) const
{
    std::vector<SettingDescriptor> result;
    for (const auto& t : _targets)
    {
        for (const auto& p : t.type.properties)
        {
            auto descriptor = describe({_generation, t.id, p.name});
            if (descriptor.available || includeUnavailable)
            {
                result.push_back(std::move(descriptor));
            }
        }
    }
    return result;
}
std::any SettingsCatalog::read(const SettingHandle& handle) const
{
    const auto& t = target(handle);
    const auto& p = *t.type.property(handle.property);
    const auto value = p.getter(t.instance);
    validateValue(p, value);
    return value;
}
void SettingsCatalog::validateValue(const reflection::PropertyInfo& p, const std::any& value)
{
    using T = reflection::PropertyType;
    switch (p.type)
    {
    case T::Bool:
        requireType<bool>(value);
        break;
    case T::Int:
        number(p, std::any_cast<int32_t>(value));
        break;
    case T::UInt:
        number(p, std::any_cast<uint32_t>(value));
        break;
    case T::Float:
        number(p, std::any_cast<float>(value));
        break;
    case T::Double:
        number(p, std::any_cast<double>(value));
        break;
    case T::Enum:
    {
        const auto v = std::any_cast<int64_t>(value);
        if (p.metadata.enumOptions.empty() || std::none_of(p.metadata.enumOptions.begin(), p.metadata.enumOptions.end(), [&](const auto& option)
            {
                return option.second == v;
            }))
        {
            throw std::invalid_argument("Unknown enum identity for setting: " + p.name);
        }
        break;
    }
    case T::String:
        requireType<std::string>(value);
        break;
    case T::Path:
        requireType<std::filesystem::path>(value);
        break;
    case T::Resource:
        if (p.metadata.resourceValueType == T::String)
        {
            requireType<std::string>(value);
        }
        else if (p.metadata.resourceValueType == T::Path)
        {
            requireType<std::filesystem::path>(value);
        }
        else
        {
            throw std::invalid_argument("Unsupported resource setting storage type");
        }
        break;
    case T::Vec2:
    {
        const auto v = std::any_cast<glm::vec2>(value);
        for (int i = 0; i < 2; ++i)
        {
            number(p, v[i]);
        }
        break;
    }
    case T::Vec3:
    {
        const auto v = std::any_cast<glm::vec3>(value);
        for (int i = 0; i < 3; ++i)
        {
            number(p, v[i]);
        }
        break;
    }
    case T::Vec4:
    {
        const auto v = std::any_cast<glm::vec4>(value);
        for (int i = 0; i < 4; ++i)
        {
            number(p, v[i]);
        }
        break;
    }
    case T::Color:
    {
        const auto v = std::any_cast<base::Color>(value);
        for (float c : {v.r, v.g, v.b, v.a})
        {
            number(p, c);
        }
        break;
    }
    case T::Mat4:
    {
        const auto v = std::any_cast<glm::mat4>(value);
        for (int c = 0; c < 4; ++c)
        {
            for (int row = 0; row < 4; ++row)
            {
                number(p, v[c][row]);
            }
        }
        break;
    }
    default:
        throw std::invalid_argument("Settings codec supports scalar properties, not owned object replacement");
    }
}
SettingsResult SettingsCatalog::validate(const std::vector<SettingChange>& changes) const
{
    SettingsResult result;
    std::set<std::pair<std::string, std::string>> seen;
    std::set<std::string> components;
    for (const auto& change : changes)
    {
        try
        {
            const auto d = describe(change.handle);
            if (!d.available || d.property.isReadOnly())
            {
                throw std::invalid_argument("Setting is unavailable or read-only");
            }
            if (!seen.emplace(change.handle.component, change.handle.property).second)
            {
                throw std::invalid_argument("Duplicate setting assignment");
            }
            validateValue(d.property, change.value);
            components.insert(change.handle.component);
        }
        catch (const std::exception& error)
        {
            result.issues.push_back({"invalid_value", change.handle.component + "/" + change.handle.property, error.what()});
        }
    }
    if (!result.valid())
    {
        return result;
    }
    for (const auto& id : components)
    {
        const auto found = std::find_if(_targets.begin(), _targets.end(), [&](const auto& target)
        {
            return target.id == id;
        });
        if (!found->validate)
        {
            continue;
        }
        try
        {
            std::map<std::string, std::any> values;
            for (const auto& p : found->type.properties)
            {
                values[p.name] = p.getter(found->instance);
            }
            for (const auto& change : changes)
            {
                if (change.handle.component == id)
                {
                    values[change.handle.property] = change.value;
                }
            }
            found->validate(values);
        }
        catch (const std::exception& error)
        {
            result.issues.push_back({"invalid_combination", id, error.what()});
        }
    }
    return result;
}
SettingsResult SettingsCatalog::apply(const std::vector<SettingChange>& changes)
{
    auto result = validate(changes);
    if (!result.valid())
    {
        return result;
    }
    std::vector<std::any> previous;
    try
    {
        for (const auto& change : changes)
        {
            previous.push_back(read(change.handle));
        }
    }
    catch (const std::exception& error)
    {
        result.issues.push_back({"getter_failed", "", error.what()});
        return result;
    }
    size_t attempted = 0;
    try
    {
        for (const auto& change : changes)
        {
            const auto& t = target(change.handle);
            ++attempted;
            t.type.property(change.handle.property)->setter(t.instance, change.value);
        }
    }
    catch (const std::exception& error)
    {
        result.issues.push_back({"setter_failed", "", error.what()});
        while (attempted > 0)
        {
            --attempted;
            try
            {
                const auto& t = target(changes[attempted].handle);
                t.type.property(changes[attempted].handle.property)->setter(t.instance, previous[attempted]);
            }
            catch (const std::exception& rollback)
            {
                result.issues.push_back({"rollback_failed", changes[attempted].handle.component, rollback.what()});
            }
        }
        return result;
    }
    std::set<std::string> notified;
    for (const auto& change : changes)
    {
        const auto& t = target(change.handle);
        if (t.changed && notified.insert(t.id).second)
        {
            try
            {
                t.changed();
            }
            catch (const std::exception& error)
            {
                result.issues.push_back({"post_commit_failed", t.id, error.what()});
            }
        }
    }
    result.applied = true;
    return result;
}
}
