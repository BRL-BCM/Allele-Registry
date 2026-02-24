dir="${1}"
bloom="${2}"
rm -f ${bloom}/*
k=4000000000
s=0
SCRIPT_DIR=$( dirname "${BASH_SOURCE[0]}" )
for i in {1..1}
do
        skip=`echo "($i-1+$s)*$k" | bc`
        #./fetchCAIds $skip $k $dir $bloom
        ${SCRIPT_DIR}/getDuplicateCaIdRecords $skip $k $dir $bloom
        if [[ -f stop ]]; then
                echo "Terminating, remove 'stop' file to continue"
                exit 0
        fi
done
