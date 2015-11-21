/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the LICENSE file for more information.
 */

#include <memory>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <restpp/JSONRESTHandler.h>
#include <restpp/MemoryFileHandler.h>
#include <restpp/RESTServer.h>
#include <restpp/SessionCollection.h>
#include <restpp/SessionRESTHandler.h>
#include <restpp/WebSocket.h>
#include <restpp/WebSocketHinter.h>


#include "config.h"

using namespace librestpp;
using namespace restsocket;

class DemoSession {
	public:
		DemoSession(WebSocket::ref webSocket) : webSocket_(webSocket) {}
		WebSocket::ref webSocket_;
};

class DemoSessionCollection : public SessionCollection<std::shared_ptr<DemoSession>> {
	public:
		DemoSessionCollection() {}
		virtual ~DemoSessionCollection() {}

		std::string addSession(std::shared_ptr<DemoSession> session) {
			std::string key = boost::lexical_cast<std::string>(uuidGenerator_());
			sessions_[key] = session;
			webSocketSessions_[session->webSocket_] = std::pair<std::shared_ptr<DemoSession>, std::string>(session, key);
			return key;
		}

		void removeSession(WebSocket::ref webSocket) {
			std::shared_ptr<DemoSession> session = webSocketSessions_[webSocket].first;
			std::string key = webSocketSessions_[webSocket].second;
			if (!session) return;
			sessions_.erase(key);
			webSocketSessions_.erase(session->webSocket_);
		}

		bool hasSession(WebSocket::ref webSocket) {
			return webSocketSessions_.count(webSocket) > 0;
		}

		virtual std::shared_ptr<DemoSession> getSession(const std::string& sessionKey) {
			return sessions_[sessionKey];
		}
	private:
		std::map<WebSocket::ref, std::pair<std::shared_ptr<DemoSession>, std::string>> webSocketSessions_;
		boost::uuids::random_generator uuidGenerator_;
		std::map<std::string, std::shared_ptr<DemoSession>> sessions_;
};

std::shared_ptr<JSONObject> configToJSON(Config* config) {
	std::shared_ptr<JSONObject> json = std::make_shared<JSONObject>();
	json->set("name", std::make_shared<JSONString>(config->name_));
	return json;
}

void handleConfigGetRequest(std::shared_ptr<RESTRequest> request, Config* config) {
	request->setReplyHeader(RESTRequest::HTTP_OK);
	request->addReplyContent(configToJSON(config));

	request->sendReply();
}


class WebSockets {
	public:
		WebSockets(std::shared_ptr<DemoSessionCollection> sessions) : sessions_(sessions) {}
		void handleNewWebSocket(WebSocket::ref webSocket) {
			std::cout << "Received new websocket connection" << std::endl;
			webSocket->onMessage.connect(boost::bind(&WebSockets::handleMessage, this, webSocket, _1));
		}
		void sendModelHint(const std::string& uri) {
			for (size_t i = 0; i < hinters_.size(); i++) {
				hinters_[i]->sendModelHint(uri);
			}
		}

	private:
		void handleMessage(WebSocket::ref webSocket, std::shared_ptr<JSONObject> message) {
			std::cout << "Receiving websocket message " << message->serialize() << std::endl;
			if (sessions_->hasSession(webSocket)) {
				// Unexpected message, but just ignore for this demo and reprocess it
			}

			std::shared_ptr<JSONString> type = std::dynamic_pointer_cast<JSONString>(message->getValues()["type"]);

			if (!type || type->getValue() != "login") {
				std::cout << "Not a login" << std::endl;
				return;
			} // Bad format

			std::shared_ptr<JSONString> user = std::dynamic_pointer_cast<JSONString>(message->getValues()["user"]);
			std::shared_ptr<JSONString> password = std::dynamic_pointer_cast<JSONString>(message->getValues()["password"]);

			if (!user || !password || user->getValue() != "demo" || password->getValue() != "secret!") {
				std::cout << "Login invalid" << std::endl;
				// Do real error handling in a real application
				return;
			}

			std::cout << "Login successful" << std::endl;

			std::shared_ptr<DemoSession> session = std::make_shared<DemoSession>(webSocket);
			std::string key = sessions_->addSession(session);
			WebSocketHinter::ref hinter = std::make_shared<WebSocketHinter>(webSocket);
			hinters_.push_back(hinter);
			webSocket->onClosed.connect(boost::bind(&WebSockets::handleWebSocketClosed, this, hinter));

			std::shared_ptr<JSONObject> reply = std::make_shared<JSONObject>();
			reply->set("type", std::make_shared<JSONString>("login-success"));
			reply->set("cookie", std::make_shared<JSONString>("librestpp_session=" + key));
			std::cout << "Sending reply:" << reply->serialize() << std::endl;
			hinter->send(reply);
		}

