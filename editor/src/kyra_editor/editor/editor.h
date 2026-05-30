#pragma once

#include "kyra_editor/defines/shared.h"
#include "kyra_editor/defines/editor/editor.h"

#include <kyra/defines/core/types.h>
#include <kyra/defines/core/containers.h>


// API functions -------------------------------------------------- //

KYRA_EDITOR_API EditorResult    editor_startup(ConstStr config_filepath);
KYRA_EDITOR_API EditorResult    editor_shutdown(void);
KYRA_EDITOR_API EditorResult    editor_update(void);

KYRA_EDITOR_API EditorResult    editor_set_title(ConstStr title);
KYRA_EDITOR_API EditorResult    editor_set_size(Int32 width, Int32 height);
KYRA_EDITOR_API EditorResult    editor_set_vsync(Bool vsync);
KYRA_EDITOR_API EditorResult    editor_request_shutdown(void);

KYRA_EDITOR_API Bool            editor_should_close(void);

KYRA_EDITOR_API ConstStr        editor_result_to_string(const EditorResult result);
