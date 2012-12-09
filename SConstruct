# -*- mode: python; -*-
import os

env = Environment(ENV=os.environ)
#Prefer MinGW over other compilers
Tool('mingw')(env)
env.Replace(CCFLAGS=['-std=c++11', '-g'])


env.Program(target='test.exe', source='test.cpp')
