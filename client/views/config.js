// Copyright (c) 2015 Isode Limited.
// All rights reserved.
// See the LICENSE file for more information.

var View = require('ampersand-view');
var templates = require('../templates/compiled');
var FormView = require('ampersand-form-view');
var InputView = require('ampersand-input-view');

var ConfigForm = FormView.extend({
	fields: function() {
		return [
			new InputView({
				label: 'Name',
				name: 'name',
				value: this.model && this.model.name,
				placeholder: 'Application Name',
				parent: this
			})
		];
	}
});

module.exports = View.extend({
	template: templates.config,
	autoRender: true,
	render: function() {
		this.renderWithTemplate();
		this.form = new ConfigForm({
			model: this.model,
			el: this.queryByHook('config-form'),
			submitCallback: function(data) {
				this.model.name = data['name'];
				this.model.save();
			}
		});
		this.registerSubview(this.form);
	},
	bindings: {
		'model.name': {
			type: 'text',
			hook: 'name'
		}
	}
});
