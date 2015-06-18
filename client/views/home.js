// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var View = require('ampersand-view');
var templates = require('../templates/compiled');

module.exports = View.extend({
	template: templates.home,
	autoRender: true,
	render: function () {
		this.renderWithTemplate();
	}
});
