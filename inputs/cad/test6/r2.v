
// Generated by Cadence Genus(TM) Synthesis Solution 19.20-d227_1
// Generated on: Aug  3 2021 02:58:51 PDT (Aug  3 2021 09:58:51 UTC)

// Verification Directory fv/top 

module top(clk, ena, rst, Tsync, Tgdel, Tgate, Tlen, Sync, Gate, Done,
     prev_state, prev_cnt, prev_cnt_len, cnt);
  input clk, ena, rst;
  input [7:0] Tsync, Tgdel;
  input [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  input [4:0] prev_state;
  output Sync, Gate, Done;
  output [15:0] cnt;
  wire clk, ena, rst;
  wire [7:0] Tsync, Tgdel;
  wire [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  wire [4:0] prev_state;
  wire Sync, Gate, Done;
  wire [15:0] cnt;
  wire n_0, n_1, n_2, n_3, n_5, n_6, n_7, n_8;
  wire n_9, n_11, n_13, n_15, n_16, n_17, n_18, n_19;
  wire n_21, n_22, n_27, n_28, n_30, n_32, n_33, n_36;
  wire n_231, n_232, n_233, n_234, n_337, n_338, n_458, n_459;
  wire n_562, n_563;
  assign Done = 1'b0;
  assign Gate = 1'b0;
  nand g1757 (n_36, n_30, n_6);
  nand g1759 (n_33, n_22, n_3);
  nand g1760 (n_32, n_27, n_5);
  nand g1766 (n_30, prev_cnt[7], n_28);
  nand g1775 (n_27, prev_cnt[3], n_28);
  nand g1784 (n_22, prev_cnt[2], n_28);
  and g1772 (n_21, ena, prev_cnt[13], n_16, n_17);
  and g1769 (n_18, ena, prev_cnt[15], n_16, n_17);
  and g1763 (n_15, ena, prev_cnt[14], n_16, n_17);
  and g1774 (n_13, ena, prev_cnt[12], n_16, n_17);
  and g1780 (n_11, ena, prev_cnt[11], n_16, n_17);
  and g1777 (n_9, ena, prev_cnt[10], n_16, n_17);
  and g1782 (n_8, ena, prev_cnt[9], n_16, n_17);
  and g1786 (n_7, ena, prev_cnt[8], n_16, n_17);
  nand g1770 (n_6, Tsync[7], n_19);
  nand g1783 (n_5, Tsync[3], n_19);
  nand g1785 (n_3, Tsync[2], n_19);
  and g1787 (n_28, ena, n_16, n_17);
  and g1788 (n_19, ena, n_16, n_1, n_2);
  nand g1789 (n_17, n_1, n_2);
  nor g1790 (n_2, prev_state[3], prev_state[4], n_0);
  nor g1791 (n_1, prev_state[1], prev_state[2]);
  not g1792 (n_16, rst);
  not g1793 (n_0, prev_state[0]);
  assign Sync = n_19;
  assign cnt[11] = n_11;
  assign cnt[13] = n_21;
  assign cnt[12] = n_13;
  assign cnt[8] = n_7;
  assign cnt[14] = n_15;
  assign cnt[9] = n_8;
  assign cnt[15] = n_18;
  assign cnt[10] = n_9;
  assign cnt[7] = n_36;
  assign cnt[5] = n_232;
  assign cnt[0] = n_563;
  assign cnt[2] = n_33;
  assign cnt[1] = n_459;
  assign cnt[4] = n_338;
  assign cnt[6] = n_234;
  assign cnt[3] = n_32;
  not g3 (n_232, n_231);
  wire w, w0;
  and g2 (n_231, w, w0);
  nand g0 (w0, prev_cnt[5], n_28);
  nand g (w, Tsync[5], n_19);
  not g1951 (n_234, n_233);
  wire w1, w2;
  and g1952 (n_233, w1, w2);
  nand g4 (w2, prev_cnt[6], n_28);
  nand g1 (w1, Tsync[6], n_19);
  not g2055 (n_338, n_337);
  wire w3, w4;
  and g2056 (n_337, w3, w4);
  nand g6 (w4, prev_cnt[4], n_28);
  nand g5 (w3, Tsync[4], n_19);
  not g2159 (n_459, n_458);
  wire w5, w6;
  and g2160 (n_458, w5, w6);
  nand g8 (w6, prev_cnt[1], n_28);
  nand g7 (w5, Tsync[1], n_19);
  not g2263 (n_563, n_562);
  wire w7, w8;
  and g2264 (n_562, w7, w8);
  nand g10 (w8, prev_cnt[0], n_28);
  nand g9 (w7, Tsync[0], n_19);
endmodule
