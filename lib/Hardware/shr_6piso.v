// ASCII Verilog TMPAB4 CRLF
// Attribute:
// LastCheck: RFX10
// AllAuthor: @dosconio
// ModuTitle: Shifter Right 6-bit Parallel-Input Series-Output 

module shr_6piso(
	input CLK,
	input EN,
	input RST,// neglogi
	input [5:0] DIN,
	output READY,
	output reg DOUT
);
	reg [5:0] TEMP;
	reg [2:0] count;
	reg smooth;// Harunono cut stab (RFX101644)

	always @(posedge CLK or negedge RST)
	begin
		if (!RST)
		begin
			TEMP <= 6'b100000;
			count <= 3'h5;
		end
		else if (CLK && EN)
		begin
			if (READY)
			begin
				DOUT <= DIN[0];
				TEMP <= {1'b0,DIN[5:1]};
				count <= 3'h5;
			end
			else
			begin
				DOUT <= TEMP[0];
				{TEMP[4:0]} <= {TEMP[5:1]};
				count <= count - 1;
				smooth <= (count==1);
			end
		end
	end
	
	assign READY = (count == 0) && smooth;
endmodule
