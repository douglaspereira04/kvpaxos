#!/bin/bash

versions=(OLD NON_STOP ASYNC BATCH)

for file in workloads/*; do
	filename=$(basename -- "$file")
	filename="${filename%.*}"
	requests_file_name=${filename%.*}_requests.txt
	for v in "${versions[@]}"; do
		if ls ${v,,}* 1> /dev/null 2>&1; then
			if [ ! -f ${requests_file_name} ]; then
				./${v,,}_0_0 ${file}
				mv requests.txt ${requests_file_name}
			fi
		fi
	done;
done;
