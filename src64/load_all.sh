#!/bin/bash

set -x
set -e

ADDR="http://localhost:16004"
STRG="/media/storage1/CARsources"
CMD='curl -X PUT'

# TODO - missing files
for i in 1 2 3 4 5 6 7 8 9
do
	$CMD "${ADDR}/alleles.json?file=def+CA+dbSNP.rs&fields=none" --data-binary @${STRG}/CAR/car_20171012_${i}.txt  >${STRG}/out_car_${i}.txt
done

#for i in 1 2 3 4 5 6 7 8
#do
#  curl -X PUT "${ADDR}/alleles.json?file=hgvs+dbSNP.rs&fields=none" --data-binary @${STRG}/dbSNP/dbsnp_${i}.txt >${STRG}/out_dbsnp_${i}.txt
#done


for i in 1 2 3
do
  $CMD "${ADDR}/alleles.json?file=MyVariantInfo_hg38.id&fields=none" --data-binary @${STRG}/myVariantInfo/mvi_hg38_${i}.txt  >${STRG}/out_mvi_hg38_${i}.txt
done


for i in 1 2 3 4
do
  $CMD "${ADDR}/alleles.json?file=MyVariantInfo_hg19.id&fields=none" --data-binary @${STRG}/myVariantInfo/mvi_hg19_${i}.txt  >${STRG}/out_mvi_hg19_${i}.txt
done


for i in 1 2 3
do
  $CMD "${ADDR}/alleles.json?file=gnomAD.id&fields=none" --data-binary @${STRG}/gnomAD/gnomad_${i}.txt  >${STRG}/out_gnomad_${i}.txt
done


$CMD "${ADDR}/alleles.json?file=ExAC.id&fields=none" --data-binary @${STRG}/ExAC/exac.txt  >${STRG}/out_exac.txt

#$CMD "${ADDR}/alleles.json?file=hgvs+ClinVar.alleleId+ClinVar.preferredName&fields=none" --data-binary @${STRG}/ClinVar/clinvar20170612_Alleles.txt  >${STRG}/out_cv_alleles.txt
#$CMD "${ADDR}/alleles.json?file=hgvs+ClinVar.variationId+ClinVar.RCV&fields=none"        --data-binary @${STRG}/ClinVar/clinvar20170612_Variants.txt >${STRG}/out_cv_variants.txt

#$CMD "${ADDR}/alleles.json?file=hgvs+COSMIC.id&fields=none" ${STRG}/COSMIC/hgvs_cosmic_coding.txt     >${STRG}/out_cosmic_1.txt
#$CMD "${ADDR}/alleles.json?file=hgvs+COSMIC.id&fields=none" ${STRG}/COSMIC/hgvs_cosmic_noncoding.txt  >${STRG}/out_cosmic_2.txt

