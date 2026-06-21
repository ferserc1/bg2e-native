# Tutorial 10: CubemapRenderPass — IBL Environment Map Generation

This tutorial walks through the `10_cubemap_render_pass` example: generating three environment cubemaps (original, diffuse irradiance, and specular reflection) using `gpu::CubemapRenderPass`, then visualizing them with a reflective cube. The `CubemapRenderPass` utility encapsulates the repetitive iteration over cubemap faces and mip levels, reducing boilerplate for IBL (Image-Based Lighting) precomputation.

**Source:** `examples/gpu/10_cubemap_render_pass/src/main.cpp`

## What you will learn

- How to use `gpu::CubemapRenderPass::render()` to generate cubemaps without manual face iteration
- How to create cubemap render targets with multiple mip levels
- How to implement equirectangular-to-cubemap projection
- How to implement diffuse irradiance convolution
- How to implement specular reflection prefiltering with roughness
- How to use `minMipSize` to control the mip chain depth
- How to use `textureLod()` in GLSL to sample specific mip levels
- How to use Hammersley quasi-random sampling and GGX importance sampling in shaders

## Prerequisites

- Completed [09_cubemap](09_cubemap.md) -- you should understand cubemap basics, image layout transitions, and multi-pass pipelines
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding IBL cubemap generation

Image-Based Lighting (IBL) precomputes lighting information from an environment map into textures that can be efficiently sampled at runtime. This example generates three cubemaps:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│ 1. Original         │     │ 2. Irradiance       │     │ 3. Specular         │
│    Environment      │     │    Convolution      │     │    Prefilter         │
│    (1024 px)        │────>│    (32 px)          │     │    (1024 px + mips)  │
│                     │     │                     │     │                     │
│ Equirect -> Cube    │     │ Cosine-weighted     │     │ GGX importance      │
│ projection          │     │ hemisphere integral │     │ sampling per mip    │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

| Cubemap | Size | Mip Levels | Purpose |
|---------|------|------------|---------|
| Original | 1024 px | 1 | Raw environment capture |
| Irradiance | 32 px | 1 | Diffuse IBL lookup |
| Specular | 1024 px | Multiple (down to 8 px) | Specular IBL with roughness |

The `CubemapRenderPass` helper iterates the six faces and, for multi-mip cubemaps, all mip levels. The callback receives the current face, mip level, and the precomputed projection/view matrices.

**Reference:** [GPU API -- CubemapRenderPass header](../../lib/include/bg2e/gpu/CubemapRenderPass.hpp)

## Step-by-step code explanation

### 1. Data structures for push constants and UBOs

```cpp
struct CubemapCameraPushConstants {
    glm::mat4 projection;
    glm::mat4 view;
};

struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 cameraPos;
};

struct ModelUBO {
    glm::mat4 model;
};

struct RoughnessUBO {
    float roughness = 0.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

struct ReflectPushConstants {
    glm::vec3 cameraPos;
    float     lod = 0.0f;
};
```

This example defines five data structures:

- **`CubemapCameraPushConstants`**: projection and view matrices for each cubemap face, passed via push constants to the cubemap renderer vertex shader
- **`CameraUBO`**: persistent camera data with projection, view, and world position
- **`ModelUBO`**: model matrix, updated per-frame for the rotating cube
- **`RoughnessUBO`**: per-mip roughness value for the specular prefilter shader. The 16-byte alignment is enforced with explicit padding fields
- **`ReflectPushConstants`**: camera position and LOD level, passed via push constants to the reflection fragment shader

The `RoughnessUBO` uses `float` with explicit padding to ensure 16-byte alignment, which is required for Vulkan uniform buffer compatibility. Each mip level of the specular cubemap receives its own UBO with a roughness value linearly interpolated from 0.0 (mip 0) to 1.0 (last mip).

### 2. Four pipeline layouts for different cubemap operations

```cpp
// Equirect -> cube: vertex push constants + set0 (2D texture).
gpu::PipelineLayoutDescription equirectLayoutDesc{};
equirectLayoutDesc.pushConstants.push_back(
    {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
);
equirectLayoutDesc.resourceBindings.push_back(
    {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1}
);
equirectLayoutDesc.resourceBindings.push_back(
    {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1}
);
auto equirectLayout = device->createPipelineLayout(equirectLayoutDesc);
```

