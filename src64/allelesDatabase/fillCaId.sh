for i in {1..1000}
do
	./convertCaId
	if [[ -f stop ]]; then
		echo "Terminating, remove 'stop' file to continue"
		exit 0
	fi
done
