/*
 * media-player-cpp — HttpControlServer
 * Copyright (c) vecnode 2026
 *
 * Lightweight REST control plane over ofxTCPServer. Not a general-purpose web
 * server — tuned for small JSON request/response cycles from local automation.
 */

#include "HttpControlServer.h"

#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

constexpr int kMaxRequestBytes = 8192;
constexpr const char* kApiPrefix = "/api/";

std::string toLowerAscii(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return value;
}

std::string trim(const std::string& value) {
	const auto start = value.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return {};
	}
	const auto end = value.find_last_not_of(" \t\r\n");
	return value.substr(start, end - start + 1);
}

bool parseContentLength(const std::string& headers, int& contentLength) {
	contentLength = 0;

	std::istringstream stream(headers);
	std::string line;
	while (std::getline(stream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		const auto colon = line.find(':');
		if (colon == std::string::npos) {
			continue;
		}

		const std::string name = toLowerAscii(trim(line.substr(0, colon)));
		if (name == "content-length") {
			contentLength = ofToInt(trim(line.substr(colon + 1)));
			return true;
		}
	}

	return false;
}

bool isLoopbackAddress(const std::string& ip) {
	return ip == "127.0.0.1"
		|| ip == "::1"
		|| ip == "000.000.000.000"
		|| ip == "0.0.0.0";
}

ofJson statusToJson(const MediaPlayerStatus& status) {
	return ofJson{
		{"loaded", status.loaded},
		{"playing", status.playing},
		{"clipIndex", status.clipIndex},
		{"clipCount", status.clipCount},
		{"clipName", status.clipName},
		{"subtitlesEnabled", status.subtitlesEnabled}
	};
}

} // namespace

void HttpControlServer::setup(int listenPort, MediaPlayerController* mediaController) {
	controller = mediaController;
	port = listenPort;
	localhostOnly = true;
	clientBuffers.clear();

	if (!server.setup(port)) {
		running = false;
		ofLogError("HttpControlServer") << "Failed to bind port " << port
			<< " — HTTP API disabled";
		return;
	}

	running = true;
	ofLogNotice("HttpControlServer") << "HTTP API listening on http://127.0.0.1:"
		<< port << " (localhost clients only)";
	ofLogNotice("HttpControlServer") << "  GET  /api/status";
	ofLogNotice("HttpControlServer") << "  GET  /api/clips";
	ofLogNotice("HttpControlServer") << "  GET  /api/health";
	ofLogNotice("HttpControlServer") << "  POST /api/play | /api/stop | /api/next | /api/subtitles";
	ofLogNotice("HttpControlServer") << "  POST /api/clips/{index}";
}

void HttpControlServer::shutdown() {
	if (running) {
		server.close();
		clientBuffers.clear();
		running = false;
	}
}

bool HttpControlServer::isClientAllowed(int clientId) {
	if (!localhostOnly) {
		return true;
	}
	return isLoopbackAddress(server.getClientIP(clientId));
}

bool HttpControlServer::appendReceivedBytes(int clientId, std::string& buffer) {
	char chunk[1024];
	const int received = server.receiveRawBytes(clientId, chunk, sizeof(chunk));
	if (received <= 0) {
		return false;
	}

	buffer.append(chunk, received);
	return true;
}

bool HttpControlServer::tryParseRequest(const std::string& raw, ParsedRequest& request) const {
	if (raw.size() > static_cast<std::size_t>(kMaxRequestBytes)) {
		return false;
	}

	const std::size_t headerEnd = raw.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		return false;
	}

	const std::string headerBlock = raw.substr(0, headerEnd);
	const std::string body = raw.substr(headerEnd + 4);

	const std::size_t lineBreak = headerBlock.find("\r\n");
	if (lineBreak == std::string::npos) {
		return false;
	}

	const std::string requestLine = headerBlock.substr(0, lineBreak);
	std::istringstream lineStream(requestLine);
	lineStream >> request.method >> request.path;

	if (request.method.empty() || request.path.empty()) {
		return false;
	}

	const std::size_t queryPos = request.path.find('?');
	if (queryPos != std::string::npos) {
		request.path = request.path.substr(0, queryPos);
	}

	int contentLength = 0;
	const std::string headerLines = headerBlock.substr(lineBreak + 2);
	if (parseContentLength(headerLines, contentLength)) {
		if (contentLength < 0 || body.size() < static_cast<std::size_t>(contentLength)) {
			return false;
		}
		request.body = body.substr(0, static_cast<std::size_t>(contentLength));
	} else if (!body.empty()) {
		request.body = body;
	}

	return true;
}

void HttpControlServer::sendJson(int clientId, int httpStatus, const std::string& statusText,
	const std::string& jsonBody) {

	const std::string response =
		"HTTP/1.1 " + ofToString(httpStatus) + " " + statusText + "\r\n"
		"Content-Type: application/json\r\n"
		"Connection: close\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Length: " + ofToString(jsonBody.size()) + "\r\n"
		"\r\n"
		+ jsonBody;

	server.sendRawBytes(clientId, response.c_str(), static_cast<int>(response.size()));
}