This layout defines the resources for the equirectangular-to-cubemap projection pass:

- **Push constants**: projection + view matrices for the vertex stage
- **Set 0, binding 0**: sampled 2D texture (the equirectangular HDR image)
- **Set 0, binding 1**: sampler for the texture

The irradiance convolution layout is identical in structure but binds a cubemap instead of a 2D texture. The specular prefilter layout adds a second descriptor set for the per-mip roughness UBO.

**Reference:** [GPU API -- PipelineLayout](../../api/gpu/PipelineLayout.md)

### 3. Cubemap render targets with mip levels

```cpp
// Original environment cubemap, single mip.
const uint32_t originalSize = 1024;
auto originalCubeMap = device->createCubeMap({
    .size = originalSize,
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled
           | gpu::ImageUsage::ColorAttachment
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::TransferDst,
    .mipLevels = 1,
    .debugName = "Original environment cubemap"
});

// Specular reflection map: same size as the original, with multiple mips.
// minMipSize = 8 makes the mip chain stop at 8x8 px.
auto specularCubeMap = device->createCubeMap({
    .size = originalSize,
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled
           | gpu::ImageUsage::ColorAttachment
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::TransferDst,
    .minMipSize = 8,
    .debugName = "Specular reflection cubemap"
});
```

Key points:

- **`R16G16B16A16_SFLOAT`**: half-float format preserves HDR range for environment data
- **`mipLevels = 1`**: original and irradiance cubemaps have a single mip level
- **`minMipSize = 8`**: the specular cubemap auto-generates mip levels down to 8x8 pixels. The `minMipSize` parameter controls the minimum mip dimension, determining how many mip levels are created (for 1024px with minMipSize=8: 1024 -> 512 -> 256 -> ... -> 8 = 8 levels)
- **Usage flags**: `Sampled` for shader access, `ColorAttachment` for rendering, `TransferSrc/Dst` for layout transitions

The `mipLevels()` method returns the actual number of mip levels after creation, which is used to create per-mip roughness resources.

**Reference:** [GPU API -- Image](../../api/gpu/Image.md)

### 4. Resource sets for textures and roughness

```cpp
auto equirectSet = device->createResourceSet(equirectLayout.get(), 0, "Equirect source set");
equirectSet->setSampledImage({.vulkan = 0, .metal = 0}, equirectTexture.get());
equirectSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
equirectSet->update();
```

Resource sets bind GPU resources to descriptor set slots. The equirect set binds the HDR texture and sampler to set 0.

For the specular prefilter, a separate UBO and resource set are created per mip level:

```cpp
for (uint32_t mip = 0; mip < specularMipLevels; ++mip)
{
    RoughnessUBO data{};
    data.roughness = specularMipLevels > 1
        ? float(mip) / float(specularMipLevels - 1)
        : 0.0f;

    auto ubo = device->createBuffer("Roughness UBO mip[" + std::to_string(mip) + "]");
    ubo->createUniformBuffer(data);

    auto set = device->createResourceSet(specularLayout.get(), 1,
        "Roughness set mip[" + std::to_string(mip) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 1}, ubo);
    set->update();

    roughnessUbos.push_back(ubo);
    roughnessSets.push_back(set);
}
```

Each mip level gets a distinct buffer because all resource sets are consumed within a single command buffer submission. The roughness value is linearly interpolated: mip 0 = 0.0 (smooth), last mip = 1.0 (rough).

**Reference:** [GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)

### 5. Mesh creation for different purposes

