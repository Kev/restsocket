// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var View = require('ampersand-view');
var templates = require('../templates/compiled');
var FormView = require('ampersand-form-view');
var InputView = require('ampersand-input-view');

var LoginForm = FormView.extend({
	fields: function() {
		return [
			new InputView({
				label: 'Username',
				name: 'user',
				placeholder: 'demo',
				parent: this
			}),
			new InputView({
				label: 'Password',
				name: 'password',
				placeholder: 'secret!',
				parent: this
			})
		];
	}
});

module.exports = View.extend({
	template: templates.login,
	autoRender: true,
	render: function() {
		var self = this;
		self.renderWithTemplate();
		self.loggedIn = false;
		self.form = new LoginForm({
			el: self.queryByHook('login-form'),
			submitCallback: function(data) {
				self.websocket.addEventListener('message', function(message) {
					if (self.loggedIn) {
						return;
					}
					try {
				     	var data = JSON.parse(message.data);
				    }
				    catch (err) {
				      	console.error("Received bad JSON. " + err);
				    }
					var type = data.type;
				    if (type != 'login-success') {
						console.log("Ignoring non-login type " + type);
			      		return;
				    }
					var cookie = data.cookie;
					document.cookie = cookie;
					console.log("Logged in and setting cookie: " + cookie);
					self.loggedIn = true;
					self.loginCallback();
				});
				var payload = JSON.stringify({
					type: 'login',
					user: data['user'],
					password: data['password']
				});
				console.log("Sending login payload: " + payload);
				self.websocket.send(payload);
			}
		});
		self.registerSubview(self.form);
	}
});
