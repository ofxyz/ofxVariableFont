#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLFWWindowSettings settings;
    settings.setSize(1440, 900);
    settings.title = "Variable Font Viewer";
    settings.resizable = true;
    auto window = ofCreateWindow(settings);
    auto app = std::make_shared<ofApp>();
    ofRunApp(window, std::move(app));
    ofRunMainLoop();
}
