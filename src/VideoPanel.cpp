/*
 * media-player-cpp — VideoPanel
 * Copyright (c) vecnode 2026
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

	ofLogNotice("VideoPanel") << "Total: " << videoPaths.size() << " video(s)";
}

void VideoPanel::setup() {
	if (!playerConfigured) {
		PlatformVideo::configurePlayer(player);
		playerConfigured = true;
	}

	scanDataFolder();

	if (videoPaths.empty()) {
		ofLogError("VideoPanel") << "No playable video found. Searched: " << searchLog;
		return;
	}

	if (!loadAtIndex(0)) {
		ofLogError("VideoPanel") << "Failed to load first video";
		return;
	}

	PlatformVideo::primeFirstFrame(player);
}

bool VideoPanel::loadAtIndex(std::size_t index) {
	if (index >= videoPaths.size()) {
		return false;
	}

	currentIndex = index;
	fs::path absPath = fs::path(videoPaths[index]);
	absPath.make_preferred();

	player.close();
	PlatformVideo::configurePlayer(player);

	if (!player.load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPanel") << "Failed to load " << formatPathForLog(absPath);
		loadedPath.clear();
		return false;
	}

	loadedPath = absPath.filename().string();
	player.setLoopState(OF_LOOP_NORMAL);
	player.setVolume(1.0f);

	ofLogNotice("VideoPanel") << "Loaded " << loadedPath
		<< " (" << player.getWidth() << "x" << player.getHeight() << ")";

	return true;
}

void VideoPanel::play() {
	if (!player.isLoaded()) {
		return;
	}

	player.setPaused(false);
	player.play();
}

void VideoPanel::stop() {
	PlatformVideo::stopPlayback(player);
}

void VideoPanel::cycleNext() {
	if (videoPaths.empty()) {
		return;
	}

	const std::size_t nextIndex = (currentIndex + 1) % videoPaths.size();
	if (loadAtIndex(nextIndex)) {
		PlatformVideo::primeFirstFrame(player);
	}
}

void VideoPanel::update() {
	if (player.isLoaded()) {
		player.update();
	}
}

void VideoPanel::draw(const ofRectangle& bounds) const {
	if (!player.isLoaded()) {
		return;
	}

	player.draw(bounds.x, bounds.y, bounds.width, bounds.height);
}
