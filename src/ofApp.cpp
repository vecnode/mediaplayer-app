/*
 * media-player-cpp — ofApp
 * Copyright (c) vecnode 2026
 */

#include "ofApp.h"
#include "PlatformVideo.h"

ofRectangle ofApp::mediaBoundsForWindow(int w, int h) {
	// Media fills the entire window/display - no scale-down, no centering.
	// The window itself is fixed 1080p (16:9), so this stays 16:9 whether
	// windowed or fullscreen (F11).
	return ofRectangle(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
}

void ofApp::setup() {
	// Uncapped loop: VSync + frame caps caused stutter with MF on this stack.
	ofSetVerticalSync(false);
	ofSetFrameRate(0);
	ofBackground(0);

#ifdef TARGET_WIN32
	PlatformVideo::configureDataPathRoot();
#endif

	panelBounds = mediaBoundsForWindow(ofGetWidth(), ofGetHeight());

	subtitles.setup();
	mediaPanel.setup();

	controller.setup(&mediaPanel, &subtitles);
	httpServer.setup(HttpControlServer::kDefaultPort, &controller);

	cycleButton.addListener(this, &ofApp::onCyclePressed);
	randomButton.addListener(this, &ofApp::onRandomPressed);
	subtitlesToggle.addListener(this, &ofApp::onSubtitlesToggled);
	regionBBoxToggle.addListener(this, &ofApp::onRegionBBoxToggled);
	regionFocusToggle.addListener(this, &ofApp::onRegionFocusToggled);
	regionPanToggle.addListener(this, &ofApp::onRegionPanToggled);
	animateToggle.addListener(this, &ofApp::onAnimateToggled);

	gui.setup("Controls");
	gui.add(cycleButton.setup("Next"));
	gui.add(randomButton.setup("Random"));
	gui.add(subtitlesToggle.setup("Subtitles", true));
	gui.add(regionBBoxToggle.setup("Region bbox", true));
	gui.add(regionFocusToggle.setup("Region zoom", false));
	gui.add(regionPanToggle.setup("Region pan", true));
	gui.add(animateToggle.setup("Animate", true));
	gui.add(statusLabel.setup("clip", ""));

	refreshGuiFromController();
}

void ofApp::exit() {
	httpServer.shutdown();

	cycleButton.removeListener(this, &ofApp::onCyclePressed);
	randomButton.removeListener(this, &ofApp::onRandomPressed);
	subtitlesToggle.removeListener(this, &ofApp::onSubtitlesToggled);
	regionBBoxToggle.removeListener(this, &ofApp::onRegionBBoxToggled);
	regionFocusToggle.removeListener(this, &ofApp::onRegionFocusToggled);
	regionPanToggle.removeListener(this, &ofApp::onRegionPanToggled);
	animateToggle.removeListener(this, &ofApp::onAnimateToggled);
}

void ofApp::onCyclePressed() {
	controller.nextClip();
	refreshGuiFromController();
}

void ofApp::onRandomPressed() {
	controller.randomClip();
	refreshGuiFromController();
}

void ofApp::onSubtitlesToggled(bool& value) {
	controller.setSubtitlesEnabled(value);
	refreshGuiFromController();
}

void ofApp::onRegionBBoxToggled(bool& value) {
	controller.setShowRegionBBox(value);
}

void ofApp::onRegionFocusToggled(bool& value) {
	controller.setRegionFocusEnabled(value);
	regionPanToggle = controller.regionPanEnabled();
}

void ofApp::onRegionPanToggled(bool& value) {
	controller.setRegionPanEnabled(value);
	regionFocusToggle = controller.regionFocusEnabled();
}

void ofApp::onAnimateToggled(bool& value) {
	controller.setAnimationsEnabled(value);
}

void ofApp::refreshGuiFromController() {
	subtitlesToggle = controller.isSubtitlesEnabled();
	regionBBoxToggle = controller.showRegionBBox();
	regionFocusToggle = controller.regionFocusEnabled();
	regionPanToggle = controller.regionPanEnabled();
	animateToggle = controller.animationsEnabled();

	const MediaPlayerStatus status = controller.getStatus();

	if (!status.loaded) {
		statusLabel = "no clip loaded";
		return;
	}

	const std::string state = status.isImage ? "image" : (status.playing ? "playing" : "stopped");
	statusLabel = ofToString(status.clipIndex + 1) + "/"
		+ ofToString(status.clipCount) + "  "
		+ status.clipName + "  (" + state + ")";
}

void ofApp::update() {
	httpServer.update();
	mediaPanel.update();
}

void ofApp::draw() {
	ofSetColor(255);
	mediaPanel.draw(panelBounds);
	subtitles.draw(ofRectangle(0, 0, static_cast<float>(ofGetWidth()), static_cast<float>(ofGetHeight())));

	if (!mediaPanel.isLoaded()) {
		ofSetColor(255);
		const std::string msg = mediaPanel.getClipCount() > 0
			? "Media found but failed to load. Check console."
			: "No media found. Put images/videos in bin/data. Searched:";
		ofDrawBitmapStringHighlight(msg, 20, ofGetHeight() - 56);
		ofDrawBitmapStringHighlight(mediaPanel.getSearchLog(), 20, ofGetHeight() - 40);
	}

	if (guiVisible_) {
		gui.draw();
	}
}

void ofApp::keyPressed(int key) {
	if (key == 'g' || key == 'G') {
		guiVisible_ = !guiVisible_;
	}
	if (key == OF_KEY_F11) {
		// The window isn't drag-resizable (see main.cpp); this is the
		// "maximize" action instead - toggles true OS fullscreen.
		ofToggleFullscreen();
	}
}

void ofApp::windowResized(int w, int h) {
	panelBounds = mediaBoundsForWindow(w, h);
}
