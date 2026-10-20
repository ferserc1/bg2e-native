# Composer preferences, temporal history and scene pacing

Manual regression checks for `draw_bg2e_composer`. Run on Metal and Vulkan using
an existing build. These checks do not require changes to CMake.

## Automatic graphics preferences

1. Change exposure, indirect lighting, temporal and denoise controls. Do not use
   a save button. Exit normally and relaunch: the applied values must return.
2. Resize the window and open/create another document. Graphics preferences must
   remain active; startup restoration must not replay over subsequent edits.
3. Change document lights/environment, save the scene and relaunch. Application
   graphics preferences must not overwrite the new document's authored values.
4. Save preferences to a JSON file, change settings, then restore from that file.
   Imported settings must apply and become the next startup defaults. Canceling
   either file dialog must leave settings unchanged. Invalid imports must report
   errors without replacing the saved application defaults.

## Temporal camera invalidation

1. Enable a supported noisy AO/GI/reflection effect and temporal accumulation.
   Leave the camera stationary: the signal must accumulate without input.
2. Move the pointer without dragging, click UI controls unrelated to rendering,
   and select objects. These events alone must not restart temporal history.
3. Orbit, pan or zoom the camera: history must restart on actual camera matrix
   changes, then accumulate again after movement stops. Verify the same behavior
   with an authored/animated camera, not only mouse-driven orbit input.
4. Enable upscaler jitter with a stationary camera. Sampling jitter alone must
   not trigger the camera-motion reset.

## Independent scene/UI rates

1. Open Window > UI Settings. Defaults: foreground scene FPS = 0 (unlimited),
   background scene FPS = 1. The UI runs at the available presentation rate.
2. Set foreground scene FPS to 5. Observe an animated/noisy scene advancing at
   approximately 5 FPS while menus, dragging panels and other UI remain smooth.
   Pointer events must not bypass the scene cap.
3. Focus another application without minimizing Composer. Scene rendering must
   switch to the background limit while UI presentation continues independently.
   Restore focus: the foreground limit must take effect promptly.
4. Set either limit to 0: that focus state must become unlimited. Relaunch:
   both values must be restored automatically.
5. Resize, load/import a scene and perform/cancel a lightmap bake. Scene pauses
   must remain honored; afterward continuous rendering and the selected limits
   must resume. Temporal history must use consecutive *scene* frame numbers,
   regardless of the number of UI presentations between scene updates.

No runtime results are claimed by this checklist.
