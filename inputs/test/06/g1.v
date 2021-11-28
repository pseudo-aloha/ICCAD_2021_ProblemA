module top( h, i, l, m, n, a, b, c, d, e);
input a, b, c, d, e;
output h, i, l, m, n;
assign m = 1'b1;
assign n = 1'b0;
or 	g1(k, a, e);
and g2(j, b, e);
or 	g3(h, c, e);
and g4(i, d, e);
and g5(l, k, j);
endmodule