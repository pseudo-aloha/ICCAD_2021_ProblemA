module top(o, a, b, c, d, e, f);
output o;
input a, b, c, d, e, f;
or g0( g0, a, b );
and g1( g1, c, d );
and g2( g2, e, f );
and g3( g3, g0, g1 );
or g4( o, g3, g2);
endmodule
