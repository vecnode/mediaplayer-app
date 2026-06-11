/*
 * media-player-cpp
 * Copyright (c) vecnode 2026
 */

#include "ofMain.h"
#include "ofApp.h"

int main() {
	// 1920x1080 matches the 1080p/16:9 assets in bin/data.
	ofGLWindowSettings settings;
	settings.setSize(1920, 1080);
	settings.windowMode = OF_WINDOW; // OF_FULLSCREEN for kiosk / install use

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
