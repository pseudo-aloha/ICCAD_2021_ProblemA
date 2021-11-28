module top(h, i, l, m, n, a, b, c, d, e);
input a, b, c, d, e;
output h, i, l, m, n;
assign m = 1'b0;
assign n = 1'b0;
or 	g1(k, a, 1'b0);
and g2(j, b, c);
or 	g3(h, c, b);
and g4(i, d, e);
and g5(l, k, j);
endmodule