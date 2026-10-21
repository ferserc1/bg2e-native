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

#include <bg2e/bake/LightmapOutputWriter.hpp>
#include <bg2e/db/file.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/db/texture_archive.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/utils/utils.hpp>
#include <algorithm>
#include <cctype>
#include <type_traits>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/Chain.hpp>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>
#include <typeinfo>

namespace bg2e::bake {
namespace {
using Path = std::filesystem::path;
using namespace json;

Path resolvedPath(const Path& p)
{
    if (p.empty())
    {
        throw std::invalid_argument("Bake output: empty path");
    }
    return std::filesystem::absolute(p).lexically_normal();
}

std::string key(const Path& p)
{
    auto s = std::filesystem::weakly_canonical(p).generic_string();
#ifdef _WIN32
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
    {
        return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    });
#endif
    return s;
}

bool inside(const std::string& child, const std::string& parent)
{
    return child == parent || (child.size() > parent.size() && child.starts_with(parent) &&
        (parent.back() == '/' || child[parent.size()] == '/'));
}

uint64_t hashFile(const Path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f)
    {
        throw std::runtime_error("Bake output: cannot read input " + p.string());
    }
    uint64_t h = 14695981039346656037ull;
    char buffer[65536];
    while (f.read(buffer, sizeof(buffer)) || f.gcount())
    {
        for (std::streamsize i = 0; i < f.gcount(); ++i)
        {
            h = (h ^ static_cast<unsigned char>(buffer[i])) * 1099511628211ull;
        }
    }
    if (!f.eof())
    {
        throw std::runtime_error("Bake output: failed reading input " + p.string());
    }
    return h;
}

void writeJson(const Path& p, const std::shared_ptr<JsonNode>& value)
{
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.exceptions(std::ios::badbit | std::ios::failbit);
    f << value->toString() << '\n';
    f.close();
}

template <typename T> void append(std::string& s, const T& v)
{
    static_assert(std::is_trivially_copyable_v<T>);
    s.append(reinterpret_cast<const char*>(&v), sizeof(v));
}

template <typename T> void appendVectorFingerprint(std::string& s, const std::vector<T>& values)
{
    static_assert(std::is_trivially_copyable_v<T>);
    append(s, values.size());
    uint64_t h = 14695981039346656037ull;
    const auto* bytes = reinterpret_cast<const unsigned char*>(values.data());
    for (size_t i = 0; i < values.size() * sizeof(T); ++i)
    {
        h = (h ^ bytes[i]) * 1099511628211ull;
    }
    append(s, h);
}

void appendGeometry(std::string& s, const draw::Drawable::Mesh& mesh)
{
    appendVectorFingerprint(s, mesh.vertices);
    appendVectorFingerprint(s, mesh.indices);
    appendVectorFingerprint(s, mesh.submeshes);
}

void appendText(std::string& s, const std::string& text)
{
    append(s, text.size());
    s += text;
}

