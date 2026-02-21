test_function() {
	sleep $RANDOM
	curl "URL" &
	pid=$?
	[[ $RANDOM -gt 0.5 ]] && kill $pid
}

test_size=1000
for i in `seq 1 $test_size`; do
	test_function &
	curl test &
done
