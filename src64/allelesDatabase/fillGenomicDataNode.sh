k=250000000
s=4
for i in {1..10000}
do
	skip=`echo "($i-1+$s)*$k" | bc`
	./convertGenomicDataNode $skip $k
	if [[ -f stop ]]; then
		echo "Terminating, remove 'stop' file to continue"
		exit 0
	fi
done
