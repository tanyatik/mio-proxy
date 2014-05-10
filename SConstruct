source_files = Glob('proxy/*.cpp')

libraries = ['pthread', 'boost_regex']
library_paths = ''

flags = ['-Wall', '-g', '-std=c++0x']

include_paths = '.'

compiler = 'clang++'

env = Environment(CXX = compiler)
env.Append(LIBS = libraries,
           CPPFLAGS = flags,
           CPPPATH = include_paths)

env.Program('build/proxy_server', source_files)
