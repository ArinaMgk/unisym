	if (val < 0) { localout("-", 1); val = -val; }
	out_integer((stduint)val, 10, false, false, 0, true, intlog2_iexpo(byteof(stduint)));
	val -= (stduint)val;
	val *= 1000000;
	val += 0.5;
	if (_IMM(val)) {
		stduint afterdot_digits = 6;
		stduint v = val;
		while (afterdot_digits > 0 && !(v % 10)) {
			v /= 10;
			afterdot_digits--;
		}
		localout(".", 1);
		out_integer(v, 10, false, false, afterdot_digits, true, intlog2_iexpo(byteof(stduint)));
	}

