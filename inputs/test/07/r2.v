module top_eco(a,b,c,d,e,,g,h);
input b,c,d,e,g,h;
output a;
and g1(a,i,f);
nand g3(i,b,c,d,e);
xor g2(f,g,h);
endmodule