module top_eco(a,b,c,d,e,,g,h);
input b,c,d,e,g,h;
output a;
and g1(a,b,c,d,e,f);
xor g2(f,g,h);
endmodule