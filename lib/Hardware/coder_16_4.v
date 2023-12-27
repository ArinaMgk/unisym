// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFZ21
// AllAuthor: @dosconio
// ModuTitle: Encoder Bit 16 to Hex 4 with priority

module coder_16_4 (
	input [15:0] INP, 
	output reg [3:0] RES
);

always @* begin
	casex (INP)
		16'bxxxxxxxxxxxxxxx0: RES <= 4'b0000;
		16'bxxxxxxxxxxxxxx01: RES <= 4'b0001;
		16'bxxxxxxxxxxxxx011: RES <= 4'b0010;
		16'bxxxxxxxxxxxx0111: RES <= 4'b0011;
		16'bxxxxxxxxxxx01111: RES <= 4'b0100;
		16'bxxxxxxxxxx011111: RES <= 4'b0101;
		16'bxxxxxxxxx0111111: RES <= 4'b0110;
		16'bxxxxxxxx01111111: RES <= 4'b0111;
		16'bxxxxxxx011111111: RES <= 4'b1000;
		16'bxxxxxx0111111111: RES <= 4'b1001;
		16'bxxxxx01111111111: RES <= 4'b1010;
		16'bxxxx011111111111: RES <= 4'b1011;
		16'bxxx0111111111111: RES <= 4'b1100;
		16'bxx01111111111111: RES <= 4'b1101;
		16'bx011111111111111: RES <= 4'b1110;
		16'b0111111111111111: RES <= 4'b1111;
		default:              RES <= 4'b1111;
	endcase
end

endmodule

// Optional:
//always @* begin
//	if (!INP[0])       RES = 4'b0000;
//	else if (!INP[1])  RES = 4'b0001;
//	else if (!INP[2])  RES = 4'b0010;
//	else if (!INP[3])  RES = 4'b0011;
//	else if (!INP[4])  RES = 4'b0100;
//	else if (!INP[5])  RES = 4'b0101;
//	else if (!INP[6])  RES = 4'b0110;
//	else if (!INP[7])  RES = 4'b0111;
//	else if (!INP[8])  RES = 4'b1000;
//	else if (!INP[9])  RES = 4'b1001;
//	else if (!INP[10]) RES = 4'b1010;
//	else if (!INP[11]) RES = 4'b1011;
//	else if (!INP[12]) RES = 4'b1100;
//	else if (!INP[13]) RES = 4'b1101;
//	else if (!INP[14]) RES = 4'b1110;
//	else if (!INP[15]) RES = 4'b1111;
//end

