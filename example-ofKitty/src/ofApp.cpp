#include "ofApp.h"

#include <GLFW/glfw3.h>

using namespace ofkitty;

namespace {

static const char* kSampleTexts[] = {
    "The quick brown fox jumps over the lazy dog",
    "Variable Fonts\nWeight  Width  Slant  Custom axes",
    "ABCDEFGHIJKLM\nNOPQRSTUVWXYZ\nabcdefghijklm\nnopqrstuvwxyz",
    nullptr
};

} // namespace

void ofApp::initVarFontGpuRenderer() {
    if (!ImVarFont::InitRenderer(
            reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress))) {
        ofLogWarning("example-ofKitty") << "GPU renderer init failed; filled glyphs may be skipped";
    }
}

void ofApp::setup() {
    ofDisableArbTex();
    ofSetFrameRate(60);
    ofBackground(18, 18, 24);

    runtime().setAppName("Variable Font Modulators");
    runtime().setDataSubdir("ofKitty");
    // No Scene / Properties — this demo only needs Controls + Modulators.

    m_view = runtime().setMainView2D({1200.f, 800.f}, "px");
    m_view->builtinSelect2D = false;

    initVarFontGpuRenderer();

    m_modGraph = std::make_unique<VarFontModGraph>(runtime().registry());
    m_modGraph->setup();
    m_modGraph->graph().setDefaultBodyWidth(120.f);
    tryLoadFont(m_fontPath);

    registerGraphPrefs();
    registerControlsWindow();
    registerModulatorsWindow();
    registerFontMenu();
    applyGraphHostMode();
}

void ofApp::exit() {
    if (m_modGraph) m_modGraph->exit();
    ImVarFont::ShutdownRenderer();
}

void ofApp::tryLoadFont(const std::string& path) {
    m_face.unload();
    m_fontLoaded = false;
    if (path.empty()) return;

    m_fontPath = path;
    if (m_face.load(path)) {
        m_fontLoaded = true;
        m_face.applyAxes(m_extrapolate);
        m_modGraph->setFace(&m_face);
        {
            auto view = runtime().registry().view<ofxne::node_type_component>();
            if (view.begin() == view.end())
                m_modGraph->seedDemoGraph();
        }
    } else {
        ofLogError("example-ofKitty") << "Failed to load font: " << path;
    }
}

void ofApp::applyGraphHostMode() {
    if (!m_view) return;

    if (m_graphOverlay) {
        m_view->overlayDraw = [this](ofkitty::MainView2D& v) { drawNodeOverlay(v); };
        // Keep ofxKit specimen pan (MMB / Alt+LMB / double-click fit).
        // Only mute wheel so the node editor can own zoom.
        m_view->panOnMiddle         = true;
        m_view->panOnAltLMB         = true;
        m_view->allowWheel          = false;
        m_view->allowDoubleClickFit = true;
        runtime().setWindowVisible("Modulators", false);
    } else {
        m_view->overlayDraw = nullptr;
        m_view->panOnMiddle         = true;
        m_view->panOnAltLMB         = true;
        m_view->allowWheel          = true;
        m_view->allowDoubleClickFit = true;
        runtime().setWindowVisible("Modulators", true);
    }
}

void ofApp::registerGraphPrefs() {
    runtime().registerPrefSerializer(
        "ofKitty",
        [this](ofJson& j) {
            j["ofKitty"] = {{"graphOverlay", m_graphOverlay}};
        },
        [this](const ofJson& j) {
            if (!j.contains("ofKitty")) return;
            const auto& s = j["ofKitty"];
            if (s.contains("graphOverlay")) {
                m_graphOverlay = s["graphOverlay"].get<bool>();
                // loadAppPrefs runs after ofApp::setup — re-wire host.
                applyGraphHostMode();
            }
        });

    runtime().registerPreferencePage({
        "Variable Font",
        "Modulators",
        "ofKitty.prefs.modulators",
        [this] {
            bool overlay = m_graphOverlay;
            if (ImGui::Checkbox("Graph on canvas overlay", &overlay) &&
                overlay != m_graphOverlay) {
                m_graphOverlay = overlay;
                applyGraphHostMode();
                runtime().persistAppPrefs();
            }
            ImGui::TextDisabled(
                "Overlay: patch over the specimen. Docked: Modulators panel; "
                "font canvas keeps pan/zoom.");
        }});
}