void HttpControlServer::sendError(int clientId, int httpStatus, const std::string& statusText,
	const std::string& message) {

	const ofJson payload = {{"ok", false}, {"error", message}};
	sendJson(clientId, httpStatus, statusText, payload.dump());
}

void HttpControlServer::handleRequest(int clientId, const ParsedRequest& request) {
	if (!controller) {
		sendError(clientId, 503, "Service Unavailable", "controller not ready");
		return;
	}

	const std::string& method = request.method;
	const std::string& path = request.path;

	if (path == "/api/health" && method == "GET") {
		const ofJson payload = {{"ok", true}};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (path == "/api/status" && method == "GET") {
		sendJson(clientId, 200, "OK", statusToJson(controller->getStatus()).dump());
		return;
	}

	if (path == "/api/clips" && method == "GET") {
		ofJson payload = ofJson::array();
		for (const auto& clip : controller->getClips()) {
			payload.push_back({
				{"index", clip.index},
				{"name", clip.name},
				{"path", clip.path}
			});
		}
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (path == "/api/play" && method == "POST") {
		controller->play();
		ofJson payload = {{"ok", true}, {"status", statusToJson(controller->getStatus())}};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (path == "/api/stop" && method == "POST") {
		controller->stop();
		ofJson payload = {{"ok", true}, {"status", statusToJson(controller->getStatus())}};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (path == "/api/next" && method == "POST") {
		controller->nextClip();
		ofJson payload = {{"ok", true}, {"status", statusToJson(controller->getStatus())}};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (path == "/api/subtitles" && method == "POST") {
		const ofJson body = ofJson::parse(request.body, nullptr, false);
		if (body.is_discarded() || !body.contains("enabled") || !body["enabled"].is_boolean()) {
			sendError(clientId, 400, "Bad Request", "expected JSON body: {\"enabled\": true|false}");
			return;
		}

		controller->setSubtitlesEnabled(body["enabled"].get<bool>());
		ofJson payload = {
			{"ok", true},
			{"subtitlesEnabled", controller->isSubtitlesEnabled()},
			{"status", statusToJson(controller->getStatus())}
		};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	if (method == "POST" && ofIsStringInString(path, "/api/clips/")) {
		const std::string indexText = path.substr(std::string("/api/clips/").size());
		if (indexText.empty() || !std::all_of(indexText.begin(), indexText.end(),
			[](unsigned char c) { return std::isdigit(c) != 0; })) {
			sendError(clientId, 400, "Bad Request", "invalid clip index");
			return;
		}

		const std::size_t index = static_cast<std::size_t>(ofToInt(indexText));
		if (!controller->openClipAtIndex(index)) {
			sendError(clientId, 404, "Not Found", "clip index out of range or failed to load");
			return;
		}

		ofJson payload = {{"ok", true}, {"status", statusToJson(controller->getStatus())}};
		sendJson(clientId, 200, "OK", payload.dump());
		return;
	}

	sendError(clientId, 404, "Not Found", "unknown endpoint");
}

void HttpControlServer::processClient(int clientId) {
	if (!server.isClientConnected(clientId)) {
		clientBuffers.erase(clientId);
		return;
	}

	if (!isClientAllowed(clientId)) {
		ofLogWarning("HttpControlServer") << "Rejected non-local client " << server.getClientIP(clientId);
		sendError(clientId, 403, "Forbidden", "localhost only");
		server.disconnectClient(clientId);
		clientBuffers.erase(clientId);
		return;
	}

	std::string& buffer = clientBuffers[clientId];

	while (server.getNumReceivedBytes(clientId) > 0) {
		if (!appendReceivedBytes(clientId, buffer)) {
			break;
		}
	}

	if (buffer.empty()) {
		return;
	}

	if (buffer.size() > static_cast<std::size_t>(kMaxRequestBytes)) {
		sendError(clientId, 413, "Payload Too Large", "request exceeds size limit");
		server.disconnectClient(clientId);
		clientBuffers.erase(clientId);
		return;
	}

	ParsedRequest request;
	if (!tryParseRequest(buffer, request)) {
		return;
	}

	handleRequest(clientId, request);
	server.disconnectClient(clientId);
	clientBuffers.erase(clientId);
}

void HttpControlServer::update() {
	if (!running) {
		return;
	}

	const int lastId = server.getLastID();
	for (int clientId = 0; clientId < lastId; ++clientId) {
		if (!server.isClientConnected(clientId)) {
			clientBuffers.erase(clientId);
			continue;
		}

		if (server.getNumReceivedBytes(clientId) > 0) {
			processClient(clientId);
		}
	}
}
