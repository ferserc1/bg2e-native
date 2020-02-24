
# bg2 engine - APIs de C++

## Filosofía

`business-grade graphic engine`, en adelante `bg2 engine`, o simplemente `bg2`, es un motor gráfico especialmente creado para implementar aplicaciones con gráficos 3D de última generación en aplicaciones orientadas a empresas. Aunque existen motores gráficos mucho más potentes, la mayoría de éstos están orientados a la creación de videojuegos, y presentan ciertos problemas a la hora de realizar cierto tipo de acciones que suelen ser necesarias en aplicaciones que no son videojuegos:

- Soporte de múltiples ventanas: la mayoría de los motores gráficos populares para videojuegos no soportan múltiples ventanas.
- Framework basado en STL: el uso de la biblioteca estándar STL de c++ simplifica el proceso de aprendizaje y facilita la integración con otras bibliotecas.
- Extremadamente ligero: no es necesario cargar con un framework que ocupa decenas o centenares de megabytes, bg2 engine tiene lo justo para funcionar, sin extras que pueden ser prescindibles.
- Recursos portables: escenas, modelos 3d, materiales, etc. son facilmente portables. No es necesario empaquetar previamente los recursos, y de echo, las escenas se basan en ficheros de recursos independientes.
- 100% open source con licencia MIT: evidentemente, esto es una ventaja.

## Cuando usar bg2 engine (y cuando no usarlo)

Si estás pensando en crear un videojuego, o ninguno de los puntos vistos en el apartado anterior son importantes para tu aplicación, te recomiendo que utilices otros motores gráficos. Existen opciones libres de derechos o con licencias basadas en royalties sobre los beneficios de venta que quizá te convengan más. De echo, yo mismo utilizo otros motores gráficos para algunos de mis proyectos.

bg2 engine está disponible también en JavaScript con WebGL ([https://bitbucket.org/ferserc1/bg2e-js](https://bitbucket.org/ferserc1/bg2e-js), también disponible en [npm](https://www.npmjs.com/package/bg2e-js)). Las escenas y modelos 3D están optimizados para ser portables entre ambas APIs, incluso el editor de modelos y escenas es una aplicación [Electron](https://www.electronjs.org/) implementada con el API de JavaScript. Por lo tanto, es una buena opción si quieres hacer un proyecto web que además tenga soporte para aplicación nativa.

Existe una tercera API de bg2 engine basada en el motor gráfico Unreal ([https://github.com/ferserc1/bg2-unreal-tools](https://github.com/ferserc1/bg2-unreal-tools)). Está disponible como un plugin que permite cargar escenas y modelos 3D desde una aplicación Unreal, en tiempo de ejecución. Antes de decidirte por utilizar este API, quizá deberías investigar esta opción, que en muchos casos será más conveniente.

Por último, como bg2 engine es un framework ligero, está basado completamente en `Vulkan` ([https://www.khronos.org/vulkan/](https://www.khronos.org/vulkan/)), y en principio no contemplo la posibilidad de hacer una implementación que soporte otros motores. Si es importante para ti utilizar APIs específicas para tu plataforma destino, quizá esta no sea la mejor opción (aunque en macOS e iOS Vulkan está implementado como un wrapper de Metal, así que en cierto modo en estos casos bg2 funciona con Metal).

Si utilizas las APIs de más alto nivel, verás que la implementación es bastante agnóstica en cuanto a tecnología de renderización, pero tan pronto como quieras implementar funciones más cercanas al core necesitarás conocer `Vulkan` en detalle.

## Inicio rápido

- [Sistema de ventanas](examples/example-wnd.md)
- [Instancia de Vulkan](examples/example-vk-instance.md)
- [cubo con textura](examples/example-cube.md)

## Frameworks

- base
- db
- file
- math
- render
- wnd
