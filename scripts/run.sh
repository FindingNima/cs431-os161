#!/bin/bash
$HOME/cs431-os161/scripts/compile.sh $1
cd $HOME/cs431-os161/root
sys161 kernel-ASST$1
