gcc ../build.c -o ../build
if [ $? != 0 ]; then
	exit $?
fi
(cd ../ && ./build)
exit $?