// Retain the baseline queue's owners in State so pointer identities cannot be recycled.
std::string sceneSignature(scene::Node* root, draw::Engine& engine, draw::RenderQueue& queue)
{
    draw::RenderQueueVisitor visitor;
    visitor.enqueue(root, engine, queue);
    std::string s;
    std::set<const draw::Drawable::Mesh*> geometrySeen;
    const auto graph = [&](const auto& self, scene::Node* node) -> void
    {
        append(s, node);
        appendText(s, node->name());
        append(s, node->enabled());
        append(s, node->steady());
        append(s, node->children().size());
        append(s, node->orderedComponents().size());
        for (const auto& c : node->orderedComponents())
        {
            append(s, c.get());
            append(s, c->ignoreSerialization());
            if (auto component = dynamic_cast<scene::DrawableComponent*>(c.get()))
            {
                if (const auto d = component->drawable())
                {
                    append(s, d.get());
                    append(s, d->mesh().get());
                    if (d->mesh() && geometrySeen.insert(d->mesh().get()).second)
                    {
                        appendGeometry(s, *d->mesh());
                    }
                    append(s, d->submeshesCount());
                    for (uint32_t i = 0; i < d->submeshesCount(); ++i)
                    {
                        appendText(s, d->submeshName(i));
                        appendText(s, d->submeshGroupName(i));
                        append(s, d->submeshVisibility(i));
                        append(s, d->submeshTransform(i));
                    }
                }
            }
            auto value = c->serializeReferences({});
            appendText(s, value ? value->toString() : "null");
        }
        for (const auto& child : node->children())
        {
            self(self, child.get());
        }
    };
    graph(graph, root);
    append(s, queue.items().size());
    for (const auto& i : queue.items())
    {
        append(s, i.drawMesh.get());
        append(s, i.drawMesh->vertexBuffer());
        append(s, i.drawMesh->indexBuffer());
        if (geometrySeen.insert(&i.drawMesh->meshData()).second)
        {
            appendGeometry(s, i.drawMesh->meshData());
        }
        append(s, i.drawMaterial.get());
        append(s, i.submeshIndex);
        append(s, i.worldMatrix);
        append(s, i.materialData);
        append(s, i.materialRevisions.values);
        append(s, i.materialRevisions.bindings);
        append(s, i.materialRevisions.classification);
        append(s, i.tracingMesh.get());
        append(s, i.tracingMesh ? i.tracingMesh->buildGeneration() : 0);
        append(s, i.rayTracingEnabled);
        for (size_t t = 0; t < i.textures.size(); ++t)
        {
            append(s, i.textures[t].get());
            append(s, i.textureImages[t]);
            append(s, i.textureSamplers[t]);
            append(s, i.textureRevisions[t]);
            append(s, i.imageGenerations[t]);
        }
    }
    append(s, queue.lights());
    const auto environment = queue.environment();
    append(s, bool(environment));
    if (environment)
    {
        appendText(s, environment->environmentImage());
        append(s, environment->skyboxBlur());
        append(s, environment->skyboxVisible());
        for (const auto& color : {environment->skyHorizonColor(), environment->skyZenithColor(), environment->groundColor()})
        {
            append(s, color.r);
            append(s, color.g);
            append(s, color.b);
            append(s, color.a);
        }
    }
    return s;
}

bool sameGeometry(const draw::Drawable::Mesh& a, const draw::Drawable::Mesh& b)
{
    return a.indices == b.indices && a.vertices.size() == b.vertices.size() &&
        a.submeshes.size() == b.submeshes.size() &&
        std::equal(a.vertices.begin(), a.vertices.end(), b.vertices.begin(), [](const auto& x, const auto& y)
        {
            return x.position == y.position && x.normal == y.normal && x.texCoord0 == y.texCoord0 &&
                x.texCoord1 == y.texCoord1 && x.tangent == y.tangent;
        }) && std::equal(a.submeshes.begin(), a.submeshes.end(), b.submeshes.begin(), [](const auto& x, const auto& y)
        {
            return x.firstIndex == y.firstIndex && x.indexCount == y.indexCount;
        });
}

bool knownComponent(const scene::Component& c)
{
    // Exact RTTI intentionally excludes subclasses with undeclared dependencies.
    const auto& t = typeid(c);
    return t == typeid(scene::DrawableComponent) || t == typeid(scene::TransformComponent) ||
        t == typeid(scene::CameraComponent) || t == typeid(scene::LightComponent) ||
        t == typeid(scene::EnvironmentComponent) || t == typeid(scene::InputChainJointComponent) ||
        t == typeid(scene::OutputChainJointComponent) || t == typeid(scene::OrbitCameraComponent) ||
        t == typeid(scene::PolarTransformControllerComponent) ||
        t == typeid(scene::FixedScaleTransformControllerComponent) || t == typeid(scene::ChainComponent);
}

std::string stem(const std::string& identity, size_t index)
{
    std::string result = "target-" + std::to_string(index) + "-";
    for (unsigned char c : identity)
    {
        if (result.size() >= 100)
        {
            break;
        }
        result += (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ? static_cast<char>(c) : '_';
    }
    return result;
}
}

struct LightmapOutputWriter::State {
    struct Input {
        Path path;
        std::string canonical;
        bool directory;
        uintmax_t size = 0;
        std::filesystem::file_time_type time;
        uint64_t hash = 0;
    };
    draw::Engine* engine = nullptr;
    LightmapSettings settings;
    Path directory, manifest, stage, generation, lock;
    std::string directoryKey, signature;
    db::ImageFormat format;
    bool overwrite = false, ready = false, committed = false, ownsStage = false, ownsLock = false;
    std::vector<Target> targets;
    std::vector<OutputPaths> outputs;
    std::vector<bool> written;
    std::vector<Input> inputs;
    std::shared_ptr<scene::Node> root;
    draw::RenderQueue baseline;

