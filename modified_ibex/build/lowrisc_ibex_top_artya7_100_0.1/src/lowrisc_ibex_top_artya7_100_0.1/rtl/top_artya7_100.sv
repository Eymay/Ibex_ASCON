// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

module top_artya7_100 (
    input               IO_CLK,
    input               IO_RST_N,
    output [31:0]        LED
);

  parameter int          MEM_SIZE  = 256*1024;//64 * 1024; //32'h10800000; // 64 kB
  parameter logic [31:0] MEM_START = 32'h00000000;
  parameter logic [31:0] MEM_MASK  = MEM_SIZE-1;

  logic clk_sys, rst_sys_n;

  // Instruction connection to SRAM
  logic        instr_req;
  logic        instr_gnt;
  logic        instr_rvalid;
  logic [31:0] instr_addr;
  logic [31:0] instr_rdata;

  // Data connection to SRAM
  logic        data_req;
  logic        data_gnt;
  logic        data_rvalid;
  logic        data_we;
  logic  [3:0] data_be;
  logic [31:0] data_addr;
  logic [31:0] data_wdata;
  logic [31:0] data_rdata;

  // SRAM arbiter
  logic [31:0] mem_addr;
  logic        mem_req;
  logic        mem_write;
  logic  [3:0] mem_be;
  logic [31:0] mem_wdata;
  logic        mem_rvalid;
  logic [31:0] mem_rdata, pc_id_ila;

  logic [31:0] ram_data_top ;
  logic [31:0] ram_addr_out_ex_o;
  logic custom_enable;
  logic custom_valid;
  logic [31:0] custom_data;
  
  ibex_core #(
     .DmHaltAddr(32'h00000000),
     .DmExceptionAddr(32'h00000000)
  ) u_core (
     .clk_i                 (clk_sys),
     .rst_ni                (rst_sys_n),

     .test_en_i             ('b0),

     .hart_id_i             (32'b0),
     // First instruction executed is at 0x0 + 0x80
     .boot_addr_i           (32'h00000000),

     .instr_req_o           (instr_req),
     .instr_gnt_i           (instr_gnt),
     .instr_rvalid_i        (instr_rvalid),
     .instr_addr_o          (instr_addr),
     .instr_rdata_i         (instr_rdata),
     .instr_err_i           ('b0),
     
     .ram_data_core_i(ram_data_top),
     .ram_addr_out_ex(ram_addr_out_ex_o),
     .custom_en_ex(custom_enable),
     .custom_valid_ex(custom_valid),
     .custom_data_ex(custom_data),

     .data_req_o            (data_req),
     .data_gnt_i            (data_gnt),
     .data_rvalid_i         (data_rvalid),
     .data_we_o             (data_we),
     .data_be_o             (data_be),
     .data_addr_o           (data_addr),
     .data_wdata_o          (data_wdata),
     .data_rdata_i          (data_rdata),
     .data_err_i            ('b0),

     .irq_software_i        (1'b0),
     .irq_timer_i           (1'b0),
     .irq_external_i        (1'b0),
     .irq_fast_i            (15'b0),
     .irq_nm_i              (1'b0),

     .debug_req_i           ('b0),

     .fetch_enable_i        ('b1),
     .core_sleep_o          (),
     .pc_id_ila             (pc_id_ila)
  );

  // Connect Ibex to SRAM
  always_comb begin
    mem_req        = 1'b0;
    mem_addr       = 32'b0;
    mem_write      = 1'b0;
    mem_be         = 4'b0;
    mem_wdata      = 32'b0;
    if (instr_req) begin
      mem_req        = (instr_addr & ~MEM_MASK) == MEM_START;
      mem_addr       = instr_addr;
    end
    else if (data_req) begin
      mem_req        = (data_addr & ~MEM_MASK) == MEM_START;
      mem_write      = data_we;
      mem_be         = data_be;
      mem_addr       = data_addr;
      mem_wdata      = data_wdata;
    end
    else if (custom_enable) begin
         mem_req = 1;
         mem_addr = ram_addr_out_ex_o;
             if (custom_valid) begin
                 mem_wdata = custom_data;
                 mem_be = 'hf;
                 mem_write = 1;
             end
    end
  end

  // SRAM block for instruction and data storage
  ram_1p #(
    .Depth(MEM_SIZE / 4)
  ) u_ram (
    .clk_i     ( clk_sys        ),
    .rst_ni    ( rst_sys_n      ),
    .req_i     ( mem_req        ),
    .we_i      ( mem_write      ),
    .be_i      ( mem_be         ),
    .addr_i    ( mem_addr       ),
    .wdata_i   ( mem_wdata      ),
    .rvalid_o  ( mem_rvalid     ),
    .rdata_o   ( mem_rdata      )
  );

  // SRAM to Ibex
  assign instr_rdata    = mem_rdata;
  assign data_rdata     = mem_rdata;
  assign ram_data_top  = mem_rdata; //Custom module
  assign instr_rvalid   = mem_rvalid;
  always_ff @(posedge clk_sys or negedge rst_sys_n) begin
    if (!rst_sys_n) begin
      instr_gnt    <= 'b0;
      data_gnt     <= 'b0;
      data_rvalid  <= 'b0;
    end else begin
      instr_gnt    <= instr_req && mem_req;
      data_gnt     <= ~instr_req && data_req && mem_req;
      data_rvalid  <= ~instr_req && data_req && mem_req;
    end
  end

  // Connect the LED output to the lower four bits of the most significant
  // byte
  logic [31:0] leds;
  always_ff @(posedge clk_sys or negedge rst_sys_n) begin
    if (!rst_sys_n) begin
      leds <= 32'b0;
    end else begin
      if (mem_req && (custom_enable || (data_req && data_we))) begin
            leds <= data_wdata;
          
        end
      end
    end

  assign LED = leds;
  logic [31:0] clk_sys_counter = 32'b0;
  // Clock and reset
  
//  ila_0 ila (
//    clk_sys,
    
//    leds,
//    clk_sys_counter,
//    pc_id_ila   
  
//  );
  
  always@(posedge clk_sys) begin
    if (IO_RST_N == 1'b0) begin
        clk_sys_counter <= 32'b0;
    end
    else begin
        clk_sys_counter <= clk_sys_counter +1;
    end
  end
  
  clkgen_xil7series
    clkgen(
      .IO_CLK,
      .IO_RST_N,
      .clk_sys,
      .rst_sys_n
    );

endmodule
