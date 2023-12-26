// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFX03
// AllAuthor: @dosconio
// ModuTitle: Counter 8 bit with double-direction

module counter_8_dd(
	input CLK,
	input RST,// Asynchronous reset
	input EN, // Synchronous enable
	input LOAD,// Synchronous load
	input DIR,// Synchronous direction (0 for down, 1 for up)
	input [7:0] DATA,
	output COUT,
	output [7:0] DOUT
);
	reg [7:0] count_reg;

	always @(posedge CLK or negedge RST)
	begin
		if (!RST)
			count_reg = 0;
		else if (EN)
		begin
			if (LOAD)
				count_reg = DATA;
			else if (DIR)
				count_reg = count_reg + 1;
			else
				count_reg = count_reg - 1;
		end
	end
	assign COUT = (DIR && count_reg==8'b11111111 || !DIR && count_reg==8'b00000000) ? 1 : 0;
	assign DOUT = count_reg;
endmodule

