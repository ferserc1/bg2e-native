"""Send the active Blender scene to a running bg2e Composer instance."""

bl_info = {
    "name": "Export to bg2e Composer",
    "author": "bg2e",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Composer",
    "description": "Export the active scene as GLB and import it into Composer",
    "category": "Import-Export",
}

import json
from pathlib import Path
from queue import Empty, Queue
import tempfile
from threading import Thread
import uuid
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

import bpy
from bpy.props import IntProperty


_SCENE_ID_KEY = "bg2e_composer_export_id"
_TIMEOUT_SECONDS = 70  # Composer returns 504 after 60 seconds.


def _export_path(scene):
    """Keep the same absolute path for every export of this scene."""
    export_id = scene.get(_SCENE_ID_KEY)
    try:
        export_id = str(uuid.UUID(str(export_id)))
    except (ValueError, TypeError, AttributeError):
        export_id = None

    # Duplicating a Blender scene copies custom properties. Give the copy its
    # own path so exporting it cannot replace the original scene in Composer.
    earlier_scenes = list(bpy.data.scenes)
    earlier_scenes = earlier_scenes[:earlier_scenes.index(scene)]
    if export_id is None or any(other.get(_SCENE_ID_KEY) == export_id for other in earlier_scenes):
        export_id = str(uuid.uuid4())
        scene[_SCENE_ID_KEY] = export_id

    directory = Path(tempfile.gettempdir()) / "bg2e_composer_export"
    directory.mkdir(parents=True, exist_ok=True)
    return directory / (export_id + ".glb")


def _response_message(raw):
    try:
        payload = json.loads(raw.decode("utf-8"))
        if isinstance(payload, dict) and isinstance(payload.get("message"), str):
            return payload["message"]
    except (UnicodeDecodeError, ValueError):
        pass
    return raw.decode("utf-8", errors="replace").strip() or "No response details"


def _send_import(path, file_name, port, results):
    body = json.dumps({
        "filePath": str(path),
        "fileName": file_name,
        "units": "m",
        "coordinateSystem": "y_up",
    }).encode("utf-8")
    request = Request(
        "http://127.0.0.1:{}/import".format(port),
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urlopen(request, timeout=_TIMEOUT_SECONDS) as response:
            message = _response_message(response.read())
            results.put((response.status == 200, message))
    except HTTPError as error:
        results.put((False, "Composer HTTP {}: {}".format(
            error.code, _response_message(error.read()))))
    except (URLError, TimeoutError, OSError) as error:
        results.put((False, "Cannot reach Composer: {}".format(error)))
    except Exception as error:
        results.put((False, "Composer request failed: {}".format(error)))


class COMPOSER_OT_export_scene(bpy.types.Operator):
    bl_idname = "composer.export_scene"
    bl_label = "Export to Composer"
    bl_description = "Export the active scene to GLB and import it into Composer"

    _busy = False

    @classmethod
    def poll(cls, context):
        return context.scene is not None and not cls._busy

    def execute(self, context):
        try:
            path = _export_path(context.scene)
            result = bpy.ops.export_scene.gltf(
                filepath=str(path),
                export_format="GLB",
                export_yup=True,
                use_active_scene=True,
                check_existing=False,
            )
            if "FINISHED" not in result or not path.is_file():
                raise RuntimeError("GLB export did not complete")
        except Exception as error:
            self.report({"ERROR"}, "Composer export failed: {}".format(error))
            return {"CANCELLED"}

        preferences = context.preferences.addons[__package__].preferences
        self._results = Queue(maxsize=1)
        self._timer = context.window_manager.event_timer_add(0.2, window=context.window)
        try:
            Thread(
                target=_send_import,
                args=(path, path.name, preferences.port, self._results),
                daemon=True,
            ).start()
            context.window_manager.modal_handler_add(self)
        except Exception as error:
            context.window_manager.event_timer_remove(self._timer)
            self.report({"ERROR"}, "Cannot start Composer request: {}".format(error))
            return {"CANCELLED"}
        type(self)._busy = True
        self.report({"INFO"}, "Waiting for Composer import...")
        return {"RUNNING_MODAL"}

    def modal(self, context, event):
        if event.type != "TIMER":
            return {"PASS_THROUGH"}
        try:
            ok, message = self._results.get_nowait()
        except Empty:
            return {"PASS_THROUGH"}

        context.window_manager.event_timer_remove(self._timer)
        type(self)._busy = False
        self.report({"INFO"} if ok else {"ERROR"}, message)
        return {"FINISHED"} if ok else {"CANCELLED"}


class COMPOSER_PT_export(bpy.types.Panel):
    bl_label = "Composer"
    bl_idname = "COMPOSER_PT_export"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Composer"

    def draw(self, context):
        self.layout.operator(COMPOSER_OT_export_scene.bl_idname, icon="EXPORT")


class COMPOSER_AddonPreferences(bpy.types.AddonPreferences):
    bl_idname = __package__

    port: IntProperty(name="Composer port", default=8643, min=1024, max=49151)

    def draw(self, context):
        self.layout.prop(self, "port")


_CLASSES = (COMPOSER_AddonPreferences, COMPOSER_OT_export_scene, COMPOSER_PT_export)


def register():
    for cls in _CLASSES:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_CLASSES):
        bpy.utils.unregister_class(cls)
