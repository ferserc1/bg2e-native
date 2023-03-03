#ifndef bg2e_base_environment_hpp
#define bg2e_base_environment_hpp

#include <bg2e/export.hpp>

#include <bg2e/tools/Json.hpp>

#include <string>
#include <memory>

namespace bg2e {
namespace base {

class BG2E_EXPORT Environment {
public:
    Environment();
    
    Environment(Environment&&) = delete;
    Environment& operator=(Environment&&) = delete;

    inline const std::string& equirectangularTexture() const { return _equirectangularTexture; }
    inline float irradianceIntensity() const { return _irradianceIntensity; }
    inline bool showSkybox() const { return _showSkybox; }
    inline uint32_t cubemapSize() const { return _cubemapSize; }
    inline uint32_t irradianceMapSize() const { return _irradianceMapSize; }
    inline uint32_t specularMapSize() const { return _specularMapSize; }
    inline uint32_t specularMapL2Size() const { return _specularMapL2Size; }

    inline void setEquirectangularTexture(const std::string& p) { _equirectangularTexture = p; _dirty = true; }
    inline void setIrradianceIntensity(float p) { _irradianceIntensity = p; _dirty = true; }
    inline void setShowSkybox(bool p) { _showSkybox = p; _dirty = true; }
    inline void setCubemapSize(uint32_t p) { _cubemapSize = p; _dirty = true; }
    inline void setIrradianceMapSize(uint32_t p) { _irradianceMapSize = p; _dirty = true; }
    inline void setSpecularMapSize(uint32_t p) { _specularMapSize = p; _dirty = true; }
    inline void setSpecularMapL2Size(uint32_t p) { _specularMapL2Size = p; _dirty = true; }

    std::shared_ptr<Environment> clone()
    {
        auto copy = std::make_shared<Environment>();
        *copy = *this;
        return copy;
    }
    
    Environment& operator=(const Environment& other)
    {
        _equirectangularTexture = other._equirectangularTexture;
        _irradianceIntensity = other._irradianceIntensity;
        _showSkybox = other._showSkybox;
        _cubemapSize = other._cubemapSize;
        _irradianceMapSize = other._irradianceMapSize;
        _specularMapSize = other._specularMapSize;
        _specularMapL2Size = other._specularMapL2Size;
        return *this;
    }
    
    void deserialize(const std::shared_ptr<tools::JsonNode>&);

    void serialize(tools::JsonObject&);

protected:
    std::string _equirectangularTexture = "";
    float _irradianceIntensity = 1.0f;
    bool _showSkybox = true;
    uint32_t _cubemapSize = 512;
    uint32_t _irradianceMapSize = 32;
    uint32_t _specularMapSize = 32;
    uint32_t _specularMapL2Size = 32;
    
    bool _dirty = true;
};

}
}
#endif
