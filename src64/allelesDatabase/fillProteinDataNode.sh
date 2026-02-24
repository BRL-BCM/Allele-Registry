k=250000000
s=0
for i in {1..10000}
do
	skip=`echo "($i-1+$s)*$k" | bc`
	./convertProteinDataNode $skip $k
	if [[ -f stop ]]; then
		echo "Terminating, remove 'stop' file to continue"
		exit 0
	fi
done
