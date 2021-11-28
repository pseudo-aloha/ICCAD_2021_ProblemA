module top(o, a, b, c);
output o;
input a, b, c;
and g2(o, a, n1);
and g1(n1, b, c);
endmodule
