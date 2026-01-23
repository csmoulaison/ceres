gcc -g ../src/build.c -o ../build \
	-I ../../src/ -I ../../extern/ \
	-I ../../extern/freetype-2.14.1/out/include/freetype2/ \
	-L ../../extern/freetype-2.14.1/out/include/freetype2/ \
	-lm -lX11 -lX11-xcb -lxcb -lXfixes -lfreetype

if [ $? != 0 ]; then
	exit $?
fi
(cd ../ && ./build)
exit $?