```cpp
// Inverted sphere with equirect UVs for the equirect -> cube pass
std::unique_ptr<bg2e::geo::MeshPU> sphereData(bg2e::geo::createSpherePU(50.0f, 32, 32, true));
gpu::MeshPU skyboxSphere;
skyboxSphere.setMeshData(*sphereData);
skyboxSphere.build(device.get());

// Unit cube for direction sampling in convolution/prefilter passes
std::unique_ptr<bg2e::geo::MeshPU> dirCubeData(bg2e::geo::createCubePU(2.0f, 2.0f, 2.0f));
gpu::MeshPU dirCube;
dirCube.setMeshData(*dirCubeData);
dirCube.build(device.get());

// On-screen reflective cube with normals
std::unique_ptr<bg2e::geo::MeshPNU> reflectCubeData(bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f));
gpu::MeshPNU reflectCube;
reflectCube.setMeshData(*reflectCubeData);
reflectCube.build(device.get());
```

Three meshes serve different purposes:

- **`skyboxSphere`**: inverted sphere with equirectangular UV coordinates. Used for the equirect-to-cube pass where the fragment shader samples the HDR texture using UV coordinates
- **`dirCube`**: unit cube spanning [-1, 1]. Used for irradiance and specular passes where the vertex position is used as the sampling direction (the fragment shader reads `fragDir = inPosition`)
- **`reflectCube`**: standard cube with position, normal, and UV attributes. Rendered on-screen with environment reflection

### 6. CubemapRenderPass usage

```cpp
gpu::CubemapRenderPassInfo genInfo{};
genInfo.clear           = true;
genInfo.clearColor      = gpu::Color(0.0f, 0.0f, 0.0f, 1.0f);
genInfo.transitionBefore = true;
genInfo.transitionAfter  = true;

device->immediateSubmit([&](gpu::CommandBuffer* cmd)
{
    // 1) Original environment cubemap from the equirectangular texture.
    gpu::CubemapRenderPass::render(cmd, originalCubeMap.get(), genInfo,
        [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t,
            const glm::mat4& projection, const glm::mat4& view)
        {
            CubemapCameraPushConstants pc{ projection, view };
            c->bindPipeline(equirectPipeline.get());
            c->bindResourceSet(equirectPipeline.get(), 0, equirectSet.get());
            c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
            skyboxSphere.draw(c);
        });

    // 2) Diffuse irradiance map convolved from the original cubemap.
    gpu::CubemapRenderPass::render(cmd, irradianceCubeMap.get(), genInfo,
        [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t,
            const glm::mat4& projection, const glm::mat4& view)
        {
            CubemapCameraPushConstants pc{ projection, view };
            c->bindPipeline(irradiancePipeline.get());
            c->bindResourceSet(irradiancePipeline.get(), 0, irradianceSourceSet.get());
            c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
            dirCube.draw(c);
        });

    // 3) Specular reflection map with per-mip roughness.
    gpu::CubemapRenderPass::render(cmd, specularCubeMap.get(), genInfo,
        [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t mipLevel,
            const glm::mat4& projection, const glm::mat4& view)
        {
            CubemapCameraPushConstants pc{ projection, view };
            c->bindPipeline(specularPipeline.get());
            c->bindResourceSet(specularPipeline.get(), 0, specularSourceSet.get());
            c->bindResourceSet(specularPipeline.get(), 1, roughnessSets[mipLevel].get());
            c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
            dirCube.draw(c);
        });
});
```

`CubemapRenderPass::render()` encapsulates:

1. **Face iteration**: loops over 6 cubemap faces (+X, -X, +Y, -Y, +Z, -Z)
2. **Mip iteration**: for multi-mip cubemaps, loops over all mip levels
3. **Matrix computation**: builds 90-degree FOV projection and per-face view matrices
4. **Image transitions**: transitions cubemap to `ColorAttachment` before and `ShaderReadOnly` after
5. **Begin/end rendering**: calls `cmd->beginRendering(cubemap, face, mipLevel)` and `cmd->endRendering()`
6. **Viewport/scissor**: derived automatically from the mip size

The callback receives the current face, mip level, and matrices. For the specular map, the `mipLevel` parameter is used to select the appropriate roughness resource set.

**Reference:** [GPU API -- CubemapRenderPass header](../../lib/include/bg2e/gpu/CubemapRenderPass.hpp)

### 7. Render loop with controls

