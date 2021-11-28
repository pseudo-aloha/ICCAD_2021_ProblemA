module top(in0, in1, out);
input in0, in1;
output out;
wire x, y, t;
not n0(x, in0); 
buf n1(y, in1);
and eco1(t, x, y); 
not n3(out, t); 
endmodule