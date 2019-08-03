#!/bin/sh
#\
exec tclsh "$0" "$@"

if {$argc < 1} {
    puts "Usage: [file tail [info script]] <DIR>"
    puts ""
    exit 1
}


set DIR [lindex $argv 0]


set dirmtime [file mtime $DIR]
set dirs [list $DIR]

proc scandir {initdir} {
    foreach file [glob -nocomplain -directory $initdir *] {
        if {[file mtime $file] > $::dirmtime} {
            set ::dirmtime [file mtime $file]
        }

        if {[file isdirectory $file]} {
            lappend ::dirs $file
            scandir $file
        } else {

        }
    }
}

scandir $DIR

if {[file mtime $DIR] < $dirmtime} {
    puts "Update timestamps"
    foreach dir [lreverse $dirs] {
        if {[file mtime $dir] < $dirmtime} {
            puts "Touch: $dir"
            file mtime $dir $dirmtime
        }
    }
}

