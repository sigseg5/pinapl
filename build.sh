#!/bin/bash

function handle_errors {
    rc=$?
    if [[ "$rc" -ne 0 ]]
    then
        echo "ERROR (return code $rc)"
        exit $rc
    fi
}

CC=gcc
ASM=as
LINKER=ld

function compile_assembly {
    SOURCE_FILE=$1
    OUTPUT_FILE=$2
    DEBUG=$3

    $ASM $SOURCE_FILE $DEBUG -o $OUTPUT_FILE
    handle_errors
}

compile_assembly "start.s" "start.o" "-ggdb"
compile_assembly "syscall.s" "syscall.o" "-ggdb"

$CC main.c -c -ggdb -nostdlib -Wall -Werror -std=c11 -o main.o
handle_errors

$LINKER start.o syscall.o main.o -o main
handle_errors


rm start.o
rm syscall.o
rm main.o
chmod +w main

