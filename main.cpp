/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the LICENSE file for more information.
 */

#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
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

class DemoSessionCollection : public SessionCollection<boost::shared_ptr<DemoSession>> {
	public:
		DemoSessionCollection() {}
		virtual ~DemoSessionCollection() {}

		std::string addSession(boost::shared_ptr<DemoSession> session) {
			std::string key = boost::lexical_cast<std::string>(uuidGenerator_());
			sessions_[key] = session;
			webSocketSessions_[session->webSocket_] = std::pair<boost::shared_ptr<DemoSession>, std::string>(session, key);
			return key;
		}

		void removeSession(WebSocket::ref webSocket) {
			boost::shared_ptr<DemoSession> session = webSocketSessions_[webSocket].first;
			std::string key = webSocketSessions_[webSocket].second;
			if (!session) return;
			sessions_.erase(key);
			webSocketSessions_.erase(session->webSocket_);
		}

		bool hasSession(WebSocket::ref webSocket) {
			return webSocketSessions_.count(webSocket) > 0;
		}
	private:
		std::map<WebSocket::ref, std::pair<boost::shared_ptr<DemoSession>, std::string>> webSocketSessions_;
		boost::uuids::random_generator uuidGenerator_;
};

boost::shared_ptr<JSONObject> configToJSON(Config* config) {
	boost::shared_ptr<JSONObject> json = boost::make_shared<JSONObject>();
	json->set("name", boost::make_shared<JSONString>(config->name_));
	return json;
}

void handleConfigGetRequest(boost::shared_ptr<RESTRequest> request, Config* config) {
	request->setReplyHeader(RESTRequest::HTTP_OK);
	request->addReplyContent(configToJSON(config));

	request->sendReply();
}


class WebSockets {
	public:
		WebSockets(boost::shared_ptr<DemoSessionCollection> sessions) : sessions_(sessions) {}
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
		void handleMessage(WebSocket::ref webSocket, boost::shared_ptr<JSONObject> message) {
			std::cout << "Receiving websocket message " << message->serialize() << std::endl;
			if (sessions_->hasSession(webSocket)) {
				// Unexpected message, but just ignore for this demo and reprocess it
			}
			
			boost::shared_ptr<JSONString> type = boost::dynamic_pointer_cast<JSONString>(message->getValues()["type"]);
			
			if (!type || type->getValue() != "login") { 
				std::cout << "Not a login" << std::endl;
				return;
			} // Bad format
			
			boost::shared_ptr<JSONString> user = boost::dynamic_pointer_cast<JSONString>(message->getValues()["user"]);
			boost::shared_ptr<JSONString> password = boost::dynamic_pointer_cast<JSONString>(message->getValues()["password"]);
			
			if (!user || !password || user->getValue() != "demo" || password->getValue() != "secret!") {
				std::cout << "Login invalid" << std::endl;
				// Do real error handling in a real application
				return;
			}
			
			std::cout << "Login successful" << std::endl;
			
			boost::shared_ptr<DemoSession> session = boost::make_shared<DemoSession>(webSocket);
			std::string key = sessions_->addSession(session);
			WebSocketHinter::ref hinter = boost::make_shared<WebSocketHinter>(webSocket);
			hinters_.push_back(hinter);
			webSocket->onClosed.connect(boost::bind(&WebSockets::handleWebSocketClosed, this, hinter));
			
			boost::shared_ptr<JSONObject> reply = boost::make_shared<JSONObject>();
			reply->set("type", boost::make_shared<JSONString>("login-success"));
			reply->set("cookie", boost::make_shared<JSONString>("librestpp_session=" + key));
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
		boost::shared_ptr<DemoSessionCollection> sessions_;

};

void handleConfigPostRequest(boost::shared_ptr<RESTRequest> request, Config* config, WebSockets* webSockets) {
	boost::shared_ptr<JSONObject> json = request->getJSON();

	boost::shared_ptr<JSONString> str;

	if (!!json) {
		auto values = json->getValues();
		auto nameJSON = values["name"];
		str = boost::dynamic_pointer_cast<JSONString>(nameJSON);
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
	librestpp::RESTServer server(1080);

	Config config;
	config.name_ = "Demo";

	boost::shared_ptr<DemoSessionCollection> sessions = boost::make_shared<DemoSessionCollection>();

	WebSockets webSockets(sessions);

	typedef SessionRESTHandler<boost::shared_ptr<DemoSession>> DemoSessionRestHandler;

	boost::shared_ptr<DemoSessionRestHandler> configGetHandler = boost::make_shared<DemoSessionRestHandler>(sessions, [&config](boost::shared_ptr<DemoSession>, boost::shared_ptr<RESTRequest> request) {handleConfigGetRequest(request, &config);});
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::GET), configGetHandler);

	boost::shared_ptr<DemoSessionRestHandler> configPostHandler = boost::make_shared<DemoSessionRestHandler>(sessions, [&config, &webSockets](boost::shared_ptr<DemoSession>, boost::shared_ptr<RESTRequest> request) {handleConfigPostRequest(request, &config, &webSockets);});
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::POST), configPostHandler);


	boost::shared_ptr<JSONRESTHandler> cssHandler = boost::make_shared<MemoryFileHandler>("client/_build/styles.nonCached.css");
	server.addJSONEndpoint(PathVerb("/styles.nonCached.css", PathVerb::GET), cssHandler);

	boost::shared_ptr<JSONRESTHandler> jsHandler = boost::make_shared<MemoryFileHandler>("client/_build/app.nonCached.js");
	server.addJSONEndpoint(PathVerb("/app.nonCached.js", PathVerb::GET), jsHandler);

	boost::shared_ptr<JSONRESTHandler> htmlHandler = boost::make_shared<MemoryFileHandler>("client/_build/index.html");
	server.addDefaultGetEndpoint(htmlHandler);

	server.onWebSocketConnection.connect(boost::bind(&WebSockets::handleNewWebSocket, &webSockets, _1));
	server.run();
}
