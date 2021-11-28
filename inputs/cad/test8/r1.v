
// Generated by Cadence Genus(TM) Synthesis Solution 19.20-d227_1
// Generated on: Aug  3 2021 19:30:57 PDT (Aug  4 2021 02:30:57 UTC)

// Verification Directory fv/top 

module top(clk, ena, rst, Tsync, Tgdel, Tgate, Tlen, Sync, Gate, Done,
     prev_state, prev_cnt, prev_cnt_len, cnt, state, state2);
  input clk, ena, rst;
  input [7:0] Tsync, Tgdel;
  input [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  input [4:0] prev_state;
  output Sync, Gate, Done;
  output [15:0] cnt;
  output [4:0] state, state2;
  wire clk, ena, rst;
  wire [7:0] Tsync, Tgdel;
  wire [15:0] Tgate, Tlen, prev_cnt, prev_cnt_len;
  wire [4:0] prev_state;
  wire Sync, Gate, Done;
  wire [15:0] cnt;
  wire [4:0] state, state2;
  wire n_0, n_1, n_2, n_3, n_4, n_5, n_6, n_7;
  wire n_8, n_9, n_10, n_11, n_12, n_13, n_14, n_15;
  wire n_16, n_17, n_18, n_19, n_20, n_21, n_22, n_23;
  wire n_24, n_25, n_26, n_27, n_28, n_29, n_30, n_31;
  wire n_33, n_35, n_37, n_38, n_39, n_40, n_41, n_42;
  wire n_43, n_44, n_45, n_46, n_47, n_48, n_49, n_50;
  wire n_51, n_52, n_53, n_54, n_55, n_56, n_57, n_58;
  wire n_59, n_60, n_61, n_62, n_63, n_64, n_65, n_66;
  wire n_67, n_68, n_69, n_70, n_71, n_75, n_76, n_77;
  wire n_78, n_81, n_85, n_86, n_88, n_89, n_92, n_93;
  wire n_94, n_96, n_97, n_99, n_101, n_102, n_104, n_105;
  wire n_106, n_107, n_108, n_109, n_110, n_112, n_113, n_114;
  wire n_115, n_116, n_117, n_118, n_119, n_120, n_121, n_122;
  wire n_124, n_126, n_128, n_129, n_130, n_131, n_132, n_133;
  wire n_138, n_139, n_140, n_141, n_142, n_143, n_144, n_145;
  wire n_516, n_517, n_518, n_519, n_520, n_521, n_522, n_523;
  wire n_524, n_707;
  nand g7400 (n_145, n_128, n_114);
  nand g7401 (n_144, n_122, n_124);
  nand g7402 (n_143, n_113, n_116);
  nand g7403 (n_142, n_119, n_117);
  nand g7404 (n_141, n_120, n_126);
  nand g7405 (n_140, n_131, n_112);
  nand g7406 (n_139, n_115, n_121);
  nand g7407 (n_138, n_110, n_118);
  nand g7420 (n_133, n_94, n_101);
  nand g7427 (n_132, n_92, n_97);
  wire w;
  nor g7424 (n_131, w, n_99);
  and g (w, Tsync[2], n_31);
  nand g7410 (n_130, n_88, n_106);
  nand g7409 (n_129, n_108, n_86);
  wire w0;
  nor g7416 (n_128, w0, n_104);
  and g0 (w0, Tsync[5], n_31);
  wire w1, w2;
  and g7425 (n_126, w1, w2);
  nand g4 (w2, Tgate[1], n_78);
  nand g1 (w1, Tgdel[1], n_516);
  wire w3, w4;
  and g7413 (n_124, w3, w4);
  nand g6 (w4, Tgate[6], n_78);
  nand g5 (w3, Tgdel[6], n_516);
  wire w5;
  nor g7408 (n_122, w5, n_105);
  and g7 (w5, Tsync[6], n_31);
  wire w6, w7;
  and g7426 (n_121, w6, w7);
  nand g9 (w7, Tgate[3], n_78);
  nand g8 (w6, Tgdel[3], n_516);
  wire w8;
  nor g7428 (n_120, w8, n_96);
  and g10 (w8, Tsync[1], n_31);
  wire w9;
  nor g7418 (n_119, w9, n_109);
  and g11 (w9, Tsync[4], n_31);
  wire w10;
  nor g7430 (n_118, w10, n_93);
  and g12 (w10, Tgdel[0], n_516);
  wire w11, w12;
  and g7417 (n_117, w11, w12);
  nand g14 (w12, Tgate[4], n_78);
  nand g13 (w11, Tgdel[4], n_516);
  wire w13, w14;
  and g7411 (n_116, w13, w14);
  nand g16 (w14, Tgate[7], n_78);
  nand g15 (w13, Tgdel[7], n_516);
  wire w15;
  nor g7422 (n_115, w15, n_102);
  and g17 (w15, Tsync[3], n_31);
  wire w16, w17;
  and g7415 (n_114, w16, w17);
  nand g19 (w17, Tgate[5], n_78);
  nand g18 (w16, Tgdel[5], n_516);
  wire w18;
  nor g7412 (n_113, w18, n_89);
  and g20 (w18, Tsync[7], n_31);
  wire w19, w20;
  and g7423 (n_112, w19, w20);
  nand g22 (w20, Tgate[2], n_78);
  nand g21 (w19, Tgdel[2], n_516);
  wire w21, w22;
  and g7429 (n_110, w21, w22);
  nand g24 (w22, Tgate[0], n_78);
  nand g23 (w21, Tsync[0], n_31);
  nor g7442 (n_109, n_28, n_107);
  or g7437 (n_108, n_46, n_107);
  nand g7438 (n_106, Tgate[9], n_78);
  nor g7439 (n_105, n_40, n_107);
  nor g7441 (n_104, n_37, n_107);
  nor g7443 (n_102, n_22, n_107);
  nand g7444 (n_101, Tgate[12], n_78);
  nor g7446 (n_99, n_16, n_107);
  nand g7454 (n_97, Tgate[11], n_78);
  nor g7449 (n_96, n_10, n_107);
  or g7451 (n_94, n_58, n_107);
  nor g7452 (n_93, prev_cnt[0], n_107);
  or g7453 (n_92, n_55, n_107);
  nor g7432 (n_89, n_43, n_107);
  or g7433 (n_88, n_49, n_107);
  nand g7435 (n_86, Tgate[8], n_78);
  not g7456 (n_85, n_107);
  wire w23, w24;
  nand g7457 (n_107, w23, w24, n_67);
  or g26 (w24, n_13, n_25);
  or g25 (w23, prev_state[3], n_69);
  not g7460 (n_78, n_81);
  not g7466 (n_77, n_76);
  wire w25;
  nand g7468 (n_76, w25, n_33);
  or g27 (w25, n_19, n_75);
  or g7465 (n_81, prev_state[1], prev_state[3], n_1, n_75);
  nor g7472 (n_70, prev_state[2], n_75);
  nand g7473 (n_69, n_18, n_68);
  nand g7474 (n_75, n_67, n_68);
  nor g7475 (n_68, prev_cnt[15], prev_state[0], prev_state[4], n_63);
  wire w26, w27, w28, w29;
  nand g7476 (n_66, w27, w29);
  nand g31 (w29, w28, n_64);
  not g30 (w28, prev_cnt[15]);
  nand g29 (w27, w26, prev_cnt[15]);
  not g28 (w26, n_64);
  wire w30;
  nor g7477 (n_65, w30, n_64);
  and g32 (w30, prev_cnt[14], n_62);
  not g7478 (n_63, n_64);
  nor g7479 (n_64, prev_cnt[14], n_62);
  wire w31;
  nor g7480 (n_61, w31, n_60);
  and g33 (w31, prev_cnt[13], n_59);
  not g7481 (n_62, n_60);
  nor g7482 (n_60, prev_cnt[13], n_59);
  wire w32;
  nor g7483 (n_58, w32, n_57);
  and g34 (w32, prev_cnt[12], n_56);
  not g7484 (n_59, n_57);
  nor g7485 (n_57, prev_cnt[12], n_56);
  wire w33;
  nor g7486 (n_55, w33, n_54);
  and g35 (w33, prev_cnt[11], n_53);
  not g7487 (n_56, n_54);
  nor g7488 (n_54, prev_cnt[11], n_53);
  wire w34;
  nor g7489 (n_52, w34, n_51);
  and g36 (w34, prev_cnt[10], n_50);
  not g7490 (n_53, n_51);
  nor g7491 (n_51, prev_cnt[10], n_50);
  wire w35;
  nor g7492 (n_49, w35, n_48);
  and g37 (w35, prev_cnt[9], n_47);
  not g7493 (n_50, n_48);
  nor g7494 (n_48, prev_cnt[9], n_47);
  wire w36;
  nor g7495 (n_46, w36, n_45);
  and g38 (w36, prev_cnt[8], n_44);
  not g7496 (n_47, n_45);
  nor g7497 (n_45, prev_cnt[8], n_44);
  wire w37;
  nor g7503 (n_43, w37, n_42);
  and g39 (w37, prev_cnt[7], n_41);
  not g7505 (n_44, n_42);
  nor g7507 (n_42, prev_cnt[7], n_41);
  wire w38;
  nor g7509 (n_40, w38, n_39);
  and g40 (w38, prev_cnt[6], n_38);
  not g7510 (n_41, n_39);
  nor g7511 (n_39, prev_cnt[6], n_38);
  wire w39;
  nor g7512 (n_37, w39, n_35);
  and g41 (w39, prev_cnt[5], n_30);
  not g7514 (n_38, n_35);
  not g7501 (n_31, n_33);
  nor g7515 (n_35, prev_cnt[5], n_30);
  wire w40;
  nor g7504 (n_33, w40, n_29);
  and g42 (w40, n_23, n_14);
  wire w41;
  nor g7516 (n_28, w41, n_27);
  and g43 (w41, prev_cnt[4], n_26);
  not g7517 (n_30, n_27);
  and g7506 (n_29, n_71, n_67, n_17, n_24);
  nor g7519 (n_27, prev_cnt[4], n_26);
  nor g7508 (n_25, n_23, n_24);
  wire w42;
  nor g7518 (n_22, w42, n_21);
  and g44 (w42, prev_cnt[3], n_20);
  not g7522 (n_26, n_21);
  nor g7513 (n_24, prev_state[0], n_5, n_15);
  nor g7523 (n_21, prev_cnt[3], n_20);
  wire w43, w44;
  nand g7520 (n_19, w43, w44);
  or g46 (w44, n_71, n_17);
  or g45 (w43, prev_state[3], n_18);
  wire w45;
  nor g7524 (n_16, w45, n_12);
  and g47 (w45, prev_cnt[2], n_11);
  nand g7521 (n_15, n_8, n_3, n_4, n_7);
  nor g7525 (n_14, n_9, n_13);
  not g7526 (n_20, n_12);
  nor g7530 (n_12, prev_cnt[2], n_11);
  wire w46;
  nor g7527 (n_10, w46, n_2);
  and g48 (w46, prev_cnt[0], prev_cnt[1]);
  nand g7528 (n_13, n_71, n_17);
  wire w47;
  nor g7529 (n_18, w47, n_17);
  and g49 (w47, prev_state[1], prev_state[2]);
  not g7535 (n_67, n_9);
  nor g7531 (n_8, prev_cnt_len[9], prev_cnt_len[10], prev_cnt_len[12],
       prev_cnt_len[13]);
  nor g7532 (n_7, prev_cnt_len[8], prev_cnt_len[15], prev_cnt_len[11],
       prev_cnt_len[14]);
  not g7539 (n_6, n_17);
  and g7536 (n_23, prev_state[0], n_5);
  nor g7534 (n_4, prev_cnt_len[0], prev_cnt_len[1], prev_cnt_len[2],
       prev_cnt_len[3]);
  nor g7533 (n_3, prev_cnt_len[4], prev_cnt_len[5], prev_cnt_len[6],
       prev_cnt_len[7]);
  nand g7537 (n_9, ena, n_0);
  not g7538 (n_11, n_2);
  nor g7541 (n_17, prev_state[1], prev_state[2]);
  nor g7540 (n_2, prev_cnt[0], prev_cnt[1]);
  not g7544 (n_1, prev_state[2]);
  not g7545 (n_71, prev_state[3]);
  not g7542 (n_0, rst);
  not g7543 (n_5, prev_state[4]);
  assign Done = n_29;
  assign state[2] = n_516;
  assign cnt[11] = n_132;
  assign cnt[12] = n_133;
  assign cnt[13] = n_524;
  assign cnt[14] = n_522;
  assign cnt[10] = n_520;
  assign cnt[8] = n_129;
  assign cnt[9] = n_130;
  assign cnt[15] = n_518;
  assign cnt[7] = n_143;
  assign cnt[3] = n_139;
  assign cnt[2] = n_140;
  assign cnt[1] = n_141;
  assign cnt[4] = n_142;
  assign cnt[6] = n_144;
  assign cnt[5] = n_145;
  assign cnt[0] = n_138;
  assign state[4] = n_707;
  assign state2[0] = n_77;
  assign Sync = n_31;
  assign state[1] = n_31;
  assign state2[4] = n_707;
  assign state[0] = n_77;
  assign state2[1] = n_31;
  assign state2[3] = n_78;
  assign Gate = n_78;
  assign state2[2] = n_516;
  assign state[3] = n_78;
  and g2 (n_516, prev_state[1], n_71, n_70);
  not g3 (n_518, n_517);
  wire w48, w49;
  and g7813 (n_517, w48, w49);
  nand g51 (w49, n_66, n_85);
  nand g50 (w48, Tgate[15], n_78);
  wire w50;
  nand g7814 (n_520, w50, n_519);
  or g52 (w50, n_52, n_107);
  nand g7815 (n_519, Tgate[10], n_78);
  wire w51;
  nand g7816 (n_522, w51, n_521);
  or g53 (w51, n_65, n_107);
  nand g7817 (n_521, Tgate[14], n_78);
  wire w52;
  nand g7818 (n_524, w52, n_523);
  or g54 (w52, n_61, n_107);
  nand g7819 (n_523, Tgate[13], n_78);
  nor g8002 (n_707, n_71, n_6, n_75);
endmodule

