// ASCII Verilog Haruno(RFC20) [Verilog]

//
module mux_4sel1(// Multiplexer
	input A, B, C, D, SEL0, SEL1,
	output reg RES);
	always @(*)
	begin
		case({SEL1,SEL0})
			2'h0: RES=A;
			2'h1: RES=B;
			2'h2: RES=C;
			2'h3: RES=D;
			default: RES=A;
		endcase
	end
endmodule