```cpp
int activeMap   = 0;   // 0 = original, 1 = irradiance, 2 = specular
uint32_t specularLod = 0;

// In event handling:
if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
{
    activeMap = (activeMap + 1) % 3;
}
if (event.type == SDL_KEYDOWN &&
    (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER))
{
    specularLod = (specularLod + 1) % specularMipLevels;
}

// In render:
ReflectPushConstants reflectPush{};
reflectPush.cameraPos = cameraEye;
reflectPush.lod = (activeMap == 2) ? float(specularLod) : 0.0f;

cmd->bindResourceSet(reflectPipeline.get(), 2, cubeSets[activeMap].get());
```

The render loop draws a rotating cube with environment reflection. Controls:

- **Space**: cycles through original -> irradiance -> specular -> original cubemap
- **Enter**: when the specular map is active, cycles through mip levels (roughness values)

The `lod` push constant is only non-zero when the specular map is selected, allowing `textureLod()` in the fragment shader to sample a specific mip level.

## Shader code explanation

### Equirect to Cube (equirect_to_cube)

The vertex shader projects the inverted sphere using the cubemap face matrices and passes UV coordinates to the fragment shader:

```glsl
void main()
{
    gl_Position = camera.projection * camera.view * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

The fragment shader samples the equirectangular texture using the interpolated UV:

```glsl
outColor = texture(sampler2D(uTex, uSampler), fragUV);
```

This is the simplest pass: direct texture lookup from the HDR image to the cubemap face.

### Irradiance Convolution (irradiance_convolution)

The vertex shader passes the cube vertex position as the sampling direction:

```glsl
fragDir = inPosition;
gl_Position = camera.projection * camera.view * vec4(inPosition, 1.0);
```

The fragment shader performs a cosine-weighted hemisphere integral:

```glsl
vec3 N = normalize(fragDir);
vec3 up    = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
vec3 right = normalize(cross(up, N));
up = normalize(cross(N, right));

vec3  irradiance  = vec3(0.0);
float sampleDelta = 0.05;
float nrSamples   = 0.0;

for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
{
    for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    {
        vec3 tangentSample = vec3(sin(theta) * cos(phi),
                                  sin(theta) * sin(phi),
                                  cos(theta));
        vec3 sampleVec = tangentSample.x * right
                       + tangentSample.y * up
                       + tangentSample.z * N;
        irradiance += texture(samplerCube(uEnv, uSampler), sampleVec).rgb
                    * cos(theta) * sin(theta);
        nrSamples += 1.0;
    }
}
irradiance = PI * irradiance / nrSamples;
```

Key concepts:

- **Tangent space construction**: builds an orthonormal basis from the face normal `N`
- **Double integral**: iterates over azimuthal (`phi`) and polar (`theta`) angles covering the hemisphere
- **Cosine weighting**: `cos(theta)` weights samples by their contribution to diffuse lighting
- **Solid angle element**: `sin(theta)` accounts for the spherical coordinate Jacobian
- **Normalization**: divides by sample count and multiplies by PI to approximate the continuous integral

### Specular Prefilter (specular_prefilter)

The vertex shader is identical to irradiance (passes `fragDir = inPosition`). The fragment shader uses GGX importance sampling:

```glsl
layout(set = 1, binding = 0) uniform RoughnessUBO {
    float roughness;
    float _pad0;
    float _pad1;
    float _pad2;
} params;

