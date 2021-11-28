module top_eco(g1, c, e, b, i1, k);
input g1, c, e, b;
output i1, k;
wire g1, c, e, b, i1, k;
and eco_g0(i1, e, c);
or eco_g1(k, b, g1);
endmodule
