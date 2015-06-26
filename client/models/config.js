// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var AmpersandModel = require('ampersand-model');
var AmpersandWebSocketHint = require('ampersand-websocket-hint');

module.exports = AmpersandModel.extend(AmpersandWebSocketHint, {
	props: {
		name: 'string'
	},
	urlRoot: '/api/config'
});
