module top(k, l, a, b, c, d, e);
output k, l;
input a, b, c, d, e;
or g5(l, f1, i1);
or g4(k, b, c);
and g3(i1, e, g1);
and g2(f1, a, b);
or g1(g1, c, d);
endmodule
