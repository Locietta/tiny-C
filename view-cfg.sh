# generate png for Control Flow Graphs

opt --dot-cfg $1

dot .main.dot -Tpng:cairo -o a.png