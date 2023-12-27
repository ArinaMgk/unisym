// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFZ21
// AllAuthor: @dosconio
// ModuTitle: Encoder Bit 8 to Hex 3

module coder_8_3 (
	input [7:0] INP,
	output reg [2:0] RES
);

always @* begin
    case (INP)
        8'b00000001: RES = 3'b000;
        8'b00000010: RES = 3'b001;
        8'b00000100: RES = 3'b010;
        8'b00001000: RES = 3'b011;
        8'b00010000: RES = 3'b100;
        8'b00100000: RES = 3'b101;
        8'b01000000: RES = 3'b110;
        8'b10000000: RES = 3'b111;
        default: RES = 3'b000;
    endcase
end

endmodule
