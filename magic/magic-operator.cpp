// ASCII C++ TAB4 LF
// Docutitle: Magice Operator
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "./magice.hpp"


#include "../inc/cpp/parse.hpp"

extern uni::OstreamTrait* dst;

void fn_preposi(uni::DnodeChain* io) {}
void fn_prenega(uni::DnodeChain* io) {
	dst->OutFormat("\tneg a0, a0\n");
}
void fn_arimul(uni::DnodeChain* io) { dst->OutFormat("\tmul a0, a0, a1\n"); }
void fn_aridiv(uni::DnodeChain* io) { dst->OutFormat("\tdiv a0, a0, a1\n"); }
void fn_ariadd(uni::DnodeChain* io) {
	dst->OutFormat("\tadd a0, a0, a1\n");
}
void fn_arisub(uni::DnodeChain* io) { dst->OutFormat("\tsub a0, a0, a1\n"); }

void fn_jbelow(uni::DnodeChain* io) { dst->OutFormat("\tslt a0, a0, a1\n"); }//{} signed
void fn_jgreat(uni::DnodeChain* io) { dst->OutFormat("\tslt a0, a1, a0\n"); }//{} signed
void fn_jbeequ(uni::DnodeChain* io) { fn_jgreat(io); dst->OutFormat("\txori a0, a0, 1\n"); }//{} signed
void fn_jgrequ(uni::DnodeChain* io) { fn_jbelow(io); dst->OutFormat("\txori a0, a0, 1\n"); }//{} signed
void fn_jequal(uni::DnodeChain* io) { dst->OutFormat("\txor a0, a0, a1\n\tseqz a0, a0\n"); }//{} signed
void fn_jnoteq(uni::DnodeChain* io) { dst->OutFormat("\txor a0, a0, a1\n\tsnez a0, a0\n"); }//{} signed

void fn_assign(uni::DnodeChain* io) {
	// Arina Method
	using namespace uni;
	// a0 left  should be iden
	// a1 right may    be iden / value
	Letvar(sym, Nnode*, io);
	Nnode* left = sym->subf;
	Nnode* right = left->next;
	rostr reg = "a2";// t0
	if (left->type != tok_identy) { plogerro("Bad use of ="); return; }
	if (right->type == tok_identy) { // a = b, a
		// [a0]=[a1]
		// replace [a1] offset with its value
		dst->OutFormat("\tld %s, 0(a1)\n", reg);// t0 = offset
		dst->OutFormat("\tld %s, 0(%s)\n", reg, reg);// t0 = value
		dst->OutFormat("\tsd %s, 0(a1)\n", reg);// t0 = value
		// now same with tok_number
		dst->OutFormat("\tsd a1, 0(a0)\n", reg);
		dst->OutFormat("\tmv a0, a1\n");
	}
	else if (right->type == tok_number || right->type == tok_func) { // a = 1, a
		// [a0]=a1
		dst->OutFormat("\tsd a1, 0(a0)\n", reg);
		dst->OutFormat("\tmv a0, a1\n");
	}
}

static uni::TokenOperator
preposi{ "+","OP@PREPOSI",fn_preposi },
prenega{ "-","OP@PRENEGA",fn_prenega },
arimul{ "*","OP@ARIMUL",fn_arimul },
aridiv{ "/","OP@ARIDIV",fn_aridiv },
ariadd{ "+","OP@ARIADD",fn_ariadd },
arisub{ "-","OP@ARISUB",fn_arisub },
jbelow{ "<" ,"OP@JBELOW",fn_jbelow },
jbeequ{ "<=","OP@JBEEQU",fn_jbeequ },
jgreat{ ">" ,"OP@JGREAT",fn_jgreat },
jgrequ{ ">=","OP@JGREQU",fn_jgrequ },
jequal{ "==","OP@JEQUAL",fn_jequal },
jnoteq{ "!=","OP@JNOTEQ",fn_jnoteq },

assign{ "=","OP@ASSIGN",fn_assign },
//
op_lev0[]{ preposi, prenega },
op_lev1[]{ arimul, aridiv },
op_lev2[]{ ariadd, arisub },
op_mid005[] = { jbelow, jbeequ, jgreat, jgrequ },
op_mid006[] = { jequal, jnoteq },
op_mid013[] = { assign }
;

#define togsym(a) a,numsof(a)

#define TO_RIGHT true
#define TO_LEFT  false

void Operators::List(uni::Chain& nc) {
	using namespace uni;
	nc.Append(new TokenOperatorGroup(togsym(op_lev0), TO_LEFT, 1));// pref+-
	nc.Append(new TokenOperatorGroup(togsym(op_lev1), TO_RIGHT));// * /
	nc.Append(new TokenOperatorGroup(togsym(op_lev2), TO_RIGHT));// + -
	nc.Append(new TokenOperatorGroup(togsym(op_mid005), TO_RIGHT));// G(E)T L(E)T
	nc.Append(new TokenOperatorGroup(togsym(op_mid006), TO_RIGHT));// (N)EQU
	nc.Append(new TokenOperatorGroup(togsym(op_mid013), TO_LEFT));
}
