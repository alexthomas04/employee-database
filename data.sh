#!/bin/bash

data=(
  "Lana Merrick,42 Pinecone Ridge,58"
  "Rufus Calder,19 Elmstone Drive,102"
  "Maya Trenton,803 Silver Sparrow Way,77"
  "Julian Brock,5 Old Quarry Road,63"
  "Tessa Rylin,221 Maple Crest Lane,44"
  "Cormac Vail,912 Brookhollow Court,89"
  "Nina Halberg,37 Copper Lantern St,51"
  "Elias Morren,1448 Whisperwind Trail,72"
  "Ruby Fenwick,66 Harborview Terrace,95"
  "Arden Kessler,303 Wildflower Bend,38"
  "Sylvie Arden,10 Cedarbrook Path,84"
  "Gavin Thorn,724 Redwood Crossing,57"
);

for row in "${data[@]}"
do
  ./bin/cli 127.0.0.1 "$row"
done