void main()
{
    vec3 N = normalize(fragDir);
    vec3 R = N;
    vec3 V = N;

    const uint SAMPLE_COUNT = 64u;
    float totalWeight       = 0.0;
    vec3  prefilteredColor  = vec3(0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, params.roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            prefilteredColor += texture(samplerCube(uEnv, uSampler), L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / max(totalWeight, 0.001);
}
```

Key concepts:

- **Hammersley sequence**: generates quasi-random 2D samples with low discrepancy for uniform hemisphere coverage
- **GGX importance sampling**: concentrates samples around the GGX distribution's peak based on roughness, reducing noise compared to uniform sampling
- **Reflection vector**: computes `L` from the half-vector `H` using the reflection equation `L = 2 * dot(V, H) * H - V`
- **NdotL weighting**: weights contributions by the cosine term and normalizes by total weight

The roughness UBO (`params.roughness`) varies per mip level: mip 0 = 0.0 (mirror-like), higher mips = increasingly rough.

### Reflect Cube (reflect_cube)

The vertex shader transforms vertices to world space and computes the reflection vector:

```glsl
layout(set = 0, binding = 0) uniform CameraUBO { ... } camera;
layout(set = 1, binding = 0) uniform ModelUBO { ... } object;

void main()
{
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    gl_Position  = camera.projection * camera.view * worldPos;
    fragWorldPos = worldPos.xyz;
    fragNormal   = mat3(object.model) * inNormal;
}
```

The fragment shader computes per-pixel reflection:

```glsl
layout(push_constant) uniform PushConstants {
    vec3  cameraPos;
    float lod;
} push;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 I = normalize(fragWorldPos - push.cameraPos);
    vec3 R = reflect(I, N);
    outColor = textureLod(samplerCube(uCubeMap, uSampler), R, push.lod);
}
```

- **`reflect(I, N)`**: computes the reflection vector from the incident direction and surface normal
- **`textureLod()`**: samples the cubemap at an explicit mip level, controlled by `push.lod`. For the original and irradiance maps, `lod = 0`. For the specular map, `lod` cycles through the mip chain to visualize different roughness levels

## Key concepts

### CubemapRenderPass::render()

`CubemapRenderPass` is a low-level helper that eliminates the boilerplate of iterating cubemap faces and mip levels. It does **not** own or create the cubemap -- it only drives the render iteration. All actual rendering content lives in the callback.

The helper handles:

- Face/mip iteration
- Per-face projection (90-degree FOV, 1:1 aspect) and view matrices
- `beginRendering` / `endRendering` calls
- Viewport/scissor derivation from mip size
- Optional image transitions (before/after)

The `CubemapRenderPassInfo` struct configures:

| Field | Purpose |
|-------|---------|
| `clear` | Clear each face/mip before rendering |
| `clearColor` | Clear color value |
| `transitionBefore` | Transition cubemap to ColorAttachment before rendering |
| `transitionAfter` | Transition cubemap to ShaderReadOnly after rendering |
| `nearPlane` / `farPlane` | Perspective projection parameters |

### Mip level management

The specular cubemap uses `minMipSize = 8` to auto-generate mip levels. Each mip stores the environment prefiltered for a specific roughness value:

| Mip Level | Size | Roughness |
|-----------|------|-----------|
| 0 | 1024 px | 0.0 (mirror) |
| 1 | 512 px | ~0.14 |
| 2 | 256 px | ~0.29 |
| ... | ... | ... |
| 7 | 8 px | 1.0 (fully rough) |

At runtime, `textureLod()` selects which mip to sample based on surface roughness.

### IBL pipeline summary

```
Equirectangular HDR Texture
         │
         ▼
┌─────────────────────┐
│ Equirect -> Cube    │  CubemapRenderPass (6 faces)
│ (skybox sphere)     │
└─────────┬───────────┘
         │
         ▼
┌─────────────────────┐     ┌─────────────────────┐
│ Irradiance Conv.    │     │ Specular Prefilter   │
│ (32px, 1 mip)       │     │ (1024px, N mips)     │
│ Cosine-weighted     │     │ GGX importance       │
│ hemisphere integral │     │ sampling per mip     │
└─────────┬───────────┘     └─────────┬───────────┘
         │                           │
         ▼                           ▼
┌─────────────────────────────────────────────────┐
│ Runtime: sample irradiance for diffuse,         │
│ specular mip for glossy reflection              │
└─────────────────────────────────────────────────┘
```

## Next steps

- Experiment with different `minMipSize` values to observe the tradeoff between mip chain depth and specular quality
- Try reducing `SAMPLE_COUNT` in the specular prefilter to see the impact on noise
- Integrate the generated cubemaps into a PBR material system for full IBL lighting
- Add diffuse and specular IBL term combination with the split-sum approximation

See also: [09_cubemap](09_cubemap.md) for the foundational cubemap rendering example without `CubemapRenderPass`.
