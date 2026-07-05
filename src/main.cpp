/*
 * media-player-cpp
 * Copyright (c) vecnode 2026
 *
 * Entry point: creates a fixed 1920x1080 GL window and runs ofApp.
 * Window size matches the 1080p/16:9 assets in bin/data (1:1 pixel draw, no
 * upscale). Not drag-resizable - press F11 (see ofApp::keyPressed) to
 * maximize/fullscreen instead.
 */

#include "ofMain.h"
#include "ofApp.h"

int main() {
	// GLFW-specific settings type (not the generic ofGLWindowSettings) - needed
	// for the `resizable` flag.
	ofGLFWWindowSettings settings;
	settings.setSize(1920, 1080);
	settings.windowMode = OF_WINDOW; // OF_FULLSCREEN for kiosk / install use
	settings.resizable = false;

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
