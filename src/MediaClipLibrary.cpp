/*
 * media-player-cpp — MediaClipLibrary
 * Copyright (c) vecnode 2026
 */

#include "MediaClipLibrary.h"

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

bool isImagePath(const fs::path& path) {
	static const char* kExtensions[] = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp", ".tif", ".tiff"};
	const std::string ext = ofGetExtensionLower(path);
	return std::any_of(std::begin(kExtensions), std::end(kExtensions),
		[&](const char* candidate) { return ext == candidate; });
}

ClipMediaType mediaTypeForPath(const fs::path& path) {
	if (isVideoPath(path)) {
		return ClipMediaType::Video;
	}
	if (isImagePath(path)) {
		return ClipMediaType::Image;
	}
	return ClipMediaType::Image;
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

void collectMediaFiles(const fs::path& root, std::vector<MediaClip>& clips, std::size_t& foundInRoot) {
	std::error_code ec;

	for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
		if (ec) {
			ofLogError("MediaClipLibrary") << "scan failed for " << formatPathForLog(root)
				<< ": " << ec.message();
			break;
		}

		if (!entry.is_regular_file(ec)) {
			continue;
		}

		const fs::path filePath = entry.path();
		if (!isVideoPath(filePath) && !isImagePath(filePath)) {
			continue;
		}

		const std::string absPath = formatPathForLog(fs::absolute(filePath));
		const bool alreadyFound = std::any_of(clips.begin(), clips.end(),
			[&](const MediaClip& existing) {
				return pathsEquivalent(existing.absolutePath, absPath);
			});

		if (!alreadyFound) {
			MediaClip clip;
			clip.absolutePath = absPath;
			clip.displayName = filePath.filename().string();
			clip.mediaType = mediaTypeForPath(filePath);
			clips.push_back(std::move(clip));
			++foundInRoot;
			ofLogVerbose("MediaClipLibrary") << "  found " << filePath.filename().string();
		}
	}
}

} // namespace

const MediaClip& MediaClipLibrary::clipAt(std::size_t index) const {
	static const MediaClip kEmpty;
	if (index >= clips.size()) {
		return kEmpty;
	}
	return clips[index];
}

std::size_t MediaClipLibrary::nextIndex(std::size_t currentIndex) const {
	if (clips.empty()) {
		return 0;
	}
	return (currentIndex + 1) % clips.size();
}

std::size_t MediaClipLibrary::previousIndex(std::size_t currentIndex) const {
	if (clips.empty()) {
		return 0;
	}
	if (currentIndex == 0) {
		return clips.size() - 1;
	}
	return currentIndex - 1;
}

void MediaClipLibrary::scan() {
	clips.clear();
	searchLog.clear();

	const auto roots = existingUniqueRoots(buildSearchRoots());

	if (roots.empty()) {
		searchLog = "no folders found next to " + formatPathForLog(ofFilePath::getCurrentExeDirFS());
		ofLogError("MediaClipLibrary") << searchLog;
		return;
	}

	for (const auto& root : roots) {
		std::size_t foundInRoot = 0;
		collectMediaFiles(root, clips, foundInRoot);

		searchLog += (searchLog.empty() ? "" : " | ")
			+ formatPathForLog(root) + " (" + ofToString(foundInRoot) + ")";
		ofLogNotice("MediaClipLibrary") << "Searched " << formatPathForLog(root)
			<< " -> " << foundInRoot << " media file(s)";
	}

	std::sort(clips.begin(), clips.end(),
		[](const MediaClip& a, const MediaClip& b) {
			if (a.mediaType != b.mediaType) {
				return a.mediaType == ClipMediaType::Image;
			}
			return a.absolutePath < b.absolutePath;
		});

	const std::size_t imageCount = std::count_if(clips.begin(), clips.end(),
		[](const MediaClip& clip) { return clip.mediaType == ClipMediaType::Image; });

	ofLogNotice("MediaClipLibrary") << "Total: " << clips.size() << " media file(s) ("
		<< imageCount << " image(s), " << (clips.size() - imageCount) << " video(s))";
}
