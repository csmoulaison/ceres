gcc -g ../src/build.c -o ../build -lm -I ../../src/ -I ../../extern/ -lX11 -lX11-xcb -lxcb -lXfixes
if [ $? != 0 ]; then
	exit $?
fi
(cd ../ && ./build)
exit $?