    void checkDestination() const
    {
        if (key(directory) != directoryKey)
        {
            throw std::runtime_error("Bake output: destination changed after preflight");
        }
        const auto status = std::filesystem::symlink_status(manifest);
        if (status.type() != std::filesystem::file_type::not_found &&
            (std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status) || !overwrite))
        {
            throw std::runtime_error("Bake output: manifest exists or is not a replaceable regular file");
        }
        for (const auto& input : inputs)
        {
            const auto outputKey = key(manifest);
            if (inside(outputKey, input.canonical) ||
                (std::filesystem::exists(manifest) && !input.directory &&
                 std::filesystem::equivalent(manifest, input.path)))
            {
                throw std::runtime_error("Bake output: manifest aliases a protected input");
            }
        }
    }

    void beginStage()
    {
        if (ownsStage)
        {
            return;
        }
        checkDestination();
        std::filesystem::create_directories(directory);
        if (!std::filesystem::create_directory(lock))
        {
            throw std::runtime_error("Bake output: another writer owns .lightmaps.lock");
        }
        ownsLock = true;
        checkDestination();
        if (!std::filesystem::create_directory(stage))
        {
            throw std::runtime_error("Bake output: staging name collision");
        }
        ownsStage = true;
    }
};

LightmapOutputWriter::LightmapOutputWriter(draw::Engine& engine, Path outputDirectory,
    LightmapSettings settings, db::ImageFormat byteFormat) : _state(std::make_unique<State>())
{
    settings.validate();
    _state->engine = &engine;
    _state->settings = settings;
    _state->format = byteFormat;
    if (db::canonicalImageExtension(byteFormat).empty())
    {
        throw std::invalid_argument("Bake output: unsupported byte image format");
    }
    _state->directory = resolvedPath(outputDirectory);
    _state->directoryKey = key(_state->directory);
    _state->manifest = _state->directory / "lightmaps.json";
    _state->lock = _state->directory / ".lightmaps.lock";
    const auto id = utils::uniqueId();
    _state->stage = _state->directory / (".lightmaps-stage-" + id);
    _state->generation = _state->directory / ("lightmaps-" + id);
}

LightmapOutputWriter::~LightmapOutputWriter()
{
    std::error_code error;
    if (_state->ownsStage)
    {
        std::filesystem::remove_all(_state->stage, error);
    }
    if (_state->ownsLock)
    {
        std::filesystem::remove(_state->lock, error);
    }
}

void LightmapOutputWriter::setOverwriteExisting(bool overwrite)
{
    if (_state->ready)
    {
        throw std::logic_error("Bake output: overwrite policy is frozen");
    }
    _state->overwrite = overwrite;
}

