
# Ejemplo: sistema de ventanas

Para interactuar con el sistema de ventanas necesitas:

- Crear un delegado para la ventana: una clase donde la aplicación envía los eventos del ciclo de vida de la aplicación, y la entrada de usuario.
- Crear una instancia de la aplicación: la instancia de la aplicación es la encargada de manejar el bucle de eventos y de mantener un registro de las ventanas activas.
- Crear una ventana y asignarle el delegado

## Incluir las cabeceras del motor gráfico

La opción más sencilla consiste en incluir todo el motor gráfico:

```c++
#include <bg2engine.hpp>
```

No obstante, puedes incluir solamente el framework que necesites, por ejemplo:

```c++
#include <bg2base/all.hpp>
```

Para optimizar el tiempo de compilación, en ocasiones preferirás incluir los ficheros que necesites por separado. En este caso el patrón es el siguiente:

```c++
#include <bg2[framework]/[fichero].hpp>

// por ejemplo
#include <bg2base/image.hpp>
#include <bg2math/matrix.hpp>
```

## Delegado de la ventana

Se implementa extendiendo la clase `bg2wnd::WindowDelegate`, donde implementaremos los eventos que queramos capturar:

```c++
class MyWindowDelegate : public bg2wnd::WindowDelegate {
public:
    // Se llama justo después de mostrar la ventana
	void init() {}

    // Se llama al establecer el tamaño de la ventana por primera vez, y cada
    // vez que el usuario la redimensiona
    void resize(const bg2math::int2 & size) {}

    // Se llama una vez al principio de cada fotograma, pasando como parámetro 
    // el tiempo en ms que ha transcurrido desde la llamada anterior
    void update(float delta) {}

    // Se llama justo después de update, y aquí es donde se realizan las acciones
    // necesarias para presentar el fotograma actual
    void draw() {}

    // Se llama justo antes de destruir la ventana
    void cleanup() {}

    // Eventos de entrada de usuario:
    void keyDown(const bg2wnd::KeyboardEvent & e) {}
    void keyUp(const bg2wnd::KeyboardEvent & e) {}
    void charPress(const bg2wnd::KeyboardEvent & e) {}
    void mouseMove(const bg2wnd::MouseEvent & e) {}
    void mouseDown(const bg2wnd::MouseEvent & e) {}
    void mouseUp(const bg2wnd::MouseEvent & e) {}
    void mouseWheel(const bg2wnd::MouseEvent & e) {}
};
```

## Instancia de la aplicación

La instancia de la aplicación es un singleton que se crea mediante un método que actúa como factoría. Al usarlo, estaremos creando el tipo de aplicación específica para nuestra plataforma (Windows, Linux, macOS, etc.).

Para iniciar el bucle de eventos de la aplicación, se utiliza el método `int run()`, que permanecerá activo mientras que queden ventanas abiertas, y al acabar devolverá el código de error, o cero si todo ha ido bien.

```c++
int main(int argc, char ** argv) {
    bg2wnd::Application * app = bg2wnd::Application::Get();

    // Inserta aquí el código de creación de tu ventana

    return app->run();
}
```

## Ventanas

Las ventanas se crean mediante el método factoría `Window::New()`, que creará una instancia de la ventana específica del sistema operativo actual. Al crear la ventana tenemos que especificar el delegado de la ventana y todos los parámetros relevantes. Por último, añadiremos la ventana a la instancia de la aplicación mediante la función `Application::add(Window*)`.

```c++
auto window = bg2wnd::Window::New();
window->setWindowDelegate(new MyWindowDelegate());
window->setTitle("Window 1");
window->setPosition({ 40, 40 });
window->setSize({ 800, 600 });
app->addWindow(window);
```



