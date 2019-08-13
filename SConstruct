import os
import SCons

env = Environment(CXX = '/usr/bin/gcc-4.8')
env.Append(CCFLAGS = '-pthread -w -g -std=gnu99')
env.Append( LIBS = ['pthread'] )

header_dir = '/root/Sahil/temp/h/'
src_dir = '/root/Sahil/temp/src/'
filter_dir = '/root/Sahil/temp/src/filter_app/'
util_dir_header = '/root/Sahil/temp/utilities/h'
util_dir_src = '/root/Sahil/temp/utilities/src'

env.Repository(header_dir)
env.Repository(src_dir)
env.Repository(filter_dir)
env.Repository(util_dir_header)
env.Repository(util_dir_src)


pub_sub_list = env.Object('pub_sub_hash_container.c')+env.Object('pub_sub_list.c')+env.Object('pub_sub_queue.c')+ [src_dir+'pub_sub_transcoder.c', src_dir+'pub_sub_msg_defines.c', src_dir+'pub_sub_api.c']
filter_app_src = Glob(filter_dir+'*.c')+env.Object('pub_sub_hash_container.c')+env.Object('pub_sub_list.c')+env.Object('pub_sub_queue.c')+ [src_dir+'pub_sub_transcoder.c', src_dir+'pub_sub_msg_defines.c']



env.StaticLibrary('libpubsub', pub_sub_list)
env.Program (target = 'filter_app',
                source = (filter_app_src))
