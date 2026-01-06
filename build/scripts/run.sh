sh build.sh
if [ $? == 0 ]; then
	(cd ../../bin && ./shiptastic)
	exit $?
else
	exit 1
fi
exit 0
