# Copyright (c) 2015 Isode Limited.
# All rights reserved.
# See the LICENSE file for more information.

Import("env")

sources = ['main.cpp']

if env["SCONS_STAGE"] == "build" :
	examples_env = env.Clone()
	examples_env.AppendUnique(CPPFLAGS = ['-std=c++11'])
	examples_env.MergeFlags(examples_env['BOOST_FLAGS'])
	examples_env.MergeFlags(examples_env['RESTPP_FLAGS'])
	examples_env.Program(target = 'restsocket', source = sources)
