// ASCII Verilog Haruno(RFC20)

// [ADD] RES: Summary; CF: Carry Flag
module add_half_binary(
	input A, B,
	output CF, RES);
	assign CF =  A & B ;
	assign RES = A ^ B ;
	//{USE ADDER} assign {CF, RES} = A + B;
endmodule
