// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var Router = require('ampersand-router');
var AboutPage = require('./views/about');
var ConfigPage = require('./views/config');
var HomePage = require('./views/home');

module.exports = Router.extend({
	routes: {
		'': 'home',
		'about': 'about',
		'config': 'config'
	},

	home: function() {
		this.trigger('page', new HomePage());
	},
	about: function() {
		this.trigger('page', new AboutPage());
	},
	config: function() {
		this.trigger('page', new ConfigPage({
			'model': app.config
		}));
	}
});
