// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var AmpersandModel = require('ampersand-model');

module.exports = AmpersandModel.extend({
	props: {
		name: 'string'
	},
	urlRoot: '/api/config'
});
