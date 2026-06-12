/*
 * media-player-cpp — VideoPanel
 * Copyright (c) vecnode 2026
 *
 * Dual-player prefetch keeps the next clip decoded in the standby slot so
 * cycleNext() can swap instantly instead of blocking on Media Foundation load().
 */

#include "VideoPanel.h"
#include "PlatformVideo.h"

#include <algorithm>
#include <system_error>

namespace {

namespace fs = of::filesystem;

bool isVideoPath(const fs::path& path) {
	static const char* kExtensions[] = {".mp4", ".mov", ".avi", ".mkv", ".webm"};
	const std::string ext = ofGetExtensionLower(path);
	return std::any_of(std::begin(kExtensions), std::end(kExtensions),
		[&](const char* candidate) { return ext == candidate; });
}

bool pathsEquivalent(const fs::path& a, const fs::path& b) {
	try {
		return fs::equivalent(a, b);
	} catch (...) {
		return fs::absolute(a) == fs::absolute(b);
	}
}

fs::path normalizeDir(fs::path path) {
	if (path.empty()) {
		return path;
	}
	std::error_code ec;
	path = fs::absolute(path, ec);
	if (ec) {
		return path;
	}
	path.make_preferred();
	if (!path.string().empty()) {
		const char last = path.string().back();
		if (last != '\\' && last != '/') {
			path /= "";
		}
	}
	return path;
}

std::string formatPathForLog(const fs::path& path) {
	fs::path copy = path;
	copy.make_preferred();
	return copy.string();
}

/// Search order: {exe}/data, {exe}/, then project bin/ paths in debug builds.
std::vector<fs::path> buildSearchRoots() {
	const fs::path exeDir = normalizeDir(ofFilePath::getCurrentExeDirFS());
	std::vector<fs::path> roots;

	roots.push_back(exeDir / "data");
	roots.push_back(exeDir);

#ifndef NDEBUG
	// When running via `make RunRelease` the cwd is often the project root.
	roots.push_back(normalizeDir(fs::path("bin") / "data"));
	roots.push_back(normalizeDir("bin"));
#endif

	return roots;
}

std::vector<fs::path> existingUniqueRoots(const std::vector<fs::path>& candidates) {
	std::vector<fs::path> roots;

	for (auto root : candidates) {
		root = normalizeDir(root);
		std::error_code ec;
		if (root.empty() || !fs::exists(root, ec) || !fs::is_directory(root, ec)) {
			continue;
		}

		const bool alreadyListed = std::any_of(roots.begin(), roots.end(),
			[&](const fs::path& existing) { return pathsEquivalent(existing, root); });

		if (!alreadyListed) {
			roots.push_back(root);
		}
	}

	return roots;
}

} // namespace

void VideoPanel::scanDataFolder() {
	videoPaths.clear();
	searchLog.clear();

	const auto roots = existingUniqueRoots(buildSearchRoots());

	if (roots.empty()) {
		searchLog = "no folders found next to " + formatPathForLog(ofFilePath::getCurrentExeDirFS());
		ofLogError("VideoPanel") << searchLog;
		return;
	}

	for (const auto& root : roots) {
		std::size_t foundInRoot = 0;
		std::error_code ec;

		for (const auto& entry : fs::directory_iterator(root, ec)) {
			if (ec) {
				ofLogError("VideoPanel") << "directory_iterator failed for " << formatPathForLog(root)
					<< ": " << ec.message();
				break;
			}

			if (!entry.is_regular_file(ec)) {
				continue;
			}

			const fs::path filePath = entry.path();
			if (!isVideoPath(filePath)) {
				continue;
			}

			const std::string absPath = formatPathForLog(fs::absolute(filePath));
			const bool alreadyFound = std::any_of(videoPaths.begin(), videoPaths.end(),
				[&](const std::string& existing) {
					return pathsEquivalent(existing, absPath);
				});

			if (!alreadyFound) {
				videoPaths.push_back(absPath);
				++foundInRoot;
				ofLogNotice("VideoPanel") << "  found " << filePath.filename().string();
			}
		}

		searchLog += (searchLog.empty() ? "" : " | ")
			+ formatPathForLog(root) + " (" + ofToString(foundInRoot) + ")";
		ofLogNotice("VideoPanel") << "Searched " << formatPathForLog(root)
			<< " -> " << foundInRoot << " video(s)";
	}

	// Stable playlist order across runs (directory iteration order is not guaranteed).
	std::sort(videoPaths.begin(), videoPaths.end());

	ofLogNotice("VideoPanel") << "Total: " << videoPaths.size() << " video(s)";
}

void VideoPanel::ensurePlayerConfigured(ofVideoPlayer& player, bool& configuredFlag) {
	if (!configuredFlag) {
		PlatformVideo::configurePlayer(player);
		configuredFlag = true;
	}
}

void VideoPanel::invalidatePreload() {
	preloadedIndex = kInvalidPreloadIndex;
}

bool VideoPanel::isPreloadReady(std::size_t expectedIndex) const {
	if (preloadedIndex != expectedIndex) {
		return false;
	}

	const auto& standby = standbyPlayer();
	return standby.isLoaded()
		&& standby.getWidth() > 0
		&& standby.getHeight() > 0;
}

