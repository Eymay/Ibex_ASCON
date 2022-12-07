# Ibex ASCON Extension
Extending the Instruction Set Of RISC-V Processor for ASCON Algorithm.

# Running the processor
The project runs on only Vivado 2018.1 and its simulator, I never tried on any FPGA board and other simulators.

1. Open the `lowrisc_ibex_top_artya7_100_0.1.xpr` file in the `modified_ibex/build/lowrisc_ibex_top_artya7_100_0.1/synth-vivado` directory.

2. Modify the 65th line of the `ram_1p.sv` file to load your memory files.

3. Run the simulator.

# Compiling programs

To compile programs for the processor:

````
cd sw
make
`````
Change `Makefile` for advanced compiling options.
