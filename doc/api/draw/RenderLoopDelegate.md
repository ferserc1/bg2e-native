# RenderLoopDelegate

**Header:** `<bg2e/draw/RenderLoopDelegate.hpp>`  
**Namespace:** `bg2e::draw`

A separate experimental delegate. It does not inherit from or modify
`render::RenderLoopDelegate`, and its signatures contain no native backend types.

```cpp
virtual ~RenderLoopDelegate() = default;
virtual void init(Engine* engine);
virtual void initScene();
virtual void resize(gpu::Size2D newExtent);
virtual void update(const FrameContext& frameContext);
virtual void render(const FrameContext& frameContext) = 0;
virtual void cleanup();
```

The default init stores the borrowed Engine pointer in protected `_engine`.
Derived overrides should call the base implementation if they use that member.
initScene, resize, update and cleanup default to no-ops. render is the only
pure virtual callback. There is no public engine() or delta() accessor on this
delegate; frame metadata is provided through FrameContext.

## Resource model

Objects own and reuse resources for the in-flight slots they need. The delegate
has no production-style initFrameResources callback or central descriptor-pool
setup contract. It also has no scene/loadScene API yet: future scene-oriented
delegates can layer those operations on top independently.

## Registration and milestone status

Register with Application::setRenderDelegate(shared_ptr<draw::RenderLoopDelegate>),
then choose MainLoop's two-argument run overload. Using the production run
overload with this delegate is a configuration error. See
[Application](../app/Application_and_input.md#graphics-delegate-registration).

Milestone 02 step 03 invokes init, initScene and resize before update/render.
The first available drawable establishes the actual pixel extent. A dirty,
unpaused scene receives update then render; a clean or paused scene continues
presentation without these callbacks. Render opens and closes its own command
scopes; no depth target or resource-set initialization phase is supplied.

Cleanup is called once per initialized lifecycle, including a partially failed
init callback. Repeated RenderLoop cleanup does not invoke it again. Producers
must be stopped before teardown; the loop drains GPU work before releasing
scene resources.
