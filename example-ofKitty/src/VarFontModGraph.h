#pragma once

#include "NodeGraph.h"
#include "ofxImGuiNodeEditor.h"
#include "VarFontFace.h"
#include <entt.hpp>
#include <string>

namespace ed = ax::NodeEditor;

/// ECS-backed modulator graph for variable-font axis parameters.
class VarFontModGraph {
public:
    explicit VarFontModGraph(entt::registry& registry);

    void setup();
    void exit();

    void setFace(varfont::VarFontFace* face);

    /// Evaluate float modulators and write connected axis sinks into face axes().
    void evaluate(float timeSec);

    void drawGraph();

    entt::entity addNode(const std::string& typeName, ImVec2 pos = {200.f, 200.f});
    void addAxisNodesForFace(ImVec2 startPos = {520.f, 60.f}, float rowStep = 70.f);
    void seedDemoGraph();

    ofxne::NodeGraph& graph() { return m_graph; }

private:
    entt::registry&   m_reg;
    ofxne::NodeGraph  m_graph;
    varfont::VarFontFace* m_face {nullptr};

    void registerNodeTypes();
    float pinValue(entt::entity pinEntity) const;
    float outputPinValue(entt::entity outPin) const;
    entt::entity findPin(entt::entity node, ed::PinKind kind, int localIndex) const;
    void linkPins(entt::entity fromOut, entt::entity toIn);

    /// When an Axis node changes target, retarget a linked Map Range's min/max.
    void syncMapRangeToAxis(entt::entity axisNode, const varfont::Axis& axis);
};
