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

static uni::TokenOperator
// 1
sufmem{ ".","OP@SUFMEMB" },
sufmep{ "->","OP@SUFMEMP" },
sufadd{ "++","OP@SUFADD" },
sufsub{ "--","OP@SUFSUB" },
//    ---- (Compound literal)
// 
// 2
preadd{ "++","OP@PREADD" },
presub{ "--","OP@@PRESUB" },
preposi{ "+","OP@PREPOSI" },
prenega{ "-","OP@PRENEGA" },
prelognot{ "!","OP@LOGNOT" },
prebitnot{ "~","OP@BITNOT" },
prememb{ "*","OP@PREMEMB" },
preaddr{ "&","OP@PREADDR" },
presizeof{ "sizeof","OP@SIZEOF" },//{TODO} identity operator parse sub-method
prealignof{ "_Alignof","OP@ALIGNOF" },
//    ---- (Cast)
//
// 3
arimul{ "*","OP@ARIMUL" },
aridiv{ "/","OP@ARIDIV" },
arirem{ "%","OP@ARIREM" },
//
// 4
ariadd{ "+","OP@ARIADD" },
arisub{ "-","OP@ARISUB" },
//
// 5
bitshl{ "<<","OP@BITSHL" },
bitshr{ ">>","OP@BITSHR" },
//
// 6
jbelow{ "<" ,"OP@JBELOW" },
jbeequ{ "<=","OP@JBEEQU" },
jgreat{ ">" ,"OP@JGREAT" },
jgrequ{ ">=","OP@JGREQU" },
//
// 7
jequal{ "==","OP@JEQUAL" },
jnoteq{ "!=","OP@JNOTEQ" },
//
// 8
bit_and{ "&","OP@BITAND" },
//
// 9
bit_xor{ "^","OP@BITXOR" },
//
// 10
bit_or{ "|","OP@BITWOR" },
//
// 11
logand{ "&&","OP@LOGAND" },
//
// 12
logior{ "||","OP@LOGIOR" },
//
// 13
ternar{ "\2?\2:\2","OP@TERNAR" },//{TODO}
//
// 14
assign{ "=","OP@ASSIGN" },
agnsum{ "+=","OP@AGNSUM" },
agndif{ "-=","OP@AGNDIF" },
agnpro{ "*=","OP@AGNPRO" },
agnquo{ "/=","OP@AGNQUO" },
agnrem{ "%=","OP@AGNREM" },
agnshl{ "<<=","OP@AGNSHL" },
agnshr{ ">>=","OP@AGNSHR" },
agnand{ "&=","OP@AGNAND" },
agnxor{ "^=","OP@AGNXOR" },
agnor{ "|=","OP@ASGNOR" },
//
// 15
opcomma{ ",","OP@COMMA" },
// ----
opg01_0[] = { sufadd, sufsub },// 1 suf
opapd001[] = { sufmem, sufmep },// 1 mid
opprefix[] = { preadd, presub, preposi, prenega, prelognot, prebitnot, prememb, preaddr, presizeof, prealignof },// 2
opmid003[] = { arimul, aridiv, arirem },// 3
opmid004[]{ ariadd, arisub },// 4
opmid005[] = { bitshl, bitshr },// 5
opmid006[] = { jbelow, jbeequ, jgreat, jgrequ },
opmid007[] = { jequal, jnoteq },
opmid008[] = { bit_and },
opmid009[] = { bit_xor },
opmid010[] = { bit_or },
opmid011[] = { logand },
opmid012[] = { logior },
//{} opmid013[] = {ternar},
opmid014[] = { assign, agnsum, agndif, agnpro, agnquo, agnrem, agnshl, agnshr, agnand, agnxor, agnor },
opmid015[] = { opcomma }

;

#define togsym(a) a,numsof(a)

#define TO_RIGHT true
#define TO_LEFT  false

void Operators::List(uni::Chain& nc) {
	using namespace uni;
	// 1
	nc.Append(new TokenOperatorGroup(togsym(opg01_0), TO_RIGHT, 1));// suff+-
	nc.Append(new TokenOperatorGroup(togsym(opapd001), TO_RIGHT));// . ->
	// 2
	nc.Append(new TokenOperatorGroup(togsym(opprefix), TO_LEFT, 1));// pref+-
	// 3 - 12
	nc.Append(new TokenOperatorGroup(togsym(opmid003), TO_RIGHT));// * / %
	nc.Append(new TokenOperatorGroup(togsym(opmid004), TO_RIGHT));// + -
	nc.Append(new TokenOperatorGroup(togsym(opmid005), TO_RIGHT));// << >>
	nc.Append(new TokenOperatorGroup(togsym(opmid006), TO_RIGHT));// G(E)T L(E)T
	nc.Append(new TokenOperatorGroup(togsym(opmid007), TO_RIGHT));// (N)EQU
	nc.Append(new TokenOperatorGroup(togsym(opmid008), TO_RIGHT));// &
	nc.Append(new TokenOperatorGroup(togsym(opmid009), TO_RIGHT));// ^
	nc.Append(new TokenOperatorGroup(togsym(opmid010), TO_RIGHT));// |
	nc.Append(new TokenOperatorGroup(togsym(opmid011), TO_RIGHT));// &&
	nc.Append(new TokenOperatorGroup(togsym(opmid012), TO_RIGHT));// ||
	//{TODO} 13
	nc.Append(new TokenOperatorGroup(togsym(opmid014), TO_LEFT));
	nc.Append(new TokenOperatorGroup(togsym(opmid015), TO_LEFT));
}
