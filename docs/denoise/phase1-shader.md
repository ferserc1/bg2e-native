# Fase 1: Compute Shader GLSL

## Archivo

`shaders/src/denoise_bilateral.comp.glsl`

## Descripción

Implementar el compute shader GLSL que realiza el blur bilateral sensible a profundidad y normales.

## Especificación

### Local Size

```
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
```

### Bindings (set=0)

| Binding | Type | Variable | Descripción |
|---------|------|----------|-------------|
| 0 | `uniform sampler2D` | `g_InputImage` | Imagen a filtrar (AO) |
| 1 | `uniform sampler2D` | `g_Normal` | G-buffer normales (edge detection) |
| 2 | `uniform sampler2D` | `g_Depth` | G-buffer depth (edge detection) |
| 3 | `uniform image2D` | `outImage` | Resultado filtrado (RGBA8 storage) |

### Push Constants

```glsl
layout(push_constant) uniform PushConstant {
    vec2        outputSize;       // 8 bytes  -> offset 0 (vec2)
    int         kernelRadius;     // 4 bytes  -> offset 16
    float       depthThreshold;   // 4 bytes  -> offset 20
    float       normalThreshold;  // 4 bytes  -> offset 24
    float       depthSigma;       // 4 bytes  -> offset 28
    float       normalSigma;      // 4 bytes  -> offset 32
    uint        padding;          // 4 bytes  -> offset 36 (align to 16)
} pc;
```

Total: 40 bytes, aligned to 16.

### Algoritmo

Para cada píxel:

1. Obtener coordenadas del píxel y UV (`gl_GlobalInvocationID.xy`)
2. Boundary check: si sale del rango de la imagen, return
3. Muestrear valores centrales:
   - `inputColor = texture(g_InputImage, uv).r`
   - `centerNormal = texture(g_Normal, uv).xyz * 2.0 - 1.0`
   - `centerDepth = texture(g_Depth, uv).r`
4. Si `centerDepth >= 1.0` (fuera de escena), almacenar 1.0 y return
5. Iterar kernel `(2*radius+1) x (2*radius+1)`:
   - Calcular UV del vecino usando `texture()` (no `texelFetch`, para que soporte resoluciones diferentes)
   - Muestrear `neighborNormal` y `neighborDepth`
   - Calcular pesos:
     - `spatialWeight = exp(-distance^2 / (2.0 * float(radius*radius)))`
     - `depthDiff = abs(centerDepth - neighborDepth)`
     - `depthWeight = depthDiff < depthThreshold ? 1.0 : exp(-depthDiff^2 / (2.0 * depthSigma^2))`
     - `normalDot = clamp(dot(centerNormal, neighborNormal), 0.0, 1.0)`
     - `normalWeight = normalDot > normalThreshold ? 1.0 : exp(-(1.0 - normalDot) / normalSigma)`
     - `weight = spatialWeight * depthWeight * normalWeight`
   - Acumular `sum += neighborInput * weight` y `weightSum += weight`
6. `result = sum / weightSum`
7. `imageStore(outImage, pixelCoord, vec4(result, 0.0, 0.0, 1.0))`

### Observaciones

- Usar `texture()` en lugar de `texelFetch()` porque la imagen de entrada puede tener resolución diferente a la de los g-buffers
- Usar `imageSize()` para obtener el tamaño de la imagen de salida
- El kernel puede ser simétrico (misma ponderación espacial en todas direcciones)

## Funciones auxiliares

Incluir funciones de utilidad desde `lib/deferred_utils.glsl` si es necesario (hash, rand, etc.).

## Compilación

El CMake compilará automáticamente este `.glsl` a `.spv` en el post-build step de glslang. El archivo resultante será `denoise_bilateral.comp.spv`.