#!/bin/sh

dir=$(dirname $0)

for uml in $dir/*.uml;
do
    plantuml -failonerror -config $dir/plantuml.cfg -tsvg $uml -o $(pwd) || exit 1
done

