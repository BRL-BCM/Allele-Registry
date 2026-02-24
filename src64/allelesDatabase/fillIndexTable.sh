dir="/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output"
k=250000000
s=0
for i in {1..16}
do
	skip=`echo "($i-1+$s)*$k" | bc`
	./convertIndexIdentifiers $skip $k $dir
	if [[ -f stop ]]; then
		echo "Terminating, remove 'stop' file to continue"
		exit 0
	fi
done
