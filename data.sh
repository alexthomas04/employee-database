#!/bin/bash

curl -sS https://data-generator.alexk8s.com/v1/template/4f33666e-0996-4678-b9d6-ebb4fc6f6ba5/generate?count=20 | jq -r '.[]' | while IFS= read -r row;
do
  ./bin/cli 127.0.0.1 "$row"
done

./bin/cli 127.0.0.1
