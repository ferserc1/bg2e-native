# Export to bg2e Composer (Blender add-on)

This add-on exports the active Blender scene as a GLB and asks a running bg2e
Composer instance to import it. A later export of the same Blender scene uses
the same file path, so Composer replaces its previous import.

## Install and use

1. Install this `composer_export` folder as a Blender add-on (or make a ZIP
   containing the folder and install that ZIP through Blender Preferences).
2. Enable **Export to bg2e Composer** in Blender's add-on preferences.
3. Start Composer and check **File > Import Settings...**. The import service
   must be enabled. If its port differs from `8643`, set the matching port in
   the Blender add-on preferences.
4. In Blender's 3D View, open the sidebar (`N`), select **Composer**, and click
   **Export to Composer**. Blender reports the result after Composer responds.

The add-on uses Blender's built-in glTF exporter and Python standard library;
no Python packages need to be installed. It exports only the active scene,
using glTF's Y-up convention and meters. Composer must run on the same machine:
its service listens only on `127.0.0.1`.

Each Blender scene receives a persistent `bg2e_composer_export_id` custom
property on its first export. Its GLB is written to
`<system-temp>/bg2e_composer_export/<id>.glb`. This keeps the path stable when
an unsaved Blender document is later saved or renamed. Save the `.blend` file
to retain the identifier between Blender sessions. Unsaved scenes retain it
only for the current session. A scene duplicated within the same Blender file
receives a separate identifier when exported. Temporary GLBs may be removed by
the operating system; the next export recreates them at the same path.

The add-on waits for the import response without blocking Blender's interface.
Composer may take up to 60 seconds to return an import result. The add-on
reports HTTP errors and connection failures in Blender's status area.
