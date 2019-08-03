#/bin/sh
#\
exec tclsh "$0" "$@"

if {$argc < 2} {
    puts "Usage:"
    puts "    [file tail [info script]] <INPUT DIR> <OUTPUT FILE>"
    exit 1
}

set DIR   [lindex $argv 0]
set IMAGE [lindex $argv 1]

puts "DIR  : $DIR"
puts "IMAGE: $IMAGE"

if {![file exists $DIR] || ![file isdirectory $DIR]} {
    puts "File \"$DIR\" is not directory or not exists."
    exit 1
}

set ::FILES [list]

proc scandir {initdir} {
    foreach file [glob -nocomplain -directory $initdir *] {
        if {[file isdirectory $file]} {
            scandir $file
        } else {
            lappend ::FILES $file
        }
    }
}

scandir $DIR

set ofd [open $IMAGE w]
fconfigure $ofd -translation binary -encoding binary

set HEAD_SIZE    512
set NAME_ALIGN   4 
set ALIGN        512

puts -nonewline $ofd "-MROMFS1"

seek $ofd $HEAD_SIZE start

# XXX Limit HEAD of file to 512 bytes.

set offset $HEAD_SIZE
for {set i 0} {$i < [llength $::FILES]} {incr i} {
    set file [lindex $::FILES $i]

    set name [string range $file [string length $::DIR]+1 end]

    puts "FILE: $file -> $name"

#    set slen [string length $name]
#
#    append name [string repeat "\0" [expr {$ALIGN - ($slen % $ALIGN)}]]
#
#    set next $offset
#    incr next 4 ; # next
#    incr next 4 ; # size
#    incr next 4 ; # offset
#    incr next [string length $name]
#    incr next [file size $file]
#    if {$next % $ALIGN} {
#        incr next [expr {$ALIGN - ($next % $ALIGN)}]
#    }
#
#    if {$i == ([llength $::FILES] - 1)} {
#        puts -nonewline $ofd [binary format i 0]
#    } else {
#        puts -nonewline $ofd [binary format i $next]
#    }
#    puts -nonewline $ofd [binary format i [file size $file]]
#    puts -nonewline $ofd [binary format i [expr {$offset + 4 + 4 + 4 + [string length $name]}]]
#    puts -nonewline $ofd [binary format a* $name]

    set next $offset
    incr next $ALIGN
    incr next [file size $file]
    if {$next % $ALIGN} {
        incr next [expr {$ALIGN - ($next % $ALIGN)}]
    }

    append name [string repeat "\0" [expr {$NAME_ALIGN - ([string length $name] % $NAME_ALIGN)}]]

    # next
    if {$i == ([llength $::FILES] - 1)} {
        puts -nonewline $ofd [binary format i 0]
    } else {
        puts -nonewline $ofd [binary format i $next]
    }
    # file size
    puts -nonewline $ofd [binary format i [file size $file]]
    # offset of file data
    puts -nonewline $ofd [binary format i [expr {$offset + $ALIGN}]]
    # name
    puts -nonewline $ofd [binary format a* $name]

    seek $ofd [expr {$offset + $ALIGN}] start

    if {true} {
        set fd [open $file r]
        fconfigure $fd -translation binary -encoding binary
        puts -nonewline $ofd [read $fd]
        close $fd
    }

    if {$next != 0} {
        seek $ofd $next start
    }

    set offset $next
}


close $ofd



