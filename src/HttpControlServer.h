#pragma once

#include "MediaPlayerController.h"
#include "ofxNetwork.h"
#include <map>
#include <string>

/// Minimal HTTP/1.1 JSON API for remote control of the media player.
///
/// Poll update() from the main thread each frame. Commands execute on the same
/// thread as the GUI, so ofVideoPlayer remains thread-safe.
class HttpControlServer {
public:
	static constexpr int kDefaultPort = 8080;
	static constexpr const char* kDefaultBindHint = "127.0.0.1";

	void setup(int port, MediaPlayerController* controller);
	void update();
	void shutdown();

	bool isRunning() const { return running; }
	int getPort() const { return port; }

private:
	struct ParsedRequest {
		std::string method;
		std::string path;
		std::string body;
	};

	void processClient(int clientId);
	bool appendReceivedBytes(int clientId, std::string& buffer);
	bool tryParseRequest(const std::string& raw, ParsedRequest& request) const;
	void handleRequest(int clientId, const ParsedRequest& request);
	void sendJson(int clientId, int httpStatus, const std::string& statusText, const std::string& jsonBody);
	void sendError(int clientId, int httpStatus, const std::string& statusText, const std::string& message);
	bool isClientAllowed(int clientId);

	MediaPlayerController* controller = nullptr;
	ofxTCPServer server;
	std::map<int, std::string> clientBuffers;

	bool running = false;
	int port = kDefaultPort;
	bool localhostOnly = true;
};
