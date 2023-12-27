// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFZ21
// AllAuthor: @dosconio
// ModuTitle: 4bit-input 8bit-output unsigned multiplier

module mul_4uns8(
	output reg [3:0] RES,
	input [3:0] INA,
	input [3:0] INB
);
	reg[7:0] TMPA;
	reg[3:0] TMPB;
	always @(INA or INB)
	begin
		RES = 0;
		TMPA = INA;
		TMPB = INB;
		repeat(4)
		begin
			if (TMPB[0]) RES = RES + TMPA;
			TMPA = TMPA << 1;
			TMPB = TMPB >> 1;
		end
	end
endmodule