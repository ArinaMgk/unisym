// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFZ21
// AllAuthor: @dosconio
// ModuTitle: Decoder Hex 4 to Bit 16

module coder_4_16 (
	input [3:0] INP,
	output reg [15:0] RES
);

always @* begin
    case (INP)
        4'b0000: RES = 16'b0000000000000001;
        4'b0001: RES = 16'b0000000000000010;
        4'b0010: RES = 16'b0000000000000100;
        4'b0011: RES = 16'b0000000000001000;
        4'b0100: RES = 16'b0000000000010000;
        4'b0101: RES = 16'b0000000000100000;
        4'b0110: RES = 16'b0000000001000000;
        4'b0111: RES = 16'b0000000010000000;
        4'b1000: RES = 16'b0000000100000000;
        4'b1001: RES = 16'b0000001000000000;
        4'b1010: RES = 16'b0000010000000000;
        4'b1011: RES = 16'b0000100000000000;
        4'b1100: RES = 16'b0001000000000000;
        4'b1101: RES = 16'b0010000000000000;
        4'b1110: RES = 16'b0100000000000000;
        4'b1111: RES = 16'b1000000000000000;
        default: RES = 16'b0000000000000000;
    endcase
end

endmodule
