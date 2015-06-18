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

int main(int argc, const char* argv[])
{
	librestpp::RESTServer server(1080);

	boost::shared_ptr<JSONRESTHandler> cssHandler = boost::make_shared<MemoryFileHandler>("client/_build/styles.nonCached.css");
	server.addJSONEndpoint(PathVerb("/styles.nonCached.css", PathVerb::GET), cssHandler);

	boost::shared_ptr<JSONRESTHandler> jsHandler = boost::make_shared<MemoryFileHandler>("client/_build/app.nonCached.js");
	server.addJSONEndpoint(PathVerb("/app.nonCached.js", PathVerb::GET), jsHandler);

	boost::shared_ptr<JSONRESTHandler> htmlHandler = boost::make_shared<MemoryFileHandler>("client/_build/index.html");
	server.addDefaultGetEndpoint(htmlHandler);

	server.run();
}