		void handleWebSocketClosed(WebSocketHinter::ref hinter) {
			std::cout << "Websocket closed" << std::endl;
			sessions_->removeSession(hinter->getWebSocket());
			hinters_.erase(std::remove(hinters_.begin(), hinters_.end(), hinter), hinters_.end());
		}

	private:
		std::vector<WebSocketHinter::ref> hinters_;
		std::shared_ptr<DemoSessionCollection> sessions_;

};

void handleConfigPostRequest(std::shared_ptr<RESTRequest> request, Config* config, WebSockets* webSockets) {
	std::shared_ptr<JSONObject> json = request->getJSON();

	std::shared_ptr<JSONString> str;

	if (!!json) {
		auto values = json->getValues();
		auto nameJSON = values["name"];
		str = std::dynamic_pointer_cast<JSONString>(nameJSON);
	}
	if (str) {
		config->name_ = str->getValue();
		request->setReplyHeader(RESTRequest::HTTP_OK);
		request->addReplyContent(configToJSON(config));
	}
	else {
		request->setReplyHeader(RESTRequest::HTTP_OK);
		request->addReplyContent("Invalid JSON sent");
	}
	request->sendReply();
	webSockets->sendModelHint("/api/config");
}


int main(int argc, const char* argv[])
{
	librestpp::RESTServer server;

	Config config;
	config.name_ = "Demo";

	std::shared_ptr<DemoSessionCollection> sessions = std::make_shared<DemoSessionCollection>();

	WebSockets webSockets(sessions);

	typedef SessionRESTHandler<std::shared_ptr<DemoSession>> DemoSessionRestHandler;

	std::shared_ptr<DemoSessionRestHandler> configGetHandler = std::make_shared<DemoSessionRestHandler>(sessions, [&config](std::shared_ptr<DemoSession>, std::shared_ptr<RESTRequest> request) {handleConfigGetRequest(request, &config);});
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::GET), configGetHandler);

	std::shared_ptr<DemoSessionRestHandler> configPostHandler = std::make_shared<DemoSessionRestHandler>(sessions, [&config, &webSockets](std::shared_ptr<DemoSession>, std::shared_ptr<RESTRequest> request) {handleConfigPostRequest(request, &config, &webSockets);});
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::POST), configPostHandler);


	std::shared_ptr<JSONRESTHandler> cssHandler = std::make_shared<MemoryFileHandler>("client/_build/styles.nonCached.css");
	server.addJSONEndpoint(PathVerb("/styles.nonCached.css", PathVerb::GET), cssHandler);

	std::shared_ptr<JSONRESTHandler> jsHandler = std::make_shared<MemoryFileHandler>("client/_build/app.nonCached.js");
	server.addJSONEndpoint(PathVerb("/app.nonCached.js", PathVerb::GET), jsHandler);

	std::shared_ptr<JSONRESTHandler> htmlHandler = std::make_shared<MemoryFileHandler>("client/_build/index.html");
	server.addDefaultGetEndpoint(htmlHandler);

	server.onWebSocketConnection.connect(boost::bind(&WebSockets::handleNewWebSocket, &webSockets, _1));
	server.listen(1080);
	server.run();
}
