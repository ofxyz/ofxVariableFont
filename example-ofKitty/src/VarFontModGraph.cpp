#include "VarFontModGraph.h"

#include "ofxImGuiNodeEditor.h"
#include "PinDataTypes.h"

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {

struct vf_sine_component {
    float speed = 2.f;
    float out   = 0.f;
};

struct vf_constant_component {
    float value = 0.5f;
};

struct vf_map_range_component {
    float minVal = 0.f;
    float maxVal = 1.f;
    float out    = 0.f;
};

struct vf_axis_sink_component {
    int         axisIndex = 0;
    std::string axisTag;   ///< e.g. "wght" — shown in node title
};

struct vf_pin_default_component {
    float value = 0.f;
};

static const char* kTypeSine     = "Sine";
static const char* kTypeConstant = "Constant";
static const char* kTypeMapRange = "Map Range";
static const char* kTypeAxis     = "Axis";

} // namespace

VarFontModGraph::VarFontModGraph(entt::registry& registry)
    : m_reg(registry)
    , m_graph(registry)
{
}

void VarFontModGraph::setup()
{
    registerNodeTypes();
    m_graph.setup();
}

void VarFontModGraph::exit()
{
    m_graph.exit();
}

void VarFontModGraph::setFace(varfont::VarFontFace* face)
{
    m_face = face;
}

