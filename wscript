#!./waf

import os
import subprocess

def options(ctx):
    ctx.load('compiler_c')

import waflib.Logs
def configure(ctx):
    CC = os.environ['CC']
    if os.environ['CC'] == 'emcc':
        del os.environ['CC']
    ctx.load('compiler_c')

    if CC == 'emcc':
        ctx.env.CC = 'emcc'
        ctx.env.LINK_CC = 'emcc'

    # Last time debugged with clang it created invalid paths in the debug data
    # which lead to missing function debug data for *.c code.
    # try:
    #     ctx.find_program('clang')
    #     ctx.env.CC = 'clang'
    #     ctx.env.LINK_CC = 'clang'
    # except:
    #     pass

    # try:
    #     ctx.find_program( 'emcc', path_list=['~/Projects/emscripten'] )
    #     ctx.env.CC = 'emcc'
    #     ctx.env.LINK_CC = 'emcc'
    #     ctx.env.cprogram_PATTERN = '%s.js'
    # except:
    #     pass

    # ctx.env.append_value('CFLAGS', '-g')
    ctx.env.append_value('CFLAGS', '-O3' if CC != 'emcc' else '-O2')
    ctx.env.append_value('CFLAGS', '-std=c99')
    ctx.env.append_value('LINKFLAGS', '-O4' if CC != 'emcc' else '-O2')

    if CC == 'emcc':
        ctx.env.append_value('CFLAGS', '-s')
        ctx.env.append_value('CFLAGS', 'ASM_JS=1')
        ctx.env.append_value('CFLAGS', '-s')
        ctx.env.append_value('CFLAGS', 'FUNCTION_POINTER_ALIGNMENT=1')
        ctx.env.append_value('LINKFLAGS', '-s')
        ctx.env.append_value('LINKFLAGS', 'ASM_JS=1')
        ctx.env.cprogram_PATTERN = '%s.js'

    ctx.start_msg( 'init submodules' )
    gitStatus = ctx.exec_command( 'git submodule init && git submodule update' )
    if gitStatus == 0:
        ctx.end_msg( 'ok', 'GREEN' )
    else:
        ctx.fatal( 'fail' )

    ctx.start_msg( 'build libtap dependency' )
    libtapStatus = ctx.exec_command( 'cd vendor/libtap && make' )
    if libtapStatus == 0:
        ctx.end_msg( 'ok', 'GREEN' )
    else:
        ctx.fatal( 'fail' )

    cc_env = ctx.env

    ctx.setenv('sdl', cc_env)
    try:
        ctx.check_cfg(
            path='sdl-config', args='--cflags --libs',
            package='', uselib_store='SDL'
        )

        ctx.env.FRAMEWORK = [ 'Cocoa', 'OpenGL' ]
    except:
        ctx.env.DISABLED = True

from waflib.TaskGen import after_method,before_method,feature,taskgen_method,extension

from waflib import Task

from waflib.Node import Node

# relative include paths for emcc
@feature('c','cxx','d','asm','fc','includes')
@after_method('propagate_uselib_vars','process_source','apply_incpaths')
def apply_incpaths_emcc(self):
    if self.env.CC.endswith('emcc'):
        lst=self.to_incnodes(
            self.to_list(getattr(self,'includes',[])) +
                self.env['INCLUDES']
        )
        self.includes_nodes=lst
        self.env['INCPATHS'] = [x.path_from(self.bld.bldnode) for x in lst]

def build(bld):
    # Force emscripten to optimize compiled objects.
    os.environ['EMCC_OPTIMIZE_NORMALLY'] = '1'

    bld.install_files( '${PREFIX}/include', 'src/ph.h' )

    source = bld.path.ant_glob('src/*.c')
    d = {
        'source': source,
        'includes': 'src',
        'target': 'ph',
        'install_path': '${PREFIX}/lib',
        'lib': 'm'
    }
    bld.stlib( **d )
    # bld.shlib( **d )

    bld.program(
        source=bld.path.ant_glob('test/*.c'),
        includes='src ../vendor/libtap',
        target='ph-test',
        libpath='../vendor/libtap',
        lib='tap m',
        use='ph',
        install_path=None
    )

    bld.program(
        source=bld.path.ant_glob('bench/*.c'),
        includes='src ../vendor/libtap',
        target='ph-bench',
        libpath='../vendor/libtap',
        lib='tap m',
        use='ph',
        install_path=None
    )

    bld.program(
        source=bld.path.ant_glob('example/simple/*.c example/bench/*.c'),
        includes='src',
        target='ph-stress',
        lib='m',
        use='ph',
        install_path=None
    )

    # Build sdl example if it was able to configure.
    if 'sdl' in bld.all_envs and not bld.all_envs['sdl']['DISABLED']:
        bld.env = bld.all_envs['sdl']

        bld.program(
            source=bld.path.ant_glob('example/simple/*.c example/sdl/*.c'),
            includes='src',
            defines='FRAMESTATS=1',
            target='sdl-example',
            lib='m',
            framework=['Cocoa', 'OpenGL'],
            use='ph SDL',
            install_path=None
        )

import waflib.Scripting, waflib.Context

def test(ctx):
    waflib.Scripting.run_command( 'build' )
    ctx.to_log( '\n' )
    ctx.cmd_and_log(
        'build/ph-test\n',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        quiet=waflib.Context.BOTH
    )
