// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var View = require('ampersand-view');
var ViewSwitcher = require('ampersand-view-switcher');
var templates = require('../templates/compiled');

module.exports = View.extend({
	template: templates.main,
	autoRender: true,
	events: {
		'click a[href]': 'handleLinkClick'
	},
	initialize: function() {
		this.listenTo(app.router, 'page', this.handleNewPage);
	},
	render: function() {
		this.renderWithTemplate();
		var pagesEl = this.queryByHook('page-container');
		this.pages = new ViewSwitcher(pagesEl);
	},
	handleNewPage: function(page) {
		this.pages.set(page);
	},
	handleLinkClick: function(event) {
		var tag = event.target; // We know this'll be an <a> tag
		if (tag.host === window.location.host) {
			app.router.history.navigate(tag.pathname, {trigger: true});
			event.preventDefault();
		}
	}
});
