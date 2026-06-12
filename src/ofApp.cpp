/*
 * media-player-cpp — ofApp
 * Copyright (c) vecnode 2026
 */

#include "ofApp.h"
#include "PlatformVideo.h"

void ofApp::setup() {
	ofSetVerticalSync(false);
	ofSetFrameRate(0);
	ofBackground(0);

#ifdef TARGET_WIN32
	PlatformVideo::configureDataPathRoot();
#endif

	panelBounds.set(0, 0, ofGetWidth(), ofGetHeight());

	subtitles.setup();
	videoPanel.setup();

	controller.setup(&videoPanel, &subtitles);
	httpServer.setup(HttpControlServer::kDefaultPort, &controller);

	playButton.addListener(this, &ofApp::onPlayPressed);
	cycleButton.addListener(this, &ofApp::onCyclePressed);
	stopButton.addListener(this, &ofApp::onStopPressed);
	subtitlesToggle.addListener(this, &ofApp::onSubtitlesToggled);

	gui.setup("Controls");
	gui.add(playButton.setup("Play"));
	gui.add(cycleButton.setup("Next Video"));
	gui.add(stopButton.setup("Stop"));
	gui.add(subtitlesToggle.setup("Subtitles", false));
	gui.add(statusLabel.setup("clip", ""));

	refreshGuiFromController();
}

void ofApp::exit() {
	httpServer.shutdown();

	playButton.removeListener(this, &ofApp::onPlayPressed);
	cycleButton.removeListener(this, &ofApp::onCyclePressed);
	stopButton.removeListener(this, &ofApp::onStopPressed);
	subtitlesToggle.removeListener(this, &ofApp::onSubtitlesToggled);
}

void ofApp::onPlayPressed() {
	controller.play();
	refreshGuiFromController();
}

void ofApp::onCyclePressed() {
	controller.nextClip();
	refreshGuiFromController();
}

void ofApp::onStopPressed() {
	controller.stop();
	refreshGuiFromController();
}

void ofApp::onSubtitlesToggled(bool& value) {
	controller.setSubtitlesEnabled(value);
	refreshGuiFromController();
}

void ofApp::refreshGuiFromController() {
	subtitlesToggle = controller.isSubtitlesEnabled();

	const MediaPlayerStatus status = controller.getStatus();
	if (!status.loaded) {
		statusLabel = "no clip loaded";
		return;
	}

	const std::string transport = status.playing ? "playing" : "stopped";
	statusLabel = ofToString(status.clipIndex + 1) + "/"
		+ ofToString(status.clipCount) + "  "
		+ status.clipName + "  (" + transport + ")";
}

void ofApp::update() {
	httpServer.update();
	videoPanel.update();
}

void ofApp::draw() {
	ofSetColor(255);
	videoPanel.draw(panelBounds);
	subtitles.draw(panelBounds);

	if (!videoPanel.isLoaded()) {
		ofSetColor(255);
		const std::string msg = videoPanel.getClipCount() > 0
			? "Video(s) found but failed to load. Check console."
			: "No video found. Searched:";
		ofDrawBitmapStringHighlight(msg, 20, ofGetHeight() - 56);
		ofDrawBitmapStringHighlight(videoPanel.getSearchLog(), 20, ofGetHeight() - 40);
	}

	gui.draw();
}

void ofApp::windowResized(int w, int h) {
	panelBounds.set(0, 0, w, h);
}
