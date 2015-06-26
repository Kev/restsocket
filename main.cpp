/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the LICENSE file for more information.
 */

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <restpp/JSONRESTHandler.h>
#include <restpp/MemoryFileHandler.h>
#include <restpp/RESTServer.h>

using namespace librestpp;

struct Config {
	std::string name_;
};

boost::shared_ptr<JSONObject> configToJSON(Config* config) {
	boost::shared_ptr<JSONObject> json = boost::make_shared<JSONObject>();
	json->set("name", boost::make_shared<JSONString>(config->name_));
	return json;
}

class ConfigGetHandler : public JSONRESTHandler {
	public:
		ConfigGetHandler(Config* config) : config_(config) {}

		virtual void handleRequest(boost::shared_ptr<RESTRequest> request) {
			request->setReplyHeader(RESTRequest::HTTP_OK);
			request->addReplyContent(configToJSON(config_));

			request->sendReply();
		}

	private:
		Config* config_;
};

class ConfigPostHandler : public JSONRESTHandler {
	public:
		ConfigPostHandler(Config* config) : config_(config) {}
		virtual void handleRequest(boost::shared_ptr<RESTRequest> request) {
			boost::shared_ptr<JSONObject> json = request->getJSON();

			boost::shared_ptr<JSONString> str;

			if (!!json) {
				auto values = json->getValues();
				auto nameJSON = values["name"];
				str = boost::dynamic_pointer_cast<JSONString>(nameJSON);
			}
			if (str) {
				config_->name_ = str->getValue();
				request->setReplyHeader(RESTRequest::HTTP_OK);
				request->addReplyContent(configToJSON(config_));
			}
			else {
				request->setReplyHeader(RESTRequest::HTTP_OK);
				request->addReplyContent("Invalid JSON sent");
			}
			request->sendReply();
		}
		private:
			Config* config_;
};

int main(int argc, const char* argv[])
{
	librestpp::RESTServer server(1080);

	Config config;
	config.name_ = "Demo";

	boost::shared_ptr<JSONRESTHandler> configGetHandler = boost::make_shared<ConfigGetHandler>(&config);
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::GET), configGetHandler);

	boost::shared_ptr<JSONRESTHandler> configPostHandler = boost::make_shared<ConfigPostHandler>(&config);
	server.addJSONEndpoint(PathVerb("/api/config", PathVerb::POST), configPostHandler);


	boost::shared_ptr<JSONRESTHandler> cssHandler = boost::make_shared<MemoryFileHandler>("client/_build/styles.nonCached.css");
	server.addJSONEndpoint(PathVerb("/styles.nonCached.css", PathVerb::GET), cssHandler);

	boost::shared_ptr<JSONRESTHandler> jsHandler = boost::make_shared<MemoryFileHandler>("client/_build/app.nonCached.js");
	server.addJSONEndpoint(PathVerb("/app.nonCached.js", PathVerb::GET), jsHandler);

	boost::shared_ptr<JSONRESTHandler> htmlHandler = boost::make_shared<MemoryFileHandler>("client/_build/index.html");
	server.addDefaultGetEndpoint(htmlHandler);

	server.run();
}
