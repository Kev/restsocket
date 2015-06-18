// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var _ = require('underscore');
var templatizer = require('templatizer');

module.exports = function (obj) {
	return _.extend({
		moonboots: {
			developmentMode: true,
			main: __dirname + '/app.js',
			timingMode: true,
			libraries: [
				// __dirname + '/third-party/jquery.min.js',
				// __dirname + '/third-party/bootstrap.min.js'
			],
			stylesheets: [
				// __dirname + '/third-party/bootstrap.min.css',
				__dirname + '/stylesheets/restsocket.css'
			],
			resourcePrefix: '/',
			beforeBuildJS: function() {
				templatizer(__dirname + '/templates', __dirname + '/templates/compiled.js');
			}
		},
		public: __dirname + '/public',
		directory: __dirname + '/_build'
	}, obj);
};
