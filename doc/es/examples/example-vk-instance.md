
# Ejemplo: instancia de Vulkan

Implementado en `examples/vk-instance/main.cpp`

Aunque bg2 engine está basado en Vulkan, sus APIs están ideadas para hacer automáticas muchas cosas que en Vulkan hay que hacer manualmente. No obstante, lo primero que necesitarás antes de nada es tener una instancia de Vulkan.

La instancia de Vulkan en bg2 engine se implementa mediante la clase `bg2render::vk::Instance`, y al contrario de lo que ocurre con Vulkan, esta clase alberga más cosas:

- Instancia de vulkan: manejador nativo de Vulkan, que puede obtenerse mediante el método `Instance::instance()`.
- La superficie de renderizado (`Instance::surface()`)
- El dispositivo físico a utilizar (`Instance::renderPhysicalDevice()`)
- El dispositivo a utilizar para renderizar (`Instance::renderDevice()`)
- El dispositivo a utilizar para presentar el fotograma (`Instance::presentDevice()`)
- Las colas de renderizado y presentación (`Instance::renderQueue()` e `Instance::presentQueue()`)

La creación de la instancia se puede hacer en la función `init()` del delegado de la ventana. Usaremos el propio delegado de la ventana para almacenar la instancia de vulkan.

## Creación: método abreviado

Hay dos formas de crear una instancia de Vulkan: el método abreviado crea una instancia de vulkan con las opciones por defecto, que nos servirán en la gran mayoría de las circunstancias.

```c++
_instance = std::shared_ptr<bg2render::vk::Instance>(bg2render::vk::Instance::CreateDefault(window(), "bg2 vulkan test"));
```

`_instance` es un atributo del delegado de la ventana.

`window()` es una función del delegado de la ventana que devuelve un puntero a la ventana. A través de este puntero, la instancia de Vulkan obtendrá todos los recursos que necesite para crear los dispositivos y la superficie de renderizado, así como las extensiones requeridas por Vulkan para funcionar en la plataforma actual.

usamos `std::shared_ptr<>` para manejar los punteros. Para más información, consultar la documentación de STL.

La función `CreateDefaultWindow()` incluye automáticamente una callback de manejo de error genérica cuando se compila en la configuración `_DEBUG`.

## Creación: método personalizado

Si queremos personalizar la forma de crear la instancia de vulkan, podemos hacer el proceso anterior a mano:

```c++
_instance = std::mak_shared<bg2render::vk::Instance>();

_instance->configureAppName("bg2 render vulkan instance example");
#ifdef _DEBUG
    _instance->setDebugCallback([](
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* pData
    ) {
        if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            std::cerr << pData->pMessage << std::endl;
        }
        else {
            std::cout << pData->pMessage << std::endl;
        }
    });

    std::vector<VkExtensionProperties> extensionData;
    _instance->enumerateInstanceExtensionProperties(extensionData);
    std::cout << "Available Vulkan instance extensions: " << std::endl;
    for (const auto & ext : extensionData) {
        std::cout << "\t" << ext.extensionName << std::endl;
    }
#endif

std::vector<const char*> extensions;
window()->getVulkanRequiredInstanceExtensions(extensions);
_instance->configureRequiredExtensions(extensions);
_instance->create();

_instance->setSurface(window()->createVulkanSurface(_instance->instance()));
_instance->choosePhysicalDevice();
```

Antes de llamar a `window->configureRequiredExtensions()`, podemos añadir otras extensiones de instancia que necesitemos.

La función `Instance::choosePhysicalDevice()` escogerá el primer dispositivo que cumpla los requisitos mínimos. No obstante, este comportamiento puede ser personalizado. Por ejemplo, podríamos listar manualmente los dispositivos disponibles y dejar al usuario que escoja, elegir el dispositivo en función de una configuración previa o utilizar cualquier otro criterio. Para escoger el dispositivo manualmente, puedes tomar como ejemplo el código de `Instance::choosePhysicalDevice()`, definido en `bg2-render/src/vk_instance.cpp`:

```c++
void Instance::choosePhysicalDevices() {
    std::vector<std::shared_ptr<PhysicalDevice>> devices;
    enumeratePhysicalDevices(devices);

    // In this version, we'll use the same device for render and present
    for (const auto & device : devices) {
        if (_renderPhysicalDevice ==nullptr && 
            isDeviceSuitableForTask(device.get(), kDeviceTaskRender) &&
            isDeviceSuitableForTask(device.get(), kDeviceTaskPresent)) {
            _renderPhysicalDevice = device;
        }
    }

    if (_renderPhysicalDevice == nullptr) {
        throw std::runtime_error("No such suitable Vulkan device for render.");
    }

    // We use the same device for presentation and render
    _renderDevice = std::make_shared<Device>(_renderPhysicalDevice.get(), kDeviceTaskRender | kDeviceTaskPresent);
    if (_debugMessenger) {
        _renderDevice->configureEnabledLayers(std::vector<const char*>{
            "VK_LAYER_KHRONOS_validation"
        });
    }
    _renderDevice->configureEnabledExtensions(std::vector<const char*>{
        "VK_KHR_swapchain"
    });
    
    _renderDevice->create();

    _presentDevice = _renderDevice;
}
```

