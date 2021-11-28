module top(o, a, b, c, d, e, f, g, h);
output o;
input a, b, c, d, e, f, g, h;
nand o(o, n6, n7);
or n6(n6, n1, c);
or n7(n7, n4, n5);
nand n4(n4, n2, f);
not n5(n5, n3);
or n1(n1, a, b);
or n2(n2, d, e);
or n3(n3, g, h);
endmodule