void VarFontModGraph::registerNodeTypes()
{
    using PD = inspector::PinDataType;
    namespace ed = ax::NodeEditor;

    {
        ofxne::NodeTypeDef def;
        def.name = kTypeSine;
        def.category = "Modulators";
        def.pins = {
            {"speed", PD::Float, ed::PinKind::Input},
            {"out",   PD::Float, ed::PinKind::Output},
        };
        def.onCreate = [](entt::entity e, entt::registry& r) {
            r.emplace<vf_sine_component>(e);
        };
        def.onDraw = [](entt::entity e, entt::registry& r) {
            auto& s = r.get<vf_sine_component>(e);
            // Hidden id includes entity — belt-and-suspenders with NodeGraph PushID.
            char id[32];
            std::snprintf(id, sizeof(id), "##speed_%u",
                          static_cast<unsigned>(static_cast<uint32_t>(e)));
            ImGui::DragFloat(id, &s.speed, 0.02f, 0.05f, 16.f, "speed %.2f");
            ImGui::TextDisabled("out  %.3f", s.out);
        };
        m_graph.registerNodeType(std::move(def));
    }

    {
        ofxne::NodeTypeDef def;
        def.name = kTypeConstant;
        def.category = "Modulators";
        def.pins = { {"value", PD::Float, ed::PinKind::Output} };
        def.onCreate = [](entt::entity e, entt::registry& r) {
            r.emplace<vf_constant_component>(e);
        };
        def.onDraw = [](entt::entity e, entt::registry& r) {
            auto& c = r.get<vf_constant_component>(e);
            char id[32];
            std::snprintf(id, sizeof(id), "##value_%u",
                          static_cast<unsigned>(static_cast<uint32_t>(e)));
            ImGui::DragFloat(id, &c.value, 0.005f, 0.f, 1.f, "value %.3f");
        };
        m_graph.registerNodeType(std::move(def));
    }

    {
        ofxne::NodeTypeDef def;
        def.name = kTypeMapRange;
        def.category = "Modulators";
        def.pins = {
            {"in",  PD::Float, ed::PinKind::Input},
            {"min", PD::Float, ed::PinKind::Input},
            {"max", PD::Float, ed::PinKind::Input},
            {"out", PD::Float, ed::PinKind::Output},
        };
        def.onCreate = [](entt::entity e, entt::registry& r) {
            auto& m = r.emplace<vf_map_range_component>(e);
            m.minVal = 0.f;
            m.maxVal = 1.f;
        };
        def.onDraw = [](entt::entity e, entt::registry& r) {
            auto& m = r.get<vf_map_range_component>(e);
            const unsigned eid = static_cast<unsigned>(static_cast<uint32_t>(e));
            char idMin[32], idMax[32];
            std::snprintf(idMin, sizeof(idMin), "##min_%u", eid);
            std::snprintf(idMax, sizeof(idMax), "##max_%u", eid);
            ImGui::DragFloat(idMin, &m.minVal, 1.f, -1.e5f, 1.e5f, "min %.1f");
            ImGui::DragFloat(idMax, &m.maxVal, 1.f, -1.e5f, 1.e5f, "max %.1f");
            ImGui::TextDisabled("out  %.1f", m.out);
        };
        m_graph.registerNodeType(std::move(def));
    }

    {
        ofxne::NodeTypeDef def;
        def.name = kTypeAxis;
        def.category = "Font";
        def.pins = { {"value", PD::Float, ed::PinKind::Input} };
        def.onCreate = [](entt::entity e, entt::registry& r) {
            r.emplace<vf_axis_sink_component>(e);
        };
        def.onDraw = [this](entt::entity e, entt::registry& r) {
            auto& ax = r.get<vf_axis_sink_component>(e);
            const int nAxes = (m_face && m_face->isLoaded()) ? (int)m_face->axes().size() : 0;
            if (nAxes <= 0) {
                ImGui::TextDisabled("No font axes");
                return;
            }
            ax.axisIndex = std::clamp(ax.axisIndex, 0, nAxes - 1);

            // No BeginCombo / BeginChild — popups and child windows break inside
            // ed::BeginNode (canvas local-space). Cycle with buttons instead.
            const unsigned eid = static_cast<unsigned>(static_cast<uint32_t>(e));
            char idPrev[32], idNext[32];
            std::snprintf(idPrev, sizeof(idPrev), "##ax_prev_%u", eid);
            std::snprintf(idNext, sizeof(idNext), "##ax_next_%u", eid);

            bool changed = false;
            if (ImGui::ArrowButton(idPrev, ImGuiDir_Left)) {
                ax.axisIndex = (ax.axisIndex + nAxes - 1) % nAxes;
                changed = true;
            }
            ImGui::SameLine();
            {
                const auto& cur = m_face->axes()[(size_t)ax.axisIndex];
                const std::string label = cur.tagString().empty()
                    ? cur.name
                    : (cur.tagString() + " — " + cur.name);
                ImGui::TextUnformatted(label.c_str());
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton(idNext, ImGuiDir_Right)) {
                ax.axisIndex = (ax.axisIndex + 1) % nAxes;
                changed = true;
            }

            const auto& a = m_face->axes()[(size_t)ax.axisIndex];
            ax.axisTag = a.tagString();
            if (auto* ntc = r.try_get<ofxne::node_type_component>(e))
                ntc->title = ax.axisTag.empty()
                    ? kTypeAxis
                    : (std::string(kTypeAxis) + " · " + ax.axisTag);
            ImGui::TextDisabled("%.0f … %.0f", a.minValue, a.maxValue);

            // Retarget linked Map Range so modulation stays in the new axis's
            // design space (wrong min/max + SCAN extremes → "font disappeared").
            if (changed)
                syncMapRangeToAxis(e, a);
        };
        m_graph.registerNodeType(std::move(def));
    }
}

entt::entity VarFontModGraph::findPin(entt::entity node, ed::PinKind kind, int localIndex) const
{
    auto view = m_reg.view<ofxne::pin_component>();
    for (auto pe : view) {
        const auto& pc = view.get<ofxne::pin_component>(pe);
        if (pc.ownerNode == node && pc.kind == kind && pc.localIndex == localIndex)
            return pe;
    }
    return entt::null;
}

void VarFontModGraph::linkPins(entt::entity fromOut, entt::entity toIn)
{
    if (!m_reg.valid(fromOut) || !m_reg.valid(toIn)) return;
    entt::entity link = m_reg.create();
    m_reg.emplace<ofxne::link_component>(link, fromOut, toIn);
}

