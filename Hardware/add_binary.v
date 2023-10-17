// ASCII Verilog Haruno(RFC14)

// [ADC] C: Carry Input; CF: Carry Flag
module add_binary(input A, B, C,
	output CF, RES);
	assign CF  = (A&B) | (A&C) | (B&C);
	assign RES = A ^ B ^ C;
	//{USE ADDER} assign {CF, RES} = A + B + C;
endmodule
