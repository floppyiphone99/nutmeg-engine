#ifndef NUTMEG_EDITOR_H
#define NUTMEG_EDITOR_H

#include "nutmeg_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle representing an ImGui powered Nutmeg editor application. */
typedef struct NmEditorApp NmEditorApp;

/** Allocate and initialise a new editor instance. */
NmEditorApp *nm_editor_app_create(void);

/** Destroy an editor created with nm_editor_app_create. */
void nm_editor_app_destroy(NmEditorApp *app);

/** Attach an engine instance whose metrics should be visualised. */
void nm_editor_app_set_engine(NmEditorApp *app, NutmegEngine *engine);

/** Request that the main loop exits at the next opportunity. */
void nm_editor_app_request_close(NmEditorApp *app);

/** Enter the blocking event/render loop. */
void nm_editor_app_main_loop(NmEditorApp *app);

#ifdef __cplusplus
}
#endif

#endif /* NUTMEG_EDITOR_H */
