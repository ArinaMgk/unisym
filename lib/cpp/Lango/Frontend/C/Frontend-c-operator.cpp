// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Operators
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-internal.hpp"

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
presub{ "--","OP@PRESUB" },
preposi{ "+","OP@PREPOSI" },
prenega{ "-","OP@PRENEGA" },
prelognot{ "!","OP@LOGNOT" },
prebitnot{ "~","OP@BITNOT" },
prememb{ "*","OP@PREMEMB" },
preaddr{ "&","OP@PREADDR" },
presizeof{ "sizeof","OP@SIZEOF" },//{TODO} identity operator parse sub-method
prealignof{ "_Alignof","OP@ALIGNOF" },
precast{ "@cast","OP@CAST" },
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
opprefix[] = { preadd, presub, preposi, prenega, prelognot, prebitnot, prememb, preaddr, presizeof, prealignof, precast },// 2
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
opmid014[] = { assign, agnsum, agndif, agnpro, agnquo, agnrem, agnshl, agnshr, agnand, agnxor, agnor },
opmid015[] = { opcomma }

;

#define togsym(a) a,numsof(a)

#define TO_RIGHT true
#define TO_LEFT  false

void uni::lango::frontend_c_detail::ListPostfixOperators(uni::Chain& nc) {
	using namespace uni;
	// 1
	nc.Append(new TokenOperatorGroup(togsym(opg01_0), TO_RIGHT, 1));// suff+-
	nc.Append(new TokenOperatorGroup(togsym(opapd001), TO_RIGHT));// . ->
}

void uni::lango::frontend_c_detail::ListPrefixOperators(uni::Chain& nc) {
	using namespace uni;
	// 2
	nc.Append(new TokenOperatorGroup(togsym(opprefix), TO_LEFT, 1));// pref+-
}

void uni::lango::frontend_c_detail::ListBinaryOperators(uni::Chain& nc) {
	using namespace uni;
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
}

void uni::lango::frontend_c_detail::ListTrailingOperators(uni::Chain& nc) {
	using namespace uni;
	nc.Append(new TokenOperatorGroup(togsym(opmid014), TO_LEFT));
	nc.Append(new TokenOperatorGroup(togsym(opmid015), TO_LEFT));
}

namespace {

	bool IsSymbol(const uni::Nnode* node, const char* symbol) {
		return node && node->type == tok_symbol && node->addr &&
			!StrCompare(node->addr, symbol);
	}

	bool ClassifyCastList(uni::Nchain& syntax_tree, uni::Nnode* node,
		const uni::lango::FrontendCDiagnostics& diagnostics,
		const uni::lango::FrontendCTargetInfo& target,
		uni::lango::FrontendCTypeInfo& type_info) {
		for (; node; node = node->next) {
			if (node->subf && !ClassifyCastList(syntax_tree, node->subf,
				diagnostics, target, type_info)) return false;
			if (node->type != tok_func || node->addr || !node->subf) continue;
			uni::lango::FrontendCType cast_type;
			if (!uni::lango::frontend_c_detail::ParseTypeName(node->subf,
				cast_type, target, type_info)) continue;
			if (cast_type.IsArray() || cast_type.IsRecord()) {
				diagnostics.Report(uni::lango::DiagnosticStage::Semantic,
					"cast target must be a scalar type or void", node);
				return false;
			}
			if (!node->next) {
				diagnostics.Report(uni::lango::DiagnosticStage::Parser,
					"cast requires an operand", node);
				return false;
			}
			while (node->subf) syntax_tree.Remove(node->subf);
			type_info.Add(node, cast_type);
			node->type = tok_symbol;
			srs(node->addr, StrHeap("@cast"));
		}
		return true;
	}

	uni::Nnode* WrapRange(uni::Nchain& syntax_tree,
		uni::Nnode* first, uni::Nnode* last) {
		if (first == last) return first;
		uni::Nnode* wrapper = syntax_tree.Append(nullptr, true, first);
		wrapper->type = tok_func;
		*wrapper->GetTnodeField() = *first->GetTnodeField();
		syntax_tree.Adopt(wrapper, first, last);
		return wrapper;
	}

	bool ParseConditionalList(uni::Nchain& syntax_tree, uni::Nnode*& head,
		const uni::lango::FrontendCDiagnostics& diagnostics) {
		for (uni::Nnode* node = head; node; node = node->next)
			if (node->subf && !ParseConditionalList(
				syntax_tree, node->subf, diagnostics)) return false;

		while (true) {
			uni::Nnode* question = nullptr;
			for (uni::Nnode* node = head; node; node = node->next)
				if (IsSymbol(node, "?")) question = node;
			if (!question) return true;

			uni::Nnode* condition = question->getLeft();
			uni::Nnode* colon = question->next;
			while (colon && !IsSymbol(colon, ":")) colon = colon->next;
			if (!condition || !colon || question->next == colon || !colon->next) {
				diagnostics.Report(uni::lango::DiagnosticStage::Parser,
					"invalid conditional operator", question);
				return false;
			}

			WrapRange(syntax_tree, question->next, colon->getLeft());
			uni::Nnode* false_last = colon->next;
			while (false_last->next && !IsSymbol(false_last->next, ":") &&
				!IsSymbol(false_last->next, ",")) false_last = false_last->next;
			uni::Nnode* false_value = WrapRange(syntax_tree, colon->next, false_last);

			uni::Nnode* parent = syntax_tree.Append(
				StrHeap("OP@TERNAR"), true, condition);
			parent->type = tok_func;
			*parent->GetTnodeField() = *question->GetTnodeField();
			if (head == condition) head = parent;
			syntax_tree.Adopt(parent, condition, false_value);
			syntax_tree.Remove(question);
			syntax_tree.Remove(colon);
		}
	}

}

bool uni::lango::frontend_c_detail::ParseCastOperators(
	uni::Nchain& syntax_tree, const FrontendCDiagnostics& diagnostics,
	const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info) {
	return ClassifyCastList(syntax_tree, syntax_tree.Root(),
		diagnostics, target, type_info);
}

bool uni::lango::frontend_c_detail::ParseConditionalOperators(
	uni::Nchain& syntax_tree, const FrontendCDiagnostics& diagnostics) {
	uni::Nnode*& root = syntax_tree.RootRef();
	return !root || ParseConditionalList(syntax_tree, root, diagnostics);
}
