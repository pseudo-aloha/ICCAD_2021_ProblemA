
// Generated by Cadence Genus Synthesis Solution GENUS15.13 - 15.10-s038_1

// Verification Directory fv/top 

module top(clk, ena, rst, Tsync, Tgdel, Tgate, Tlen, Sync, Gate, Done,
     prev_state, prev_cnt, prev_cnt_len, cnt, state);
  input clk, ena, rst;
  input [7:0] Tsync, Tgdel;
  input [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  input [4:0] prev_state;
  output Sync, Gate, Done;
  output [15:0] cnt;
  output [4:0] state;
  wire clk, ena, rst;
  wire [7:0] Tsync, Tgdel;
  wire [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  wire [4:0] prev_state;
  wire Sync, Gate, Done;
  wire [15:0] cnt;
  wire [4:0] state;
  wire n_15, n_23, n_27, n_28, n_29, n_376, n_377, n_472;
  wire n_473, n_474, n_475, n_476, n_477, n_478, n_479, n_480;
  assign state[1] = Sync;
  assign state[2] = 1'b0;
  assign state[3] = 1'b0;
  assign state[4] = 1'b0;
  assign Done = 1'b0;
  assign Gate = 1'b0;
  CDN_mux3 mux_cnt_125_11_g257(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[2]), .sel2 (n_377), .data2 (prev_cnt[2]),
       .z (cnt[2]));
  CDN_mux3 mux_cnt_125_11_g260(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[1]), .sel2 (n_377), .data2 (prev_cnt[1]),
       .z (cnt[1]));
  CDN_mux3 mux_cnt_125_11_g263(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[0]), .sel2 (n_377), .data2 (prev_cnt[0]),
       .z (cnt[0]));
  CDN_mux3 mux_cnt_125_11_g287(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[7]), .sel2 (n_377), .data2 (prev_cnt[7]),
       .z (cnt[7]));
  CDN_mux3 mux_cnt_125_11_g293(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[6]), .sel2 (n_377), .data2 (prev_cnt[6]),
       .z (cnt[6]));
  CDN_mux3 mux_cnt_125_11_g296(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[5]), .sel2 (n_377), .data2 (prev_cnt[5]),
       .z (cnt[5]));
  CDN_mux3 mux_cnt_125_11_g299(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[4]), .sel2 (n_377), .data2 (prev_cnt[4]),
       .z (cnt[4]));
  CDN_mux3 mux_cnt_125_11_g302(.sel0 (n_376), .data0 (1'b0), .sel1
       (Sync), .data1 (Tsync[3]), .sel2 (n_377), .data2 (prev_cnt[3]),
       .z (cnt[3]));
  not g10 (state[0], Sync);
  nor g31 (Sync, n_15, prev_state[4], prev_state[1], rst);
  not g339 (n_472, rst);
  not g340 (n_473, prev_state[1]);
  not g341 (n_474, prev_state[4]);
  not g342 (n_475, prev_state[2]);
  not g343 (n_476, prev_state[3]);
  nand g344 (n_23, n_476, n_475, prev_state[0]);
  not g345 (n_477, n_23);
  nand g346 (n_15, n_477, ena);
  nand g347 (n_376, ena, n_472);
  nor g348 (n_28, n_376, n_474);
  not g349 (n_478, n_28);
  nor g350 (n_27, n_376, n_473);
  not g351 (n_479, n_27);
  nor g352 (n_29, n_477, n_376);
  not g353 (n_480, n_29);
  nand g354 (n_377, n_478, n_479, n_480);
  and g355 (cnt[14], n_377, prev_cnt[14]);
  and g356 (cnt[13], n_377, prev_cnt[13]);
  and g357 (cnt[12], n_377, prev_cnt[12]);
  and g358 (cnt[11], n_377, prev_cnt[11]);
  and g359 (cnt[10], n_377, prev_cnt[10]);
  and g360 (cnt[9], n_377, prev_cnt[9]);
  and g361 (cnt[8], n_377, prev_cnt[8]);
  and g362 (cnt[15], n_377, prev_cnt[15]);
endmodule

`ifdef RC_CDN_GENERIC_GATE
`else
`ifdef ONE_HOT_MUX // captures one-hot property of select inputs
module CDN_mux3(sel0, data0, sel1, data1, sel2, data2, z);
  input sel0, data0, sel1, data1, sel2, data2;
  output z;
  wire sel0, data0, sel1, data1, sel2, data2;
  reg  z;
  always 
    @(sel0 or sel1 or sel2 or data0 or data1 or data2) 
      case ({sel0, sel1, sel2})
       3'b100: z = data0;
       3'b010: z = data1;
       3'b001: z = data2;
       default: z = 1'bX;
      endcase
endmodule
`else
module CDN_mux3(sel0, data0, sel1, data1, sel2, data2, z);
  input sel0, data0, sel1, data1, sel2, data2;
  output z;
  wire sel0, data0, sel1, data1, sel2, data2;
  wire z;
  wire w_0, w_1, w_2;
  and a_0 (w_0, sel0, data0);
  and a_1 (w_1, sel1, data1);
  and a_2 (w_2, sel2, data2);
  or org (z, w_0, w_1, w_2);
endmodule
`endif // ONE_HOT_MUX
`endif
