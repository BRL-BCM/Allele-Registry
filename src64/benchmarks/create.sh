#!/bin/bash


../kyotocabinet/bin/kchashmgr create -tl -bnum 64000000 dbKcHash.kch
../kyotocabinet/bin/kctreemgr create -tl -bnum 64000000 dbKcTree.kch
#../kyotocabinet/bin/kchashmgr create -tl -bnum 32000000 dbKcHash4x.kch