std::vector<LightmapOutputWriter::OutputPaths> LightmapOutputWriter::preflight(
    const std::vector<Target>& targets, bool writeModelCopies,
    const std::vector<Path>& protectedInputs, BakeDependencyResolver resolver)
{
    auto& s = *_state;
    if (s.ready || targets.empty())
    {
        throw std::logic_error("Bake output: preflight requires a new writer and nonempty batch");
    }
    // Failure leaves no partially populated plan reusable for a second attempt.
    s.inputs.clear();
    s.outputs.clear();
    std::set<std::string> paths, identities;
    const auto protect = [&](const Path& p)
    {
        auto path = resolvedPath(p);
        const auto canonical = key(path);
        if (!paths.insert(canonical).second)
        {
            return;
        }
        const bool directory = std::filesystem::is_directory(path);
        if (!directory && !std::filesystem::is_regular_file(path))
        {
            throw std::runtime_error("Bake output: missing protected input " + path.string());
        }
        State::Input input{path, canonical, directory};
        if (!directory)
        {
            input.size = std::filesystem::file_size(path);
            input.time = std::filesystem::last_write_time(path);
            input.hash = hashFile(path);
        }
        s.inputs.push_back(std::move(input));
    };
    for (const auto& p : protectedInputs)
    {
        protect(p);
    }
    if (!targets.front().node)
    {
        throw std::invalid_argument("Bake output: null target");
    }
    s.root = targets.front().node->sceneRoot()->shared_from_this();
    const auto graph = [&](const auto& self, scene::Node* node) -> void
    {
        for (const auto& c : node->orderedComponents())
        {
            if (!knownComponent(*c))
            {
                const auto dependencies = resolver ? resolver(*c) : std::nullopt;
                if (!dependencies)
                {
                    throw std::invalid_argument("Bake output: custom component needs a complete dependency resolver");
                }
                for (const auto& p : *dependencies)
                {
                    if (std::filesystem::is_directory(p))
                    {
                        throw std::invalid_argument("Bake output: custom dependencies must enumerate files, not directories");
                    }
                    protect(p);
                }
            }
            if (auto dc = dynamic_cast<scene::DrawableComponent*>(c.get()))
            {
                if (!dc->resourcePath().empty())
                {
                    protect(dc->resourcePath());
                    auto ext = dc->resourcePath().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char v)
                    {
                        return static_cast<char>(std::tolower(v));
                    });
                    if (ext == ".obj")
                    {
                        for (const auto& p : db::readObj(dc->resourcePath())->sourcePaths)
                        {
                            protect(p);
                        }
                    }
                }
                if (auto d = dc->drawable())
                {
                    for (uint32_t i = 0; i < d->submeshesCount(); ++i)
                    {
                        const auto& m = d->material(i);
                        for (const auto& t : {m.albedoTexture(), m.metalnessTexture(), m.roughnessTexture(),
                             m.normalTexture(), m.aoTexture(), m.lightEmissionTexture()})
                        {
                            if (t && !t->imageFilePath().empty())
                            {
                                protect(t->imageFilePath());
                            }
                        }
                    }
                }
            }
            if (auto env = dynamic_cast<scene::EnvironmentComponent*>(c.get()); env && !env->environmentImage().empty())
            {
                protect(env->environmentImage());
            }
        }
        for (const auto& child : node->children())
        {
            self(self, child.get());
        }
    };
    graph(graph, s.root.get());
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const auto& t = targets[i];
        if (t.identity.empty() || !identities.insert(t.identity).second || !t.node ||
            t.node->sceneRoot() != s.root.get() || !t.node->drawable() || !t.node->drawable()->drawable())
        {
            throw std::invalid_argument("Bake output: duplicate identity or invalid scene target");
        }
        const auto drawable = t.node->drawable()->drawable();
        if (!drawable->isLoaded() || !drawable->mesh() || !drawable->drawMesh() ||
            !sameGeometry(*drawable->mesh(), drawable->drawMesh()->meshData()))
        {
            throw std::invalid_argument("Bake output: target CPU geometry differs from its uploaded geometry; reload before baking");
        }
        protect(t.inputModelPath);
        // Models loaded directly without a resourcePath still need their MTLs protected.
        auto ext = t.inputModelPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        if (ext == ".obj")
        {
            for (const auto& p : db::readObj(t.inputModelPath)->sourcePaths)
            {
                protect(p);
            }
        }
        const auto name = stem(t.identity, i);
        OutputPaths out{t.identity, s.generation / (name + (s.settings.cpuFormat() == LightmapPixelFormat::RGB32F ?
            ".bg2tex" : std::string(db::canonicalImageExtension(s.format)))),
            s.generation / (name + ".lightmap.json"), std::nullopt};
        if (writeModelCopies)
        {
            out.modelPath = s.generation / (name + "-model") / "model.bg2";
        }
        s.outputs.push_back(std::move(out));
    }
    for (const auto& input : s.inputs)
    {
        // Entire fresh generation/staging/lock namespaces must be disjoint from protected inputs.
        for (const auto& output : {s.generation, s.stage, s.lock})
        {
            if (inside(key(output), input.canonical) || inside(input.canonical, key(output)))
            {
                throw std::invalid_argument("Bake output: output namespace intersects a protected input");
            }
        }
    }
    s.checkDestination();
    for (const auto& p : {s.generation, s.stage, s.lock})
    {
        if (std::filesystem::symlink_status(p).type() != std::filesystem::file_type::not_found)
        {
            throw std::runtime_error("Bake output: reserved output namespace already exists");
        }
    }
    s.signature = sceneSignature(s.root.get(), *s.engine, s.baseline);
    s.targets = targets;
    s.written.assign(targets.size(), false);
    s.ready = true;
    return s.outputs;
}