void ofApp::registerControlsWindow() {
    runtime().registerWindow({
        "Controls", "View", true, false,
        [this](bool& visible) {
            if (!ImGui::Begin("Controls", &visible)) {
                ImGui::End();
                return;
            }

            if (ImGui::Button("Browse font...", ImVec2(-1.f, 0.f))) {
                ofFileDialogResult r = ofSystemLoadDialog("Load font", false, "");
                if (r.bSuccess) tryLoadFont(r.getPath());
            }

            if (m_fontLoaded)
                ImGui::TextColored({0.4f, 0.9f, 0.5f, 1.f}, "%s  %s",
                                   m_face.familyName().c_str(), m_face.styleName().c_str());
            else
                ImGui::TextDisabled("No font loaded");

            if (ImGui::BeginCombo("Sample", "Choose specimen…")) {
                for (int i = 0; kSampleTexts[i]; ++i) {
                    if (ImGui::Selectable(kSampleTexts[i]))
                        strncpy(m_textBuf, kSampleTexts[i], sizeof(m_textBuf) - 1);
                }
                ImGui::EndCombo();
            }

            ImGui::InputTextMultiline("##text", m_textBuf, sizeof(m_textBuf), ImVec2(-1.f, 80.f));
            ImGui::SliderFloat("Size (px)", &m_emPx, 12.f, 400.f, "%.0f");
            ImGui::SliderFloat("Line height", &m_lineHeightMult, 0.5f, 3.f, "%.2f×");
            ImGui::SliderFloat("Letter spacing", &m_letterSpacingEm, -0.2f, 0.5f, "%.3f em");
            ImGui::Checkbox("Extrapolate beyond limits", &m_extrapolate);

            if (m_fontLoaded && m_face.isVariable()) {
                ImGui::SeparatorText("Manual axes");
                ImVarFont::Face* ivf = m_face.imVarFace();
                if (ivf && ImVarFont::AxisSliders(ivf, "##manual_axes", m_extrapolate))
                    m_face.pullAxesFromImVar();
            }

            ImGui::Separator();
            bool overlay = m_graphOverlay;
            if (ImGui::Checkbox("Graph overlay on canvas", &overlay) &&
                overlay != m_graphOverlay) {
                m_graphOverlay = overlay;
                applyGraphHostMode();
                runtime().persistAppPrefs();
            }
            ImGui::TextDisabled(
                m_graphOverlay
                    ? "RMB empty canvas: add · RMB node/link: delete · Del when selected\n"
                      "MMB / Alt+LMB: pan specimen · wheel: zoom graph"
                    : "Open View ▸ Modulators · RMB to add/delete · Del when selected");
            ImGui::End();
        }
    });
    runtime().addDefaultLayoutLeftDock("Controls");
}

void ofApp::registerModulatorsWindow() {
    runtime().registerWindow({
        "Modulators", "View", !m_graphOverlay, false,
        [this](bool& visible) {
            // Never draw the graph twice — overlay mode owns the canvas host.
            if (m_graphOverlay) {
                visible = false;
                return;
            }
            if (!ImGui::Begin("Modulators", &visible)) {
                ImGui::End();
                return;
            }
            if (m_modGraph)
                m_modGraph->drawGraph();
            ImGui::End();
        }
    });
    runtime().addDefaultLayoutRightDock("Modulators");
}

void ofApp::drawAddNodeMenuItems() {
    if (!m_modGraph) return;
    if (ImGui::BeginMenu("Add node")) {
        std::string lastCat;
        for (const auto& t : m_modGraph->graph().nodeTypes()) {
            if (t.category != lastCat) {
                if (!lastCat.empty()) ImGui::Separator();
                ImGui::TextDisabled("%s", t.category.c_str());
                lastCat = t.category;
            }
            if (ImGui::MenuItem(t.name.c_str()))
                m_modGraph->addNode(t.name);
        }
        ImGui::EndMenu();
    }
}

void ofApp::registerFontMenu() {
    runtime().addMenuBarGroup("Font", [this] {
        if (ImGui::MenuItem("Load font…")) {
            ofFileDialogResult r = ofSystemLoadDialog("Load font", false, "");
            if (r.bSuccess) tryLoadFont(r.getPath());
        }
        if (ImGui::MenuItem("Reset axes") && m_face.imVarFace()) {
            ImVarFont::ResetAxes(m_face.imVarFace());
            m_face.pullAxesFromImVar();
        }
        ImGui::Separator();
        drawAddNodeMenuItems();
        if (ImGui::MenuItem("Add axis nodes for all axes"))
            m_modGraph->addAxisNodesForFace();
        if (ImGui::MenuItem("Seed demo graph (Sine → Map → wght)"))
            m_modGraph->seedDemoGraph();
        ImGui::Separator();
        if (ImGui::MenuItem("Graph on canvas overlay", nullptr, m_graphOverlay)) {
            m_graphOverlay = !m_graphOverlay;
            applyGraphHostMode();
            runtime().persistAppPrefs();
        }
    });
}

void ofApp::update() {
    if (!m_fontLoaded) return;
    m_modGraph->evaluate(ofGetElapsedTimef());
    m_face.applyAxes(m_extrapolate);
}

void ofApp::draw() {
    if (!m_view || !m_fontLoaded) return;

    auto& v = m_view->view2D;
    v.updateDerived();

    varfont::StringLayoutOptions opts;
    opts.lineHeightMult     = m_lineHeightMult;
    opts.letterSpacingEm    = m_letterSpacingEm;
    opts.allowExtrapolation = m_extrapolate;

    const ofRectangle bounds = m_face.getStringBoundsMM(m_textBuf, m_emPx, opts);
    const float cx = v.contentSize.x * 0.5f;
    const float cy = v.contentSize.y * 0.5f;
    const float px = cx - bounds.getWidth() * 0.5f - bounds.x;
    const float py = cy - bounds.getHeight() * 0.5f - bounds.y;

    ofPushMatrix();
    ofTranslate(v.ox, v.oy);
    ofScale(v.zoom_, v.zoom_);
    // GPU coverage atlas (varfont_gl) — same path as example/; morph axes update
    // each frame via applyAxes so graph sinks show on the canvas.
    m_face.drawStringGpu(m_textBuf, px, py, m_emPx, ofColor(240, 240, 245), opts);
    ofPopMatrix();
}

void ofApp::drawNodeOverlay(ofkitty::MainView2D& view) {
    // Kit's ##ofkitty_main_view2d_overlay is already the host window — draw the
    // graph directly into the canvas content region (no nested Begin).
    (void)view;
    if (m_modGraph)
        m_modGraph->drawGraph();
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;
    tryLoadFont(dragInfo.files[0].string());
}
