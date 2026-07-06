/*
 * media-player-cpp — MediaCorpusProvider
 * Copyright (c) vecnode 2026
 */

#include "MediaCorpusProvider.h"

#include <algorithm>
#include <limits>
#include <vector>

void MediaCorpusProvider::setDataDirectory(const std::string& dataDirectory) {
	dataDirectory_ = dataDirectory;
	loadAttempted_ = false;
	loaded_ = false;
	corpus_ = metaagent::media::MediaCorpus{};
}

void MediaCorpusProvider::ensureLoaded() const {
	if (loadAttempted_) {
		return;
	}
	loadAttempted_ = true;

	if (dataDirectory_.empty()) {
		return;
	}

	loaded_ = corpus_.load_from_directory(dataDirectory_);
	if (loaded_) {
		ofLogNotice("MediaCorpusProvider") << "Loaded " << corpus_.size()
			<< " corpus entries from " << dataDirectory_
			<< " (" << entriesWithRegionsCount() << " with OBJS regions in "
			<< dataDirectory_ << "/OBJS_TEXT.md)";
	} else {
		ofLogVerbose("MediaCorpusProvider") << "No corpus markdown in " << dataDirectory_
			<< " (expected PDF_TEXT.md and OBJS_TEXT.md)";
	}
}

bool MediaCorpusProvider::isLoaded() const {
	ensureLoaded();
	return loaded_;
}

std::size_t MediaCorpusProvider::entryCount() const {
	ensureLoaded();
	return corpus_.size();
}

const metaagent::media::ImageCorpusEntry* MediaCorpusProvider::findForClip(const std::string& clipPath) const {
	ensureLoaded();
	if (!loaded_) {
		return nullptr;
	}
	return corpus_.find_by_path(clipPath);
}

std::string MediaCorpusProvider::subtitlePreview(const std::string& clipPath) const {
	ensureLoaded();
	if (!loaded_) {
		return {};
	}
	return corpus_.subtitle_preview_for(metaagent::media::corpus_file_key_from_path(clipPath));
}

std::string MediaCorpusProvider::subtitleSummary(const std::string& clipPath) const {
	ensureLoaded();
	if (!loaded_) {
		return {};
	}
	return corpus_.summary_text_for(metaagent::media::corpus_file_key_from_path(clipPath));
}

std::string MediaCorpusProvider::subtitleFull(const std::string& clipPath) const {
	ensureLoaded();
	if (!loaded_) {
		return {};
	}
	return corpus_.subtitle_text_for(metaagent::media::corpus_file_key_from_path(clipPath));
}

std::size_t MediaCorpusProvider::pickRandomRegionIndex(const std::string& clipPath) const {
	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry) {
		return std::numeric_limits<std::size_t>::max();
	}

	std::vector<std::size_t> region_indices;
	region_indices.reserve(entry->text_regions.size());
	for (std::size_t i = 0; i < entry->text_regions.size(); ++i) {
		const auto& bbox = entry->text_regions[i].bbox;
		if (bbox.width > 0 && bbox.height > 0) {
			region_indices.push_back(i);
		}
	}

	if (region_indices.empty()) {
		return std::numeric_limits<std::size_t>::max();
	}

	return region_indices[static_cast<std::size_t>(
		ofRandom(static_cast<float>(region_indices.size())))];
}

bool MediaCorpusProvider::focusRectForRegion(
	const std::string& clipPath,
	const std::size_t regionIndex,
	const float viewportAspectWidthOverHeight,
	metaagent::media::IntRect& outRect) const {
	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry || viewportAspectWidthOverHeight <= 0.0f
		|| regionIndex == std::numeric_limits<std::size_t>::max()) {
		return false;
	}

	return corpus_.region_focus_crop_for_aspect(
		*entry, regionIndex, viewportAspectWidthOverHeight, outRect);
}

bool MediaCorpusProvider::regionBboxForClip(
	const std::string& clipPath,
	const std::size_t regionIndex,
	metaagent::media::IntRect& outBbox) const {
	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry || regionIndex == std::numeric_limits<std::size_t>::max()
		|| regionIndex >= entry->text_regions.size()) {
		return false;
	}

	const auto& bbox = entry->text_regions[regionIndex].bbox;
	if (bbox.width <= 0 || bbox.height <= 0) {
		return false;
	}

	outBbox = bbox;
	return true;
}

std::size_t MediaCorpusProvider::regionCountForClip(const std::string& clipPath) const {
	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry) {
		return 0;
	}

	std::size_t count = 0;
	for (const auto& region : entry->text_regions) {
		if (region.bbox.width > 0 && region.bbox.height > 0) {
			++count;
		}
	}
	return count;
}

std::size_t MediaCorpusProvider::entriesWithRegionsCount() const {
	ensureLoaded();
	if (!loaded_) {
		return 0;
	}
	return corpus_.countEntriesWithRegions();
}

std::vector<metaagent::media::IntRect> MediaCorpusProvider::emptyAreaRects(
	const std::string& clipPath, const std::size_t maxCount) const {
	std::vector<metaagent::media::IntRect> result;

	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry || entry->image_width <= 0 || entry->image_height <= 0) {
		return result;
	}

	constexpr int kGridCols = 4;
	constexpr int kGridRows = 6;
	constexpr int kMarginPx = 10;

	const int cellW = entry->image_width / kGridCols;
	const int cellH = entry->image_height / kGridRows;
	if (cellW <= 0 || cellH <= 0) {
		return result;
	}

	bool occupied[kGridRows][kGridCols] = {};
	for (const auto& region : entry->text_regions) {
		if (region.bbox.width <= 0 || region.bbox.height <= 0) {
			continue;
		}
		const int minCol = std::max(0, (region.bbox.x - kMarginPx) / cellW);
		const int maxCol = std::min(kGridCols - 1, (region.bbox.x + region.bbox.width + kMarginPx) / cellW);
		const int minRow = std::max(0, (region.bbox.y - kMarginPx) / cellH);
		const int maxRow = std::min(kGridRows - 1, (region.bbox.y + region.bbox.height + kMarginPx) / cellH);
		for (int r = minRow; r <= maxRow; ++r) {
			for (int c = minCol; c <= maxCol; ++c) {
				occupied[r][c] = true;
			}
		}
	}

	// Merge contiguous empty cells along each row into wider candidate rects,
	// rather than leaving lots of postage-stamp-sized single cells.
	for (int r = 0; r < kGridRows; ++r) {
		int c = 0;
		while (c < kGridCols) {
			if (occupied[r][c]) {
				++c;
				continue;
			}
			const int startCol = c;
			while (c < kGridCols && !occupied[r][c]) {
				++c;
			}

			metaagent::media::IntRect rect;
			rect.x = startCol * cellW;
			rect.y = r * cellH;
			rect.width = (c - startCol) * cellW;
			rect.height = cellH;
			result.push_back(rect);
		}
	}

	std::sort(result.begin(), result.end(), [](const metaagent::media::IntRect& a, const metaagent::media::IntRect& b) {
		return (static_cast<int64_t>(a.width) * a.height) > (static_cast<int64_t>(b.width) * b.height);
	});
	if (result.size() > maxCount) {
		result.resize(maxCount);
	}
	return result;
}
