// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFX03
// AllAuthor: @dosconio
// ModuTitle: Counter 20-system

module counter_20sys(
	input CLK,// Synchronous clock
	input RST,// Synchronous reset
	input EN,// Synchronous enable
	output COUT,
	output [4:0] CNT
);
	reg [4:0] TMP;

	always @(posedge CLK)
	begin
		if(!RST)
			TMP = 0;
		else if (EN)
		begin
			if (TMP >= 19)
				TMP = 0;
			else
				TMP = TMP + 1;
		end
	end

	// `assign` can be replaced by `wire`
	assign COUT = (TMP == 19) ? 1'b1 : 1'b0;
	assign CNT = TMP;
endmodule