void VarFontModGraph::syncMapRangeToAxis(entt::entity axisNode, const varfont::Axis& axis)
{
    const entt::entity inPin = findPin(axisNode, ed::PinKind::Input, 0);
    if (inPin == entt::null) return;

    auto linkView = m_reg.view<ofxne::link_component>();
    for (auto le : linkView) {
        const auto& lc = linkView.get<ofxne::link_component>(le);
        if (lc.toPin != inPin) continue;

        const auto* from = m_reg.try_get<ofxne::pin_component>(lc.fromPin);
        if (!from) continue;
        auto* map = m_reg.try_get<vf_map_range_component>(from->ownerNode);
        if (!map) continue;

        map->minVal = axis.minValue;
        map->maxVal = axis.maxValue;
        if (map->maxVal <= map->minVal)
            map->maxVal = map->minVal + 1.f;
    }
}

entt::entity VarFontModGraph::addNode(const std::string& typeName, ImVec2 pos)
{
    return m_graph.addNode(typeName, pos);
}

void VarFontModGraph::addAxisNodesForFace(ImVec2 startPos, float rowStep)
{
    if (!m_face || !m_face->isLoaded() || !m_face->isVariable()) return;

    const auto& axes = m_face->axes();
    for (size_t i = 0; i < axes.size(); ++i) {
        entt::entity node = addNode(kTypeAxis, {startPos.x, startPos.y + rowStep * (float)i});
        if (auto* ax = m_reg.try_get<vf_axis_sink_component>(node)) {
            ax->axisIndex = (int)i;
            ax->axisTag   = axes[i].tagString();
            if (auto* ntc = m_reg.try_get<ofxne::node_type_component>(node))
                ntc->title = std::string(kTypeAxis) + " · " + ax->axisTag;
        }
    }
}

void VarFontModGraph::seedDemoGraph()
{
    if (!m_face || !m_face->isLoaded() || !m_face->isVariable()) return;
    if (m_face->axes().empty()) return;

    entt::entity sine = addNode(kTypeSine, {80.f, 80.f});
    entt::entity map  = addNode(kTypeMapRange, {320.f, 80.f});
    entt::entity axis = addNode(kTypeAxis, {560.f, 80.f});

    // Prefer 'wght'; otherwise the first axis (Sixtyfour is BLED/SCAN, not wght).
    int axisIdx = 0;
    for (size_t i = 0; i < m_face->axes().size(); ++i) {
        if (m_face->axes()[i].tagString() == "wght") {
            axisIdx = (int)i;
            break;
        }
    }
    const auto& chosen = m_face->axes()[(size_t)axisIdx];

    if (auto* m = m_reg.try_get<vf_map_range_component>(map)) {
        m->minVal = chosen.minValue;
        m->maxVal = chosen.maxValue;
        if (m->maxVal <= m->minVal) {
            m->minVal = chosen.minValue;
            m->maxVal = chosen.minValue + 1.f;
        }
    }
    if (auto* ax = m_reg.try_get<vf_axis_sink_component>(axis)) {
        ax->axisIndex = axisIdx;
        ax->axisTag   = chosen.tagString();
        if (auto* ntc = m_reg.try_get<ofxne::node_type_component>(axis))
            ntc->title = std::string(kTypeAxis) + " · " + ax->axisTag;
    }

    linkPins(findPin(sine, ed::PinKind::Output, 0), findPin(map, ed::PinKind::Input, 0));
    linkPins(findPin(map, ed::PinKind::Output, 0), findPin(axis, ed::PinKind::Input, 0));
}

