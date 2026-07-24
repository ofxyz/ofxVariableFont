#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLFWWindowSettings settings;
    settings.setSize(1280, 800);
    settings.windowMode = OF_WINDOW;
    settings.resizable  = true;
    settings.title      = "Variable Font Modulators";
    auto window = ofCreateWindow(settings);

    auto app = std::make_shared<ofApp>();

    ofkitty::runtime().setAppName("Variable Font Modulators");
    ofkitty::Runtime::attach(window, app);

    ofRunApp(window, std::move(app));
    ofRunMainLoop();
}
