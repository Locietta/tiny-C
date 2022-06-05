# generate png for Control Flow Graphs

opt --dot-cfg $1

dot .$2.dot -Tpng:cairo -o $2.png