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
void fn_prenega(uni::DnodeChain* io) { dst->OutFormat("neg a0, a0\n"); }
void fn_arimul(uni::DnodeChain* io) { dst->OutFormat("mul a0, a0, a1\n"); }
void fn_aridiv(uni::DnodeChain* io) { dst->OutFormat("div a0, a0, a1\n"); }
void fn_ariadd(uni::DnodeChain* io) { dst->OutFormat("add a0, a0, a1\n"); }
void fn_arisub(uni::DnodeChain* io) { dst->OutFormat("sub a0, a0, a1\n"); }

void fn_jbelow(uni::DnodeChain* io) { dst->OutFormat("slt a0, a0, a1\n"); }//{} signed
void fn_jgreat(uni::DnodeChain* io) { dst->OutFormat("slt a0, a1, a0\n"); }//{} signed
void fn_jbeequ(uni::DnodeChain* io) { fn_jgreat(io); dst->OutFormat("\txori a0, a0, 1\n"); }//{} signed
void fn_jgrequ(uni::DnodeChain* io) { fn_jbelow(io); dst->OutFormat("\txori a0, a0, 1\n"); }//{} signed
void fn_jequal(uni::DnodeChain* io) { dst->OutFormat("xor a0, a0, a1\n\tseqz a0, a0\n"); }//{} signed
void fn_jnoteq(uni::DnodeChain* io) { dst->OutFormat("xor a0, a0, a1\n\tsnez a0, a0\n"); }//{} signed

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
//
op_lev0[]{ preposi, prenega },
op_lev1[]{ arimul, aridiv },
op_lev2[]{ ariadd, arisub },
op_mid005[] = { jbelow, jbeequ, jgreat, jgrequ },
op_mid006[] = { jequal, jnoteq }
;

#define togsym(a) a,numsof(a)

void Operators::List(uni::Chain& nc) {
	using namespace uni;
	nc.Append(new TokenOperatorGroup(togsym(op_lev0), false, 1), false);// pref+-
	nc.Append(new TokenOperatorGroup(togsym(op_lev1), true), false);// * /
	nc.Append(new TokenOperatorGroup(togsym(op_lev2), true), false);// + -
	nc.Append(new TokenOperatorGroup(togsym(op_mid005), true), false);// G(E)T L(E)T
	nc.Append(new TokenOperatorGroup(togsym(op_mid006), true), false);// (N)EQU
}
