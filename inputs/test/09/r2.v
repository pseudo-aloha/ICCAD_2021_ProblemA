module top(clk, a, b, op, oe, y, parity, overflow, greater, is_eq,
less);
input clk, oe;
input [7:0] a, b;
input [1:0] op;
output [7:0] y;
output parity, overflow, greater, is_eq, less;
assign overflow = y[7];
assign y[6] = y[7];
or g738 (n_523, wc, a[3]);
not gc (wc, b[3]);
nand g1253 (n_540_in, b[0], a[0]);
or g1320 (n_528, wc0, a[0]);
not gc0 (wc0, b[0]);
or g1321 (n_568, wc1, a[7]);
not gc1 (wc1, b[7]);
or g1872 (n_520_in, wc2, op[1]);
not gc2 (wc2_in, op[0]);
or g1873 (n_522, wc3, a[4]);
not gc3 (wc3, b[4]);
or g1874 (n_525, wc4, a[2]);
not gc4 (wc4, b[2]);
or g1875 (n_526, wc5, a[1]);
not gc5 (wc5, b[1]);
or g1877 (n_534, wc6, a[5]);
not gc6 (wc6, b[5]);
or g1890 (n_569, b[7], wc7);
not gc7 (wc7, a[7]);
or g1891 (n_570, wc8, a[6]);
not gc8 (wc8, b[6]);
or g1892 (n_571, b[6], wc9);
not gc9 (wc9, a[6]);
or g1937 (n_590, a[4], b[4]);
or g1945 (n_594, a[3], b[3]);
or g1947 (n_595, a[1], b[1]);
or g1949 (n_596, a[2], b[2]);
or g2120 (n_854, wc10, n_559);
not gc10 (wc10, n_566);
or g2121 (n_855, n_566, wc11);
not gc11 (wc11, n_559);
nand g2122 (parity, n_854, n_855);
nand g2125 (less, n_568, n_787);
or g2126 (n_857, wc12, n_561);
not gc12 (wc12, n_565);
or g2127 (n_858, n_565, wc13);
not gc13 (wc13, n_561);
nand g2128 (n_566, n_857, n_858);
nand g2129 (n_859, n_563, n_564);
or g2130 (n_860, n_563, n_564);
nand g2131 (n_565, n_859, n_860);
nand g2134 (n_787, n_591, n_569);
or g2135 (n_559, wc14, n_784);
not gc14 (wc14, n_783);
or g2138 (n_766, n_765, wc15);
not gc15 (wc15, n_568);
nand g2141 (n_784, n_781, n_782);
nand g2144 (n_591, n_570, n_769);
nand g2147 (n_783_in, n_558, n_603);
or g2148 (n_866, wc16, n_549);
not gc16 (wc16, n_546);
or g2149 (n_867_in, n_546, wc17);
not gc17 (wc17_in, n_549);
nand g2150 (n_564, n_866, n_867);
or g2153 (n_537_in, n_520, n_536);
not g2154 (y[7], n_537);
or g2155 (n_769, n_536, wc18);
not gc18 (wc18_in, n_571);
or g2156 (n_765, n_573, wc19);
not gc19 (wc19, n_536);
or g2157 (n_546, wc20, n_838);
not gc20 (wc20, n_837);
or g2158 (n_781_in, wc21, n_558);
not gc21 (wc21, n_604);
or g2159 (n_603, n_751, wc22);
not gc22 (wc22, n_544);
nand g2160 (n_604, n_756, n_757);
nand g2161 (n_751, n_749, n_750);
nand g2162 (n_536_in, n_535, n_760);
nand g2165 (n_838, n_835, n_836);
nand g2166 (n_871_in, n_555, n_552);
or g2167 (n_872_in, n_555, n_552);
nand g2168 (n_563, n_871, n_872);
nand g2169 (n_837_in, n_545, n_601);
or g2170 (n_601_in, n_727, wc23);
not gc23 (wc23, n_544);
or g2171 (n_835_in, wc24, n_545);
not gc24 (wc24_in, n_602);
or g2172 (n_573, n_709, wc25);
not gc25 (wc25, n_569);
nand g2173 (n_760_in, n_533, n_534);
or g2174 (n_555, wc26, n_826);
not gc26 (wc26, n_825);
or g2175 (n_750, n_520, wc27);
not gc27 (wc27, n_533);
or g2176 (n_749, n_538, n_557);
or g2177 (n_757_in, n_538, wc28);
not gc28 (wc28, n_557);
or g2178 (n_756_in, n_520, n_533);
nand g2179 (n_825, n_554, n_599);
or g2180 (n_709, n_708, wc29);
not gc29 (wc29, n_531);
nand g2181 (n_826, n_823, n_824);
nand g2182 (n_533_in, n_521, n_742);
nand g2183 (n_602, n_732, n_733);
nand g2184 (n_557, n_738, n_739);
nand g2185 (n_727, n_725, n_726);
or g2186 (n_599, n_691, wc30);
not gc30 (wc30, n_544);
or g2187 (n_552, wc31, n_814);
not gc31 (wc31, n_813);
or g2188 (n_726, n_520, wc32);
not gc32 (wc32, n_532);
or g2189 (n_732, n_520, n_532);
or g2190 (n_733, n_538, wc33);
not gc33 (wc33, n_543);
or g2191 (n_823, wc34, n_554);
not gc34 (wc34, n_600);
or g2192 (n_725, n_538, n_543);
nand g2193 (n_738, n_590, n_543);
or g2194 (n_708, n_707, wc35);
not gc35 (wc35, n_521);
nand g2195 (n_742_in, n_522, n_532);
nand g2196 (n_532, n_531, n_712);
nand g2197 (n_543, n_717, n_718);
or g2198 (n_707, wc36, n_577);
not gc36 (wc36, n_706);
nand g2199 (n_600, n_696, n_697);
nand g2200 (n_691, n_689, n_690);
nand g2201 (n_813_in, n_551, n_607);
nand g2202 (n_814, n_811, n_812);
nand g2203 (n_706_in, n_610, n_523);
nand g2204 (n_593, n_569, n_682);
nand g2205 (n_717, n_594, n_542);
or g2206 (n_549, wc37, n_802);
not gc37 (wc37, n_801);
or g2207 (n_607, n_652, wc38);
not gc38 (wc38, n_544);
nand g2208 (n_712, n_523, n_530);
or g2209 (n_696_in, n_520, n_530);
or g2210 (n_811_in, wc39, n_551);
not gc39 (wc39, n_608);
or g2211 (n_690, n_520, wc40);
not gc40 (wc40, n_530);
or g2212 (n_689_in, n_538, n_542);
or g2213 (n_697_in, n_538, wc41);
not gc41 (wc41, n_542);
nand g2214 (n_610, n_524, n_661);
nand g2215 (n_542_in, n_675, n_676);
nand g2216 (n_801_in, n_548, n_605);
nand g2217 (n_802, n_799, n_800);
nand g2218 (n_530, n_524, n_670);
nand g2219 (n_652, n_650, n_651);
or g2220 (n_682, n_681, wc42);
not gc42 (wc42, n_570);
nand g2221 (n_608, n_657, n_658);
nand g2222 (n_675_in, n_596, n_541);
nand g2223 (n_670, n_525, n_529);
nand g2224 (n_661_in, n_609, n_525);
or g2225 (n_658, n_538, wc43);
not gc43 (wc43, n_541);
or g2226 (n_657, n_520, n_529);
or g2227 (n_799_in, wc44, n_548);
not gc44 (wc44, n_606);
nand g2228 (n_561, n_666, n_667);
or g2229 (n_651, n_520, wc45);
not gc45 (wc45, n_529);
or g2230 (n_605, n_625, wc46);
not gc46 (wc46, n_544);
or g2231 (n_650_in, n_538, n_541);
nand g2232 (n_681, n_592, n_568);
nand g2233 (n_625, n_623, n_624);
nand g2234 (n_529, n_527, n_634);
or g2235 (n_592, n_577, wc47);
not gc47 (wc47, n_643);
nand g2236 (n_666_in, n_598, n_539);
nand g2237 (n_609_in, n_527, n_790);
nand g2238 (n_606, n_630, n_631);
nand g2239 (n_541, n_639, n_640);
or g2240 (n_624, n_520, wc48);
not gc48 (wc48, n_528);
nand g2241 (n_643, n_522, n_534);
nand g2242 (n_548, n_526, n_527);
or g2243 (n_630, n_520, n_528);
or g2244 (n_631, n_538, n_540);
or g2245 (n_782_in, n_780, n_539);
or g2246 (n_836_in, n_739, n_539);
or g2247 (n_800_in, n_640, n_539);
or g2248 (n_790, n_616, wc49);
not gc49 (wc49_in, n_526);
nand g2249 (n_558, n_534, n_535);
nand g2250 (n_598, n_528, n_616);
or g2251 (n_824, n_718, n_539);
or g2252 (n_667_in, n_539, n_540);
nand g2253 (n_551, n_524, n_525);
nand g2254 (n_554_in, n_523, n_531);
nand g2255 (n_545, n_521, n_522);
or g2256 (n_639, wc50, n_540);
not gc50 (wc50, n_595);
or g2257 (n_812, n_676, n_539);
nand g2258 (n_634, n_526, n_528);
nand g2259 (n_577, n_535, n_571);
or g2260 (n_623, n_538, wc51);
not gc51 (wc51, n_540);
nand g2261 (n_676, a[2], b[2]);
nand g2262 (n_718, a[3], b[3]);
nand g2263 (n_640, a[1], b[1]);
or g2264 (n_538, op[1], op[0]);
nand g2265 (n_544_in, op[0], op[1]);
or g2266 (n_616_in, wc52, b[0]);
not gc52 (wc52, a[0]);
nand g2267 (n_739, a[4], b[4]);
or g2268 (n_539, wc53, op[0]);
not gc53 (wc53, op[1]);
nand g2269 (n_780_in, a[5], b[5]);
or g2270 (n_527, wc54, b[1]);
not gc54 (wc54, a[1]);
or g2271 (n_524, wc55, b[2]);
not gc55 (wc55, a[2]);
or g2272 (n_531, wc56, b[3]);
not gc56 (wc56, a[3]);
or g2273 (n_521, wc57, b[4]);
not gc57 (wc57, a[4]);
or g2274 (n_535, wc58, b[5]);
not gc58 (wc58, a[5]);
and g2275 (greater, n_593, n_573);
and g2276 (y[4]_in, n_546, wc59);
not gc59 (wc59, n_537);
and g2277 (y[0]_in, n_561, wc60);
not gc60 (wc60, n_537);
and g2278 (y[1]_in, n_549, wc61);
not gc61 (wc61, n_537);
and g2279 (y[2], n_552, wc62);
not gc62 (wc62, n_537);
and g2280 (y[3], n_555, wc63);
not gc63 (wc63, n_537);
and g2281 (is_eq, wc64, n_570);
not gc64 (wc64, n_766);
and g2282 (y[5]_in, wc65, n_559);
not gc65 (wc65, n_537);
or eco_gb0(n_616, wc52, eco_w286);
buf eco_ga1(n_666, eco_w322);
nand eco_gb2(n_540, eco_w324, 1'b1);
or eco_gb3(n_667, eco_w325, n_540);
buf eco_ga4(wc2, eco_w369);
or eco_gb5(n_520, eco_w369, eco_w368);
buf eco_ga6(n_742, eco_w510);
nand eco_gb7(n_533, eco_w509, eco_w510);
nand eco_gb8(n_760, eco_w395, eco_w396);
nand eco_gb9(n_536, eco_w377, n_760);
or eco_gb10(n_537, n_520, eco_w351);
and eco_gb11(y[0], eco_w311, wc60);
nand eco_gb12(n_544, op[0], n_554_in);
buf eco_ga13(n_801, 1'b1);
buf eco_ga14(n_799, 1'b1);
buf eco_ga15(n_800, op[1]);
and eco_gb16(y[1], eco_w324, wc61);
buf eco_ga17(n_650, op[1]);
nand eco_gb18(n_813, eco_w390, n_607);
buf eco_ga19(n_811, 1'b1);
buf eco_ga20(n_554, op[1]);
buf eco_ga21(n_675, eco_w407);
nand eco_gb22(n_542, eco_w407, eco_w403);
or eco_gb23(n_689, n_538, 1'b0);
buf eco_ga24(n_696, eco_w359);
or eco_gb25(n_697, op[0], wc41);
buf eco_ga26(n_601, op[0]);
nand eco_gb27(n_837, n_551, op[0]);
buf eco_ga28(wc24, n_676);
or eco_gb29(n_835, n_676, op[0]);
buf eco_ga30(n_836, 1'b1);
and eco_gb31(y[4], eco_w325, wc59);
buf eco_ga32(n_783, 1'b1);
or eco_gb33(n_756, eco_w489, n_533);
buf eco_ga34(n_757, eco_w474);
or eco_gb35(n_781, wc21, eco_w310);
buf eco_ga36(n_780, eco_w313);
or eco_gb37(n_782, eco_w313, eco_w311);
and eco_gb38(y[5], wc65, eco_w310);
nand eco_gb39(n_871, n_555, 1'b0);
or eco_gb40(n_872, n_555, 1'b0);
not eco_gb41(wc17, eco_w390);
or eco_gb42(n_867, op[1], wc17);
not eco_gb43(wc49, 1'b1);
nand eco_gb44(n_609, wc54, n_790);
nand eco_gb45(n_661, n_609, eco_w472);
nand eco_gb46(n_706, n_610, eco_w430);
buf eco_ga47(wc18, 1'b0);
nand eco_gc48(eco_w548, n_676, n_718);
nand eco_gc49(eco_w547, n_594, eco_w548);
or eco_gc50(eco_w511, wc1, clk);
nand eco_gc51(eco_w512, b[1], clk);
nand eco_gc52(eco_w293, eco_w511, eco_w512);
nand eco_gc53(eco_w465, eco_w293, a[1]);
or eco_gc54(eco_w294, eco_w293, a[1]);
not eco_gc55(eco_w479, eco_w294);
or eco_gc56(eco_w509, wc8, clk);
nand eco_gc57(eco_w510, b[0], clk);
nand eco_gc58(eco_w286, eco_w509, eco_w510);
nand eco_gc59(eco_w285, a[0], eco_w286);
or eco_gc60(eco_w466, eco_w479, eco_w285);
nand eco_gc61(eco_w439, eco_w465, eco_w466);
and eco_gc62(eco_w564, n_594, n_596);
nand eco_gc63(eco_w567, eco_w439, eco_w564);
nand eco_gc64(eco_w397, eco_w547, eco_w567);
nand eco_gc65(eco_w392, eco_w397, n_590);
nand eco_gc66(eco_w362, n_739, eco_w392);
not eco_gc67(eco_w361, eco_w362);
or eco_gc68(eco_w360, eco_w361, n_558);
not eco_gc69(eco_w364, n_558);
or eco_gc70(eco_w363, eco_w362, eco_w364);
nand eco_gc71(eco_w348, eco_w360, eco_w363);
not eco_gc72(eco_w347, eco_w348);
or eco_gc73(eco_w340, eco_w347, op[0]);
nand eco_gc74(eco_w546, n_531, n_524);
nand eco_gc75(eco_w425, eco_w546, n_523);
or eco_gc76(eco_w467, eco_w293, wc54);
not eco_gc77(eco_w292, eco_w293);
or eco_gc78(eco_w291, eco_w292, a[1]);
nand eco_gc79(eco_w455, wc52, eco_w286);
nand eco_gc80(eco_w468, eco_w291, eco_w455);
nand eco_gc81(eco_w441, eco_w467, eco_w468);
nand eco_gc82(eco_w545, n_523, n_525);
not eco_gc83(eco_w438, eco_w545);
nand eco_gc84(eco_w426, eco_w441, eco_w438);
nand eco_gc85(eco_w298, eco_w425, eco_w426);
nand eco_gc86(eco_w375, eco_w298, n_522);
nand eco_gc87(eco_w355, n_521, eco_w375);
nand eco_gc88(eco_w354, eco_w355, n_558);
or eco_gc89(eco_w357, eco_w355, n_558);
nand eco_gc90(eco_w342, eco_w354, eco_w357);
nand eco_gc91(eco_w341, eco_w342, op[0]);
nand eco_gc92(eco_w335, eco_w340, eco_w341);
not eco_gc93(eco_w334, eco_w335);
or eco_gc94(eco_w314, op[1], eco_w334);
or eco_gc95(eco_w494, op[0], n_780_in);
nand eco_gc96(eco_w495, n_558, op[0]);
nand eco_gc97(eco_w483, eco_w494, eco_w495);
nand eco_gc98(eco_w315, eco_w483, op[1]);
nand eco_gc99(eco_w310, eco_w314, eco_w315);
not eco_gc100(eco_w414, eco_w397);
or eco_gc101(eco_w413, eco_w414, n_545);
not eco_gc102(eco_w566, n_545);
or eco_gc103(eco_w416, eco_w397, eco_w566);
nand eco_gc104(eco_w371, eco_w413, eco_w416);
not eco_gc105(eco_w370, eco_w371);
or eco_gc106(eco_w349, eco_w370, op[0]);
nand eco_gc107(eco_w417, eco_w298, n_545);
or eco_gc108(eco_w418, eco_w298, n_545);
nand eco_gc109(eco_w388, eco_w417, eco_w418);
nand eco_gc110(eco_w350, eco_w388, op[0]);
nand eco_gc111(eco_w344, eco_w349, eco_w350);
not eco_gc112(eco_w343, eco_w344);
or eco_gc113(eco_w338, op[1], eco_w343);
or eco_gc114(eco_w490, op[0], n_739);
nand eco_gc115(eco_w491, n_545, op[0]);
nand eco_gc116(eco_w486, eco_w490, eco_w491);
nand eco_gc117(eco_w339, eco_w486, op[1]);
nand eco_gc118(eco_w325, eco_w338, eco_w339);
or eco_gc119(eco_w492, n_539, wc52);
not eco_gc120(eco_w513, n_539);
or eco_gc121(eco_w493, eco_w513, a[0]);
nand eco_gc122(eco_w477, eco_w492, eco_w493);
nand eco_gc123(eco_w474, eco_w477, eco_w286);
or eco_gc124(eco_w520, b[5], a[5]);
and eco_gc125(eco_w398, eco_w520, n_590);
nand eco_gc126(eco_w381, eco_w397, eco_w398);
not eco_gc127(eco_w379, eco_w381);
nand eco_gc128(eco_w552, n_739, n_780_in);
nand eco_gc129(eco_w516, eco_w520, eco_w552);
not eco_gc130(eco_w515, eco_w516);
or eco_gc131(eco_w380, eco_w515, op[0]);
or eco_gc132(eco_w353, eco_w379, eco_w380);
not eco_gc133(eco_w352, eco_w353);
or eco_gc134(eco_w351, eco_w352, op[1]);
nand eco_gc135(eco_w289, eco_w286, wc52);
nand eco_gc136(eco_w475, eco_w289, a[1]);
or eco_gc137(eco_w504, a[1], a[0]);
not eco_gc138(eco_w505, eco_w286);
or eco_gc139(eco_w481, eco_w504, eco_w505);
not eco_gc140(eco_w480, eco_w481);
or eco_gc141(eco_w476, eco_w480, eco_w293);
nand eco_gc142(eco_w437, eco_w475, eco_w476);
nand eco_gc143(eco_w432, eco_w437, eco_w438);
nand eco_gc144(eco_w395, eco_w425, eco_w432);
not eco_gc145(eco_w450, eco_w439);
or eco_gc146(eco_w449, eco_w450, n_551);
not eco_gc147(eco_w452, n_551);
or eco_gc148(eco_w451, eco_w439, eco_w452);
nand eco_gc149(eco_w453, eco_w449, eco_w451);
not eco_gc150(eco_w568, eco_w453);
or eco_gc151(eco_w411, eco_w568, op[0]);
nand eco_gc152(eco_w444, eco_w441, n_551);
or eco_gc153(eco_w446, eco_w441, n_551);
nand eco_gc154(eco_w424, eco_w444, eco_w446);
nand eco_gc155(eco_w412, eco_w424, op[0]);
nand eco_gc156(eco_w390, eco_w411, eco_w412);
not eco_gc157(eco_w461, eco_w285);
nand eco_gc158(eco_w462, eco_w294, eco_w465);
or eco_gc159(eco_w460, eco_w461, eco_w462);
not eco_gc160(eco_w464, eco_w462);
or eco_gc161(eco_w463, eco_w285, eco_w464);
nand eco_gc162(eco_w448, eco_w460, eco_w463);
not eco_gc163(eco_w447, eco_w448);
or eco_gc164(eco_w427, eco_w447, op[0]);
nand eco_gc165(eco_w456, eco_w467, eco_w291);
nand eco_gc166(eco_w454, eco_w455, eco_w456);
or eco_gc167(eco_w457, eco_w455, eco_w456);
nand eco_gc168(eco_w443, eco_w454, eco_w457);
nand eco_gc169(eco_w428, eco_w443, op[0]);
nand eco_gc170(eco_w400, eco_w427, eco_w428);
not eco_gc171(eco_w399, eco_w400);
or eco_gc172(eco_w383, op[1], eco_w399);
or eco_gc173(eco_w498, wc54, op[0]);
not eco_gc174(eco_w499, eco_w293);
or eco_gc175(eco_w469, eco_w498, eco_w499);
or eco_gc176(eco_w500, wc54, eco_w293);
not eco_gc177(eco_w503, eco_w293);
or eco_gc178(eco_w502, a[1], eco_w503);
nand eco_gc179(eco_w482, eco_w500, eco_w502);
nand eco_gc180(eco_w470, eco_w482, op[0]);
nand eco_gc181(eco_w442, eco_w469, eco_w470);
nand eco_gc182(eco_w384, eco_w442, op[1]);
nand eco_gc183(eco_w324, eco_w383, eco_w384);
nand eco_gc184(eco_w429, eco_w441, n_525);
nand eco_gc185(eco_w420, n_524, eco_w429);
nand eco_gc186(eco_w419, eco_w420, n_554_in);
or eco_gc187(eco_w421, eco_w420, n_554_in);
nand eco_gc188(eco_w382, eco_w419, eco_w421);
nand eco_gc189(eco_w359, eco_w382, op[0]);
nand eco_gc190(eco_w423, eco_w439, n_596);
nand eco_gc191(eco_w406, n_676, eco_w423);
not eco_gc192(eco_w408, n_554_in);
or eco_gc193(eco_w407, eco_w406, eco_w408);
nand eco_gc194(eco_w489, a[0], n_539);
or eco_gc195(eco_w473, eco_w489, eco_w286);
nand eco_gc196(eco_w311, eco_w473, eco_w474);
not eco_gc197(eco_w323, eco_w325);
or eco_gc198(eco_w322, eco_w323, eco_w324);
nand eco_gc199(eco_w506, a[1], a[0]);
or eco_gc200(eco_w485, eco_w506, eco_w286);
nand eco_gc201(eco_w472, eco_w485, eco_w293);
nand eco_gc202(eco_w430, eco_w545, n_531);
not eco_gc203(eco_w554, n_534);
or eco_gc204(eco_w551, n_521, eco_w554);
nand eco_gc205(eco_w541, eco_w551, n_535);
nand eco_gc206(eco_w542, n_569, n_571);
or eco_gc207(eco_w537, eco_w541, eco_w542);
nand eco_gc208(eco_w544, n_568, n_570);
nand eco_gc209(eco_w538, n_569, eco_w544);
nand eco_gc210(eco_w377, eco_w537, eco_w538);
nor eco_gc211(eco_w396, n_643, eco_w544);
not eco_gc212(eco_w404, eco_w406);
or eco_gc213(eco_w403, eco_w404, n_554_in);
not eco_gc214(eco_w313, eco_w310);
not eco_gc215(eco_w525, eco_w541);
nand eco_gc216(eco_w369, op[0], eco_w525);
nand eco_gc217(eco_w297, eco_w298, wc47);
not eco_gc218(eco_w565, eco_w297);
or eco_gc219(eco_w368, eco_w565, op[1]);
endmodule