// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var LoginView = require('./views/login');
var MainView = require('./views/main');
var domready = require('domready');
var Router = require('./router');
var Config = require('./models/config');

window.app = {
	init: function() {
		var self = this;
		domready(function() {
			self.router = new Router();
			var ws = new WebSocket('ws://' + location.host); // We're not dealing with websocket lifetimes here - you need to.
			self.config = new Config();
			self.config.websocket = ws;
			self.config.fetchOnWebSocketHints(true);

			self.view = new LoginView({
				el: document.body
			});
			self.view.websocket = ws;
			self.view.loginCallback = function() {
				console.log("Switching to main view after login");
				self.view = new MainView({
					el: document.body
				});

				self.router.history.start({pushState: true});

				self.config.fetch();
			};
		});
	}
};

window.app.init();
