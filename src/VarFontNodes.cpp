#include "VarFontNodes.h"
#include "VarFontFace.h"

#if defined(__has_include) && __has_include("NodeTypeRegistry.h") && __has_include("NodeGraph.h")
#  define OFX_VARFONT_HAS_NODEEDITOR 1
#  include "NodeTypeRegistry.h"
#  include "NodeGraph.h"
#  include "GraphEvaluator.h"
#  include "imgui.h"
#  include <algorithm>
#endif

namespace varfont {

#if defined(OFX_VARFONT_HAS_NODEEDITOR)

namespace {

using PT = inspector::PinDataType;

struct axis_binding {
    int axisIndex {0};
};

static VarFontFace* faceOf(entt::registry& reg)
{
    if (auto* ctx = reg.ctx().find<face_ctx>())
        return ctx->face;
    return nullptr;
}

static ofxne::PinDef inPin(const char* name, PT type)
{
    return {name, type, ed::PinKind::Input};
}

static ofxne::PinDef outPin(const char* name, PT type)
{
    return {name, type, ed::PinKind::Output};
}

static void drawAxisPicker(entt::entity node, entt::registry& reg, axis_binding& bind)
{
    (void)node;
    VarFontFace* face = faceOf(reg);
    ImGui::PushID("axis");
    if (!face || !face->isLoaded() || face->axes().empty()) {
        ImGui::TextDisabled(face ? "No axes" : "No face in ctx");
        ImGui::PopID();
        return;
    }
    const int n = static_cast<int>(face->axes().size());
    bind.axisIndex = std::clamp(bind.axisIndex, 0, n - 1);
    const auto& ax = face->axes()[static_cast<size_t>(bind.axisIndex)];
    // Fixed width — ImVec2(-1,0) stretches to the canvas and breaks node drag.
    if (ImGui::Button(ax.name.empty() ? ax.tagString().c_str() : ax.name.c_str(),
                      ImVec2(ImGui::CalcItemWidth(), 0))) {
        bind.axisIndex = (bind.axisIndex + 1) % n;
    }
    ImGui::TextDisabled("%.0f … %.0f", ax.minValue, ax.maxValue);
    ImGui::PopID();
}

} // namespace

void registerVariableFontNodes()
{
    ofxne::registerGlobalNodeType({
        "Get Font Axis", "Font",
        { outPin("Value", PT::Float) },
        120.f,
        [](entt::entity node, entt::registry& reg) {
            reg.emplace_or_replace<axis_binding>(node);
        },
        [](entt::entity node, entt::registry& reg) {
            auto& b = reg.get_or_emplace<axis_binding>(node);
            drawAxisPicker(node, reg, b);
        },
        nullptr,
        [](ofxne::EvalContext& ctx) {
            auto* b = ctx.reg.try_get<axis_binding>(ctx.node);
            VarFontFace* face = faceOf(ctx.reg);
            if (!b || !face || face->axes().empty()) {
                ctx.out(0, 0.f);
                return;
            }
            const int i = std::clamp(b->axisIndex, 0, static_cast<int>(face->axes().size()) - 1);
            ctx.out(0, face->axes()[static_cast<size_t>(i)].value);
        },
        [](entt::entity node, entt::registry& reg, ofJson& j) {
            if (auto* b = reg.try_get<axis_binding>(node)) j["axis"] = b->axisIndex;
        },
        [](entt::entity node, entt::registry& reg, const ofJson& j) {
            reg.get_or_emplace<axis_binding>(node).axisIndex = j.value("axis", 0);
        },
    });

    ofxne::registerGlobalNodeType({
        "Set Font Axis", "Font",
        { inPin("Value", PT::Float) },
        120.f,
        [](entt::entity node, entt::registry& reg) {
            reg.emplace_or_replace<axis_binding>(node);
        },
        [](entt::entity node, entt::registry& reg) {
            auto& b = reg.get_or_emplace<axis_binding>(node);
            drawAxisPicker(node, reg, b);
        },
        nullptr,
        [](ofxne::EvalContext& ctx) {
            auto* b = ctx.reg.try_get<axis_binding>(ctx.node);
            VarFontFace* face = faceOf(ctx.reg);
            if (!b || !face || face->axes().empty()) return;
            const int i = std::clamp(b->axisIndex, 0, static_cast<int>(face->axes().size()) - 1);
            face->axes()[static_cast<size_t>(i)].value = ctx.in<float>(0);
            face->applyAxes();
        },
        [](entt::entity node, entt::registry& reg, ofJson& j) {
            if (auto* b = reg.try_get<axis_binding>(node)) j["axis"] = b->axisIndex;
        },
        [](entt::entity node, entt::registry& reg, const ofJson& j) {
            reg.get_or_emplace<axis_binding>(node).axisIndex = j.value("axis", 0);
        },
    });
}

#else

void registerVariableFontNodes() {}

#endif

} // namespace varfont
