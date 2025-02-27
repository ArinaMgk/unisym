	if (val < 0) { localout("-", 1); val = -val; }
	out_integer((stduint)val, 10, false, false, 0, true, intlog2_iexpo(byteof(stduint)));
	val -= (stduint)val;
	val *= 1000000;
	val += 0.5;
	if (_IMM(val)) {
		localout(".", 1);
		out_integer((stduint)val, 10, false, false, 6, true, intlog2_iexpo(byteof(stduint)));
	}

