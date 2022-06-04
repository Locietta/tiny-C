# generate png for Control Flow Graphs

opt --dot-cfg $1

dot .fib.dot -Tpng:cairo -o a.png