sh build.sh
if [ $? == 0 ]; then
	(cd ../../bin && valgrind --tool=callgrind --cache-sim=yes --branch-sim=yes ./ceres)
	exit $?
else
	exit 1
fi
exit 0