float VarFontModGraph::pinValue(entt::entity pinEntity) const
{
    if (!m_reg.valid(pinEntity)) return 0.f;

    auto linkView = m_reg.view<ofxne::link_component>();
    for (auto le : linkView) {
        const auto& lc = linkView.get<ofxne::link_component>(le);
        if (lc.toPin == pinEntity)
            return outputPinValue(lc.fromPin);
    }

    if (auto* d = m_reg.try_get<vf_pin_default_component>(pinEntity))
        return d->value;

    const auto* pc = m_reg.try_get<ofxne::pin_component>(pinEntity);
    if (!pc) return 0.f;

    entt::entity node = pc->ownerNode;
    if (auto* m = m_reg.try_get<vf_map_range_component>(node)) {
        if (pc->localIndex == 1) return m->minVal;
        if (pc->localIndex == 2) return m->maxVal;
    }
    if (auto* s = m_reg.try_get<vf_sine_component>(node)) {
        if (pc->kind == ed::PinKind::Input && pc->localIndex == 0)
            return s->speed;
    }
    return 0.f;
}

float VarFontModGraph::outputPinValue(entt::entity outPin) const
{
    const auto* pc = m_reg.try_get<ofxne::pin_component>(outPin);
    if (!pc || pc->kind != ed::PinKind::Output) return 0.f;

    entt::entity node = pc->ownerNode;
    const auto* ntc = m_reg.try_get<ofxne::node_type_component>(node);
    if (!ntc) return 0.f;

    if (ntc->typeName == kTypeConstant) {
        if (auto* c = m_reg.try_get<vf_constant_component>(node))
            return c->value;
    }
    if (ntc->typeName == kTypeSine) {
        if (auto* s = m_reg.try_get<vf_sine_component>(node))
            return s->out;
    }
    if (ntc->typeName == kTypeMapRange) {
        if (auto* m = m_reg.try_get<vf_map_range_component>(node))
            return m->out;
    }
    return 0.f;
}

void VarFontModGraph::evaluate(float timeSec)
{
    // Fixed stages so Map always sees this-frame Sine output (entt view order
    // is not dependency order).
    auto nodeView = m_reg.view<ofxne::node_type_component>();
    std::vector<entt::entity> sines, maps, axesNodes, constants;
    for (auto ne : nodeView) {
        const auto& ntc = nodeView.get<ofxne::node_type_component>(ne);
        if (ntc.typeName == kTypeSine)          sines.push_back(ne);
        else if (ntc.typeName == kTypeMapRange) maps.push_back(ne);
        else if (ntc.typeName == kTypeAxis)     axesNodes.push_back(ne);
        else if (ntc.typeName == kTypeConstant) constants.push_back(ne);
    }

    for (auto ne : sines) {
        auto& s = m_reg.get<vf_sine_component>(ne);
        const float speed = pinValue(findPin(ne, ed::PinKind::Input, 0));
        if (speed > 0.f) s.speed = speed;
        s.out = std::sin(timeSec * s.speed) * 0.5f + 0.5f;
    }
    for (auto ne : maps) {
        auto& m = m_reg.get<vf_map_range_component>(ne);
        const float in = pinValue(findPin(ne, ed::PinKind::Input, 0));
        const float mn = pinValue(findPin(ne, ed::PinKind::Input, 1));
        const float mx = pinValue(findPin(ne, ed::PinKind::Input, 2));
        m.minVal = mn;
        m.maxVal = mx;
        const float t = std::clamp(in, 0.f, 1.f);
        m.out = mn + t * (mx - mn);
    }

    if (!m_face || !m_face->isLoaded()) return;

    // Reset every axis to its default first. Otherwise switching an Axis node
    // off SCAN/BLED leaves that axis stuck at an extreme and the specimen
    // can vanish (Sixtyfour SCAN at max is effectively invisible).
    auto& axes = m_face->axes();
    for (auto& ax : axes)
        ax.value = ax.defValue;

    for (auto ne : axesNodes) {
        auto& axc = m_reg.get<vf_axis_sink_component>(ne);
        if (axc.axisIndex < 0 || axc.axisIndex >= (int)axes.size()) continue;

        const float v = pinValue(findPin(ne, ed::PinKind::Input, 0));
        auto& ax = axes[(size_t)axc.axisIndex];
        ax.value = std::clamp(v, ax.minValue, ax.maxValue);
    }
    (void)constants;
}

void VarFontModGraph::drawGraph()
{
    m_graph.drawGraph();
}
