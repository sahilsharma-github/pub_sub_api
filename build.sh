scons -c
scons
cp libpubsub.a /root/Sahil/temp/test/
cd /root/Sahil/temp/test/swig
scons -c
swig -python /root/Sahil/temp/test/swig/pub_sub_api.i
scons
mv lib_pub_sub_api.so _pub_sub_api.so