void VideoPanel::silenceStandby() {
	if (!standbyPlayer().isLoaded()) {
		return;
	}

	// Prefetch must not leak audio from the hidden player.
	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

bool VideoPanel::loadIntoPlayer(ofVideoPlayer& target, std::size_t index, bool logLoad) {
	if (index >= videoPaths.size()) {
		return false;
	}

	fs::path absPath = fs::path(videoPaths[index]);
	absPath.make_preferred();

	// Media Foundation load() closes the engine internally; avoid a redundant close() here.
	if (!target.load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPanel") << "Failed to load " << formatPathForLog(absPath);
		return false;
	}

	target.setLoopState(OF_LOOP_NORMAL);
	target.setVolume(1.0f);

	if (logLoad) {
		ofLogNotice("VideoPanel") << "Loaded " << absPath.filename().string()
			<< " (" << target.getWidth() << "x" << target.getHeight() << ")";
	}

	return true;
}

void VideoPanel::setup() {
	ensurePlayerConfigured(activePlayer(), playersConfigured[activeSlot]);

	scanDataFolder();

	if (videoPaths.empty()) {
		ofLogError("VideoPanel") << "No playable video found. Searched: " << searchLog;
		return;
	}

	if (!loadAtIndex(0)) {
		ofLogError("VideoPanel") << "Failed to load first video";
		return;
	}

	// Startup preview: hold frame 0 while paused (does not auto-play).
	PlatformVideo::primeFirstFrame(activePlayer());
}

bool VideoPanel::loadAtIndex(std::size_t index) {
	if (index >= videoPaths.size()) {
		return false;
	}

	ensurePlayerConfigured(activePlayer(), playersConfigured[activeSlot]);
	invalidatePreload();

	// Only advance playlist state after a successful load (avoids index/path mismatch on failure).
	if (!loadIntoPlayer(activePlayer(), index, true)) {
		loadedPath.clear();
		return false;
	}

	currentIndex = index;
	loadedPath = fs::path(videoPaths[index]).filename().string();
	return true;
}

void VideoPanel::preloadNextClip() {
	if (videoPaths.size() < 2 || !activePlayer().isLoaded()) {
		return;
	}

	if (preloadRetryCooldown > 0) {
		--preloadRetryCooldown;
		return;
	}

	const std::size_t nextIndex = (currentIndex + 1) % videoPaths.size();
	if (isPreloadReady(nextIndex)) {
		return;
	}

	ensurePlayerConfigured(standbyPlayer(), playersConfigured[standbySlot()]);

	if (!loadIntoPlayer(standbyPlayer(), nextIndex, false)) {
		invalidatePreload();
		preloadRetryCooldown = kPreloadRetryCooldownFrames;
		return;
	}

	preloadedIndex = nextIndex;
	silenceStandby();

	ofLogVerbose("VideoPanel") << "Prefetched " << fs::path(videoPaths[nextIndex]).filename().string();
}

void VideoPanel::swapToPrefetchedClip(std::size_t nextIndex) {
	activeSlot = standbySlot();
	currentIndex = nextIndex;
	loadedPath = fs::path(videoPaths[nextIndex]).filename().string();

	activePlayer().setVolume(1.0f);
	activePlayer().setLoopState(OF_LOOP_NORMAL);
	invalidatePreload();

	ofLogNotice("VideoPanel") << "Switched to " << loadedPath << " (prefetched)";
}

void VideoPanel::applyPlaybackStateAfterSwitch(bool wasPlaying) {
	if (wasPlaying) {
		// Preserve transport state: one Next click while playing keeps playing.
		activePlayer().setPaused(false);
		activePlayer().play();
	} else {
		PlatformVideo::primeFirstFrame(activePlayer());
	}
}

void VideoPanel::play() {
	if (!activePlayer().isLoaded()) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void VideoPanel::stop() {
	PlatformVideo::stopPlayback(activePlayer());
}

void VideoPanel::cycleNext() {
	if (videoPaths.empty()) {
		return;
	}

	const bool wasPlaying = activePlayer().isPlaying();
	const std::size_t nextIndex = (currentIndex + 1) % videoPaths.size();

	if (isPreloadReady(nextIndex)) {
		swapToPrefetchedClip(nextIndex);
	} else if (!loadAtIndex(nextIndex)) {
		// Prefetch not ready yet (e.g. first switch right after startup) — sync fallback.
		return;
	}

	applyPlaybackStateAfterSwitch(wasPlaying);
}

void VideoPanel::update() {
	// Pump both backends so MF async events and GPU textures stay current.
	if (activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (standbyPlayer().isLoaded()) {
		standbyPlayer().update();
	}

	preloadNextClip();
}

void VideoPanel::draw(const ofRectangle& bounds) const {
	if (!activePlayer().isLoaded()) {
		return;
	}

	// Prefer the backend's GPU texture (DXGI on Windows) over a CPU upload fallback.
	const ofTexture& tex = activePlayer().getTexture();
	if (tex.isAllocated()) {
		ofSetColor(255);
		tex.draw(bounds);
		return;
	}

	activePlayer().draw(bounds.x, bounds.y, bounds.width, bounds.height);
}
