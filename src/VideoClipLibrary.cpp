/*
 * media-player-cpp — VideoClipLibrary
 * Copyright (c) vecnode 2026
 */

#include "VideoClipLibrary.h"

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

std::vector<fs::path> buildSearchRoots() {
	const fs::path exeDir = normalizeDir(ofFilePath::getCurrentExeDirFS());
	std::vector<fs::path> roots;

	roots.push_back(exeDir / "data");
	roots.push_back(exeDir);

#ifndef NDEBUG
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

const VideoClip& VideoClipLibrary::clipAt(std::size_t index) const {
	static const VideoClip kEmpty;
	if (index >= clips.size()) {
		return kEmpty;
	}
	return clips[index];
}

std::size_t VideoClipLibrary::nextIndex(std::size_t currentIndex) const {
	if (clips.empty()) {
		return 0;
	}
	return (currentIndex + 1) % clips.size();
}

std::size_t VideoClipLibrary::previousIndex(std::size_t currentIndex) const {
	if (clips.empty()) {
		return 0;
	}
	if (currentIndex == 0) {
		return clips.size() - 1;
	}
	return currentIndex - 1;
}

void VideoClipLibrary::scan() {
	clips.clear();
	searchLog.clear();

	const auto roots = existingUniqueRoots(buildSearchRoots());

	if (roots.empty()) {
		searchLog = "no folders found next to " + formatPathForLog(ofFilePath::getCurrentExeDirFS());
		ofLogError("VideoClipLibrary") << searchLog;
		return;
	}

	for (const auto& root : roots) {
		std::size_t foundInRoot = 0;
		std::error_code ec;

		for (const auto& entry : fs::directory_iterator(root, ec)) {
			if (ec) {
				ofLogError("VideoClipLibrary") << "directory_iterator failed for " << formatPathForLog(root)
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
			const bool alreadyFound = std::any_of(clips.begin(), clips.end(),
				[&](const VideoClip& existing) {
					return pathsEquivalent(existing.absolutePath, absPath);
				});

			if (!alreadyFound) {
				VideoClip clip;
				clip.absolutePath = absPath;
				clip.displayName = filePath.filename().string();
				clips.push_back(std::move(clip));
				++foundInRoot;
				ofLogNotice("VideoClipLibrary") << "  found " << filePath.filename().string();
			}
		}

		searchLog += (searchLog.empty() ? "" : " | ")
			+ formatPathForLog(root) + " (" + ofToString(foundInRoot) + ")";
		ofLogNotice("VideoClipLibrary") << "Searched " << formatPathForLog(root)
			<< " -> " << foundInRoot << " video(s)";
	}

	std::sort(clips.begin(), clips.end(),
		[](const VideoClip& a, const VideoClip& b) {
			return a.absolutePath < b.absolutePath;
		});

	ofLogNotice("VideoClipLibrary") << "Total: " << clips.size() << " video(s)";
}
