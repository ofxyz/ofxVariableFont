#pragma once

// ============================================================================
// Variable-font Get/Set Axis nodes for ofxImGuiNodeEditor
// ============================================================================
// Opt-in. Add ofxImGuiNodeEditor to addons.make, then:
//
//   #include "VarFontNodes.h"
//   varfont::registerVariableFontNodes();
//
//   // Before evaluating graphs that use these nodes:
//   graph.registry().ctx().insert_or_assign(varfont::face_ctx{&myFace});
// ============================================================================

namespace varfont {

class VarFontFace;

/// Graph-registry ctx entry pointing at the face the axis nodes read/write.
struct face_ctx {
    VarFontFace* face {nullptr};
};

/// Register "Get Font Axis" / "Set Font Axis" in the global node type registry.
/// No-op when ofxImGuiNodeEditor is not on the include path.
void registerVariableFontNodes();

} // namespace varfont
