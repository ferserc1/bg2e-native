#pragma once

#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/reflection/Builder.hpp>

namespace bg2e::draw::upscaling_probe {
// Application-defined implementation: composition, no closed engine enum and no
// inheritance from an engine-selected final pass. Toggle bypass via the catalog.
class ApplicationUpscaler : public Upscaler {
  public:
    std::string identifier() const override
    {
        return "application.resampler.v1";
    }
    std::string label() const override
    {
        return "Application resampler";
    }
    UpscalerRequirements requirements() const override
    {
        return {};
    }
    void initialize(Engine& engine, const UpscalerConfiguration& configuration) override
    {
        _implementation.initialize(engine, configuration);
    }
    void resize(const UpscalerConfiguration& configuration) override
    {
        _implementation.resize(configuration);
    }
    bool available() const override
    {
        return _implementation.available();
    }
    std::string status() const override
    {
        return "Application implementation";
    }
    bool record(const UpscalerInput& input) override
    {
        if (!_enabled)
        {
            return false;
        }
        ++_calls;
        return _implementation.record(input);
    }
    void reset() override
    {
        _implementation.reset();
    }
    void cleanup() override
    {
        _implementation.cleanup();
    }
    bool enabled() const
    {
        return _enabled;
    }
    void setEnabled(bool value)
    {
        _enabled = value;
    }
    uint32_t calls() const
    {
        return _calls;
    }
    reflection::TypeInfo settingsType() const override
    {
        reflection::TypeInfoBuilder<ApplicationUpscaler> type(identifier());
        type.property("enabled", &ApplicationUpscaler::enabled, &ApplicationUpscaler::setEnabled);
        type.property("calls", &ApplicationUpscaler::calls);
        return type.build();
    }
  private:
    ResamplingUpscaler _implementation;
    bool _enabled = true;
    uint32_t _calls = 0;
};
}