void LightmapOutputWriter::validateSources() const
{
    const auto& s = *_state;
    if (!s.ready || s.committed)
    {
        throw std::logic_error("Bake output: no active preflight");
    }
    s.checkDestination();
    for (const auto& input : s.inputs)
    {
        if (key(input.path) != input.canonical ||
            (input.directory ? !std::filesystem::is_directory(input.path) :
             (!std::filesystem::is_regular_file(input.path) || std::filesystem::file_size(input.path) != input.size ||
              std::filesystem::last_write_time(input.path) != input.time)))
        {
            throw std::runtime_error("Bake output: protected input changed");
        }
    }
    draw::RenderQueue current;
    if (sceneSignature(s.root.get(), *s.engine, current) != s.signature)
    {
        throw std::runtime_error("Bake output: scene changed after preflight");
    }
}

void LightmapOutputWriter::write(const LightmapPixels& pixels, const std::string& identity)
{
    validateSources();
    auto& s = *_state;
    const auto it = std::find_if(s.targets.begin(), s.targets.end(), [&](const Target& t)
    {
        return t.identity == identity;
    });
    if (it == s.targets.end())
    {
        throw std::invalid_argument("Bake output: identity was not preflighted");
    }
    const auto index = static_cast<size_t>(it - s.targets.begin());
    if (s.written[index])
    {
        throw std::logic_error("Bake output: duplicate target write");
    }
    pixels.validate();
    if (pixels.width != s.settings.resolution() || pixels.height != s.settings.resolution() ||
        pixels.format != s.settings.cpuFormat())
    {
        throw std::invalid_argument("Bake output: pixels do not match frozen settings");
    }
    s.beginStage();
    const auto& out = s.outputs[index];
    const auto image = s.stage / out.imagePath.filename();
    if (pixels.format == LightmapPixelFormat::RGB8)
    {
        db::saveImage(image, std::get<std::vector<uint8_t>>(pixels.rgb).data(), pixels.width, pixels.height, 3);
        std::unique_ptr<base::Image> decoded(db::loadImage(image));
        if (!decoded || decoded->width() != pixels.width || decoded->height() != pixels.height)
        {
            throw std::runtime_error("Bake output: byte image decode verification failed");
        }
        if (s.format != db::ImageFormat::JPEG)
        {
            const auto& rgb = std::get<std::vector<uint8_t>>(pixels.rgb);
            if (!decoded->data() || decoded->channels() < 3)
            {
                throw std::runtime_error("Bake output: missing decoded RGB channels");
            }
            for (size_t i = 0; i < rgb.size() / 3; ++i)
            {
                if (std::memcmp(rgb.data() + i * 3, decoded->data() + i * decoded->channels(), 3) != 0)
                {
                    throw std::runtime_error("Bake output: lossless byte encoding changed pixels");
                }
            }
        }
    }
    else
    {
        const auto& rgb = std::get<std::vector<float>>(pixels.rgb);
        std::vector<float> rgba(rgb.size() / 3 * 4, 1.0f);
        for (size_t i = 0; i < rgb.size() / 3; ++i)
        {
            std::copy_n(rgb.data() + i * 3, 3, rgba.data() + i * 4);
        }
        db::TextureArchive archive;
        archive.description.size = {pixels.width, pixels.height};
        archive.description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
        archive.subresources.emplace_back(rgba.size() * sizeof(float));
        std::memcpy(archive.subresources[0].data(), rgba.data(), archive.subresources[0].size());
        db::writeTextureArchive(image, archive);
        if (db::readTextureArchive(image).subresources != archive.subresources)
        {
            throw std::runtime_error("Bake output: float archive verification failed");
        }
    }
    if (out.modelPath)
    {
        auto drawable = it->node->drawable()->drawable()->clone();
        auto texture = std::make_shared<base::Texture>(image);
        texture->setColorType(base::Color::TypeLinear);
        for (uint32_t i = 0; i < drawable->submeshesCount(); ++i)
        {
            auto& m = drawable->material(i);
            m.setAoTexture(texture);
            m.setAoUVSet(1);
            m.setAoScale(glm::vec2(1));
            m.setAoChannel(0);
            m.setIndirectTextureMode(s.settings.mode() == LightmapMode::RTAO ?
                base::MaterialAttributes::IndirectTextureMode::BakedAmbientOcclusion :
                base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination);
        }
        drawable->updateMaterials();
        auto model = std::make_shared<scene::Node>(it->node->name());
        model->addComponent(std::make_shared<scene::DrawableComponent>(drawable));
        for (const auto& c : it->node->orderedComponents())
        {
            if (dynamic_cast<scene::ChainJointComponent*>(c.get()))
            {
                model->addComponent(c->clone());
            }
        }
        const auto path = s.stage / out.modelPath->parent_path().filename() / out.modelPath->filename();
        if (!db::storeDrawableBg2(path, *s.engine, model.get()))
        {
            throw std::runtime_error("Bake output: complete model export failed");
        }
    }
    writeJson(s.stage / out.metadataPath.filename(), JSON(JsonObject{
        {"version", JSON(1)}, {"identity", JSON(identity)}, {"image", JSON(out.imagePath.filename().generic_string())},
        {"width", JSON(pixels.width)}, {"height", JSON(pixels.height)}, {"colorSpace", JSON("linear")},
        {"rowZero", JSON("v=0")}, {"uvSet", JSON(1)}, {"signal", JSON("indirectMultiplierRGB")},
        {"indirectTextureMode", JSON(s.settings.mode() == LightmapMode::RTAO ? "bakedAO" : "bakedGI")},
        {"cpuFormat", JSON(pixels.format == LightmapPixelFormat::RGB8 ? "RGB8" : "RGB32F")},
        {"storageFormat", JSON(pixels.format == LightmapPixelFormat::RGB8 ? "RGB8" : "RGBA32F-alpha-one")},
        {"exposureApplied", JSON(pixels.format == LightmapPixelFormat::RGB8 && s.settings.mode() == LightmapMode::RTGI)},
        {"exposureEV", JSON(s.settings.exposureEV())}, {"dilationPixels", JSON(s.settings.dilationPixels())},
        {"accumulationFrames", JSON(s.settings.accumulationFrames())}, {"samplesPerPixel", JSON(s.settings.samplesPerPixel())},
        {"aoRadius", JSON(s.settings.aoRadius())}, {"maxRayDistance", JSON(s.settings.maxRayDistance())},
        {"rayBias", JSON(s.settings.rayBias())}, {"padding", JSON("nearest-chart")},
        {"conversion", JSON(pixels.format == LightmapPixelFormat::RGB32F ? "identity" :
            s.settings.mode() == LightmapMode::RTGI ? "linear-exp2-exposure-clamp-round-unorm8" : "linear-clamp-round-unorm8")},
        {"giBounces", JSON(s.settings.giBounces())}, {"byteEncodingLossy", JSON(s.format == db::ImageFormat::JPEG && pixels.format == LightmapPixelFormat::RGB8)}
    }));
    validateSources();
    s.written[index] = true;
}

