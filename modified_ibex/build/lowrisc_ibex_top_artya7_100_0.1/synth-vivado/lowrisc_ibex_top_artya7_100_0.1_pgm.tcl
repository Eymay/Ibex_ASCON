# Auto-generated program tcl file

set bit lowrisc_ibex_top_artya7_100_0.1.bit
set part xc7a100tcsg324-1

open_hw
connect_hw_server

set found 0

foreach { hw_target } [get_hw_targets] {
    current_hw_target $hw_target
    open_hw_target
    foreach { hw_device } [get_hw_devices] {
	if { [string first [get_property PART $hw_device] $part] == 0 } {
	    puts "Found hardware target with a ${part} device."
	    current_hw_device $hw_device
	    set found 1
	    break
	}
    }
    if {$found} {break}
    close_hw_target
}
if { $found == 0 } {
    puts "Did not find board with a ${part} device."
    exit 1
}

set_property PROGRAM.FILE $bit [current_hw_device]
program_hw_devices [current_hw_device]
disconnect_hw_server

