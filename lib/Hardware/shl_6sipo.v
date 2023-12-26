// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFX10
// AllAuthor: @dosconio
// ModuTitle: Shifter Left 6-bit Series-In Parallel-Out

module shl_6sipo(
	input CLK,
	input RST,
	input DIN,
	input EN,
	output reg READY, // Nega Instant: RST
	output reg [5:0] DOUT // Output data signal
);

reg [5:0] DTMP;

always @(posedge CLK or negedge RST)
begin
	if (!RST)
		{READY,DTMP} <= 7'b0000001; // Reset the shift register
	else if (CLK && EN)
	begin
		if (READY)
		begin
			{READY,DTMP} <= {6'b000001,DIN};
			DOUT <= 0;
		end
		else
		begin
			{READY,DTMP} <= {DTMP,DIN};
			DOUT <= ((DTMP[5]) ? DTMP : 0);
		end
	end
end

endmodule