void LightmapOutputWriter::commit()
{
    validateSources();
    auto& s = *_state;
    if (!std::all_of(s.written.begin(), s.written.end(), [](bool v)
        {
            return v;
        }))
    {
        throw std::logic_error("Bake output: cannot commit incomplete batch");
    }
    for (const auto& input : s.inputs)
    {
        if (!input.directory && hashFile(input.path) != input.hash)
        {
            throw std::runtime_error("Bake output: protected input content changed");
        }
    }
    JsonList entries;
    for (const auto& out : s.outputs)
    {
        JsonObject entry{{"identity", JSON(out.identity)},
            {"image", JSON(out.imagePath.lexically_relative(s.directory).generic_string())},
            {"metadata", JSON(out.metadataPath.lexically_relative(s.directory).generic_string())}};
        if (out.modelPath)
        {
            entry.emplace("model", JSON(out.modelPath->lexically_relative(s.directory).generic_string()));
        }
        entries.push_back(JSON(std::move(entry)));
    }
    writeJson(s.stage / "manifest.pending.json", JSON(JsonObject{{"version", JSON(1)}, {"targets", JSON(entries)}}));
    validateSources();
    // Generation names are reserved by a cooperative exclusive directory lock.
    if (std::filesystem::symlink_status(s.generation).type() != std::filesystem::file_type::not_found)
    {
        throw std::runtime_error("Bake output: generation appeared after preflight");
    }
    std::filesystem::rename(s.stage, s.generation);
    s.ownsStage = false;
    // A failure here leaves an unreferenced complete generation, never a partial manifest.
    db::publishFile(s.generation / "manifest.pending.json", s.manifest, s.overwrite);
    s.committed = true;
}

const Path& LightmapOutputWriter::manifestPath() const
{
    return _state->manifest;
}

const std::vector<LightmapOutputWriter::OutputPaths>& LightmapOutputWriter::outputs() const
{
    return _state->outputs;
}
}
