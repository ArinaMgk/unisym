#!/bin/bash
# ASCII ShellScript TAB4 LF
# Attribute: 
# LastCheck: 
# AllAuthor: @dosconio
# ModuTitle: Makefile for Magice
# Copyright: ArinaMgk UniSym, Apache License Version 2.0
# Dependens: gcc-riscv64-linux-gnu-gcc (used riscv64-unknown-linux-gnu-gcc)

assemble_clover_object()
{
	input="$1"
	syntax="$2"
	stem="$3"
	shift 3
	clover-riscv "$input" "--asm=$syntax" "$@" -o "$stem.s" || exit 1
	if [ "$syntax" = gcc ]; then
		riscv64-linux-gnu-as -o "$stem.o" "$stem.s" || exit 1
	else
		aasm -march=riscv -f elf64 -o "$stem.o" "$stem.s" || exit 1
	fi
}

check()
{
	arguments="$1"
	expectation="$2"
	frontend_option="$3"
	backend_inst="${4:-im}"
	backend_abi="${5:-lp64}"
	if [ -n "$frontend_option" ]; then
		mgc --emit-ir "$arguments" "$frontend_option" -o "$uobjpath/check.ir" || exit 1
	else
		mgc --emit-ir "$arguments" -o "$uobjpath/check.ir" || exit 1
	fi
	clover-riscv "$uobjpath/check.ir" -mbits=64 -minst="$backend_inst" \
		-mabi="$backend_abi" -o "$uobjpath/check.s" || exit 1
	riscv64-linux-gnu-gcc -static -o "$uobjpath/check" "$uobjpath/check.s" || exit 1
	qemu-riscv64 -L "$RISCV/sysroot" "$uobjpath/check"
	feedback="$?"

	if [ "$feedback" = "$expectation" ]; then
		printf '\033[1;32mpassed\033[0m: %s !\n' "$arguments"
	else
		echo "$arguments failed for $feedback, Expecting $expectation"
		exit 1
	fi	
}

check_fail()
{
	arguments="$1"
	expectation="$2"
	feedback="$(mgc --emit-ir "$arguments" 2>&1)"
	result="$?"
	if [ "$result" != 0 ] && printf '%s' "$feedback" | grep -Fq "$expectation"; then
		printf '\033[1;32mpassed\033[0m: rejected %s !\n' "$arguments"
	else
		printf '%s\n' "$feedback"
		echo "$arguments was not rejected with: $expectation"
		exit 1
	fi
}

check_ir()
{
	arguments="$1"
	expectation="$2"
	feedback="$(mgc --emit-ir "$arguments")" || exit 1
	if printf '%s\n' "$feedback" | grep -Fq "$expectation"; then
		printf '\033[1;32mpassed\033[0m: IR contains %s !\n' "$expectation"
	else
		printf '%s\n' "$feedback"
		echo "IR did not contain: $expectation"
		exit 1
	fi
}

check_relocation_model()
{
	model="$1"
	feedback="$(mgc --emit-ir "--relocation-model=$model" 'int main(void){return 0;}')" || exit 1
	if [ "$model" = static ]; then
		if ! printf '%s\n' "$feedback" | grep -Fq 'target relocation'; then
			printf '\033[1;32mpassed\033[0m: default relocation model %s !\n' "$model"
			return
		fi
	else
		if printf '%s\n' "$feedback" | grep -Fq "target relocation $model"; then
			printf '\033[1;32mpassed\033[0m: relocation model %s !\n' "$model"
			return
		fi
	fi
	printf '%s\n' "$feedback"
	echo "IR did not retain relocation model: $model"
	exit 1
}

check_pic_address_model()
{
	address_fixture='../demo/test/clover/arch-riscv64/address-model.ir'
	address_provider='../demo/test/clover/arch-riscv64/address-provider.c'
	for model in static pic pie; do
		expected_data_relocations=6
		if [ "$model" = static ]; then
			expected_data_relocations=7
		fi
		{
			printf 'target relocation %s\n' "$model"
			sed -n '1,$p' "$address_fixture"
			if [ "$model" = static ]; then
				printf '%s\n' \
					'internal constant @readonly_local_ptr ptr align 8 = @private_data'
			fi
		} > "$uobjpath/address-$model.ir"
		for syntax in gcc aasm; do
			stem="$uobjpath/address-$model-$syntax"
			assemble_clover_object "$uobjpath/address-$model.ir" "$syntax" \
				"$stem" -minst=imfd -mabi=lp64d
			LC_ALL=C readelf -rW "$stem.o" > "$stem.relocs" || exit 1
			LC_ALL=C readelf -Ws "$stem.o" > "$stem.symbols" || exit 1
			case "$model" in
			static) got_symbols=''; expected_got=0 ;;
			pic) got_symbols='default_data default_function external_data weak_data weak_external_data weak_function external_function weak_external_function'; expected_got=8 ;;
			pie) got_symbols='external_data weak_data weak_external_data external_function weak_function weak_external_function'; expected_got=6 ;;
			esac
			if [ "$syntax" = gcc ]; then
				named_got_symbols="$got_symbols"
			else
				named_got_symbols='external_data weak_external_data external_function weak_external_function'
			fi
			for symbol in $named_got_symbols; do
				case " $got_symbols " in *" $symbol "*) ;; *) continue ;; esac
				grep -Eq "R_RISCV_GOT_HI20[[:space:]]+.*${symbol}" \
					"$stem.relocs" || exit 1
			done
			if [ "$model" = static ]; then
				for symbol in external_data weak_external_data external_function weak_external_function; do
					grep -Eq "R_RISCV_PCREL_HI20[[:space:]]+.*${symbol}" \
						"$stem.relocs" || exit 1
				done
			fi
			pcrel_count=$(grep -c 'R_RISCV_PCREL_HI20' \
				"$stem.relocs") || pcrel_count=0
			got_count=$(grep -c 'R_RISCV_GOT_HI20' \
				"$stem.relocs") || got_count=0
			pcrel_low=$(grep -c 'R_RISCV_PCREL_LO12_I' \
				"$stem.relocs") || pcrel_low=0
			[ "$got_count" = "$expected_got" ] && \
				[ "$pcrel_count" = "$((14 - expected_got))" ] && \
				[ "$((pcrel_count + got_count))" = "$pcrel_low" ] || exit 1
			grep -Eq 'WEAK[[:space:]]+DEFAULT[[:space:]]+.*weak_data$' \
				"$stem.symbols" || exit 1
			grep -Eq 'WEAK[[:space:]]+DEFAULT[[:space:]]+UND weak_external_data$' \
				"$stem.symbols" || exit 1
			[ "$(grep -c 'R_RISCV_64' "$stem.relocs")" = \
				"$expected_data_relocations" ] || exit 1
			grep -Eq 'R_RISCV_64[[:space:]]+.*external_data \+ 0$' \
				"$stem.relocs" || exit 1
			grep -Eq 'R_RISCV_64[[:space:]]+.*external_function \+ 0$' \
				"$stem.relocs" || exit 1
			case "$model" in
			static)
				riscv64-linux-gnu-gcc -static -o "$stem.elf" \
					"$address_provider" "$stem.o" || exit 1
				qemu-riscv64 -L "$RISCV/sysroot" "$stem.elf" || exit 1
				;;
			pic)
				ld.lld -m elf64lriscv -shared -o "$stem.so" "$stem.o" || exit 1
				LC_ALL=C readelf -dW "$stem.so" > "$stem.dynamic" || exit 1
				if grep -Eq 'TEXTREL|FLAGS.*TEXTREL' "$stem.dynamic"; then exit 1; fi
				LC_ALL=C readelf -rW "$stem.so" > "$stem.linked-relocs" || exit 1
				grep -Eq 'R_RISCV_64[[:space:]]+.*external_data \+ 0$' \
					"$stem.linked-relocs" || exit 1
				;;
			pie)
				riscv64-linux-gnu-gcc -fPIE -pie -o "$stem.elf" \
					"$address_provider" "$stem.o" || exit 1
				qemu-riscv64 -L "$RISCV/sysroot" "$stem.elf" || exit 1
				LC_ALL=C readelf -dW "$stem.elf" > "$stem.dynamic" || exit 1
				if grep -Eq 'TEXTREL|FLAGS.*TEXTREL' "$stem.dynamic"; then exit 1; fi
				;;
			esac
		done
	done
	printf '%s\n' \
		'target relocation pic' \
		'target ptrbits 64' \
		'declare global @external_data i32' \
		'constant @readonly_external_ptr ptr align 8 = @external_data' \
		> "$uobjpath/address-readonly.ir"
	if clover-riscv "$uobjpath/address-readonly.ir" \
		-o "$uobjpath/address-readonly.s" 2> "$uobjpath/address-readonly.err"; then
		exit 1
	fi
	grep -Fq 'PIC/PIE read-only symbol pointer initializer is not supported' \
		"$uobjpath/address-readonly.err" || exit 1
	printf '\033[1;32mpassed\033[0m: address relocations and Static/PIC/PIE links !\n'
}

check_pic_call_model()
{
	for model in static pic pie; do
		printf '%s\n' \
			"target relocation $model" \
			'target ptrbits 64' \
			'declare func @external_decl() -> i32' \
			'declare func @weak_decl() -> i32 binding weak' \
			'internal func @internal_def() -> i32 {' \
			'^entry:' \
			'  %0 = const i32 1' \
			'  ret i32 %0' \
			'}' \
			'func @default_def() -> i32 {' \
			'^entry:' \
			'  %1 = const i32 1' \
			'  ret i32 %1' \
			'}' \
			'func @hidden_def() -> i32 visibility hidden {' \
			'^entry:' \
			'  %2 = const i32 1' \
			'  ret i32 %2' \
			'}' \
			'func @protected_def() -> i32 visibility protected {' \
			'^entry:' \
			'  %3 = const i32 1' \
			'  ret i32 %3' \
			'}' \
			'func @weak_def() -> i32 binding weak {' \
			'^entry:' \
			'  %4 = const i32 1' \
			'  ret i32 %4' \
			'}' \
			'func @call_internal() -> i32 {' \
			'^entry:' \
			'  %5 = call i32 @internal_def()' \
			'  ret i32 %5' \
			'}' \
			'func @call_default() -> i32 {' \
			'^entry:' \
			'  %6 = call i32 @default_def()' \
			'  ret i32 %6' \
			'}' \
			'func @call_hidden() -> i32 {' \
			'^entry:' \
			'  %7 = call i32 @hidden_def()' \
			'  ret i32 %7' \
			'}' \
			'func @call_protected() -> i32 {' \
			'^entry:' \
			'  %8 = call i32 @protected_def()' \
			'  ret i32 %8' \
			'}' \
			'func @call_weak_def() -> i32 {' \
			'^entry:' \
			'  %9 = call i32 @weak_def()' \
			'  ret i32 %9' \
			'}' \
			'func @call_external_decl() -> i32 {' \
			'^entry:' \
			'  %10 = call i32 @external_decl()' \
			'  ret i32 %10' \
			'}' \
			'func @call_weak_decl() -> i32 {' \
			'^entry:' \
			'  %11 = call i32 @weak_decl()' \
			'  ret i32 %11' \
			'}' \
			'func @call_runtime_helper(f128 %12, f128 %13) -> f128 {' \
			'^entry:' \
			'  %14 = fadd f128 %12, %13' \
			'  ret f128 %14' \
			'}' \
			'func @call_indirect(ptr %15) -> i32 {' \
			'^entry:' \
			'  %16 = call i32 %15()' \
			'  ret i32 %16' \
			'}' > "$uobjpath/calls-$model.ir"
		for syntax in gcc aasm; do
			assemble_clover_object "$uobjpath/calls-$model.ir" "$syntax" \
				"$uobjpath/calls-$model-$syntax"
			LC_ALL=C readelf -rW "$uobjpath/calls-$model-$syntax.o" \
				> "$uobjpath/calls-$model-$syntax.relocs" || exit 1
			if [ "$syntax" = gcc ]; then
				grep -Fq 'jalr ra, 0(t0)' "$uobjpath/calls-$model-$syntax.s" || exit 1
			else
				grep -Fq 'jalr ra, t0' "$uobjpath/calls-$model-$syntax.s" || exit 1
			fi
			if [ "$syntax" = gcc ]; then
				for symbol in internal_def hidden_def protected_def; do
					grep -Eq "R_RISCV_CALL[[:space:]]+.* $symbol \+ 0$" \
						"$uobjpath/calls-$model-$syntax.relocs" || exit 1
				done
				case "$model" in
				static)
					expect_call='default_def weak_def external_decl weak_decl __addtf3'
					expect_plt=''
					;;
				pic)
					expect_call=''
					expect_plt='default_def weak_def external_decl weak_decl __addtf3'
					;;
				pie)
					expect_call='default_def'
					expect_plt='weak_def external_decl weak_decl __addtf3'
					;;
				esac
			else
				case "$model" in
				static)
					expect_call='external_decl weak_decl __addtf3'
					expect_plt=''
					;;
				pic)
					expect_call=''
					expect_plt='external_decl weak_decl __addtf3'
					;;
				pie)
					expect_call=''
					expect_plt='external_decl weak_decl __addtf3'
					;;
				esac
				for symbol in internal_def hidden_def protected_def; do
					grep -Eq "^[[:space:]]*CALL $symbol$" \
						"$uobjpath/calls-$model-$syntax.s" || exit 1
				done
				case "$model" in
				static)
					for symbol in default_def weak_def external_decl weak_decl; do
						grep -Eq "^[[:space:]]*CALL $symbol$" \
							"$uobjpath/calls-$model-$syntax.s" || exit 1
					done
					;;
				pic)
					for symbol in default_def weak_def external_decl weak_decl; do
						grep -Eq "^[[:space:]]*CALL_PLT $symbol$" \
							"$uobjpath/calls-$model-$syntax.s" || exit 1
					done
					;;
				pie)
					grep -Eq '^[[:space:]]*CALL default_def$' \
						"$uobjpath/calls-$model-$syntax.s" || exit 1
					for symbol in weak_def external_decl weak_decl; do
						grep -Eq "^[[:space:]]*CALL_PLT $symbol$" \
							"$uobjpath/calls-$model-$syntax.s" || exit 1
					done
					;;
				esac
			fi
			for symbol in $expect_call; do
				grep -Eq "R_RISCV_CALL[[:space:]]+.* $symbol \+ 0$" \
					"$uobjpath/calls-$model-$syntax.relocs" || exit 1
			done
			for symbol in $expect_plt; do
				grep -Eq "R_RISCV_CALL_PLT[[:space:]]+.* $symbol \+ 0$" \
					"$uobjpath/calls-$model-$syntax.relocs" || exit 1
			done
			LC_ALL=C readelf -Ws "$uobjpath/calls-$model-$syntax.o" \
				> "$uobjpath/calls-$model-$syntax.symbols" || exit 1
			grep -Eq 'GLOBAL[[:space:]]+DEFAULT[[:space:]]+UND external_decl$' \
				"$uobjpath/calls-$model-$syntax.symbols" || exit 1
			grep -Eq 'WEAK[[:space:]]+DEFAULT[[:space:]]+UND weak_decl$' \
				"$uobjpath/calls-$model-$syntax.symbols" || exit 1
		done
	done
	printf '\033[1;32mpassed\033[0m: PIC/PIE direct-call relocations and declarations !\n'
}

check_clover_forward_jumps()
{
	printf '%s\n' \
		'declare func @work() -> void' \
		'func @test(i1 %0) -> void {' \
		'^entry:' \
		'  jmp i1 %0, ^true.forward, ^false.forward' \
		'^true.forward:' \
		'  jmp ^chain.forward' \
		'^false.forward:' \
		'  jmp ^if.end' \
		'^chain.forward:' \
		'  jmp ^if.end' \
		'^if.end:' \
		'  ret void' \
		'}' \
		'func @keep() -> void {' \
		'^entry.forward:' \
		'  jmp ^exit' \
		'^self:' \
		'  jmp ^self' \
		'^busy:' \
		'  call void @work()' \
		'  jmp ^exit' \
		'^exit:' \
		'  ret void' \
		'}' > "$uobjpath/forward-jumps.ir"
	clover-opt "$uobjpath/forward-jumps.ir" --passes=forward-jumps \
		-o "$uobjpath/forward-jumps-passed.ir" || exit 1
	grep -Fq 'declare func @work() -> void' "$uobjpath/forward-jumps-passed.ir" || exit 1
	grep -Fq 'jmp i1 %0, ^if.end, ^if.end' "$uobjpath/forward-jumps-passed.ir" || exit 1
	if grep -Eq '^\^(true|false|chain)\.forward:' "$uobjpath/forward-jumps-passed.ir"; then exit 1; fi
	grep -Fq '^entry.forward:' "$uobjpath/forward-jumps-passed.ir" || exit 1
	grep -Fq '^self:' "$uobjpath/forward-jumps-passed.ir" || exit 1
	grep -Fq '^busy:' "$uobjpath/forward-jumps-passed.ir" || exit 1
	clover-opt "$uobjpath/forward-jumps-passed.ir" --passes=forward-jumps \
		-o "$uobjpath/forward-jumps-passed-twice.ir" || exit 1
	cmp "$uobjpath/forward-jumps-passed.ir" \
		"$uobjpath/forward-jumps-passed-twice.ir" || exit 1

	printf '%s\n' \
		'target ptrbits 64' \
		'func @main() -> i32 {' \
		'^entry:' \
		'  jmp ^forward' \
		'^forward:' \
		'  jmp ^result' \
		'^result:' \
		'  %0 = const i32 42' \
		'  ret i32 %0' \
		'}' > "$uobjpath/forward-jumps-runtime.ir"
	clover-opt "$uobjpath/forward-jumps-runtime.ir" --passes=forward-jumps \
		-o "$uobjpath/forward-jumps-runtime-passed.ir" || exit 1
	for variant in original passed; do
		if [ "$variant" = original ]; then
			runtime_ir="$uobjpath/forward-jumps-runtime.ir"
		else
			runtime_ir="$uobjpath/forward-jumps-runtime-passed.ir"
		fi
		clover-riscv "$runtime_ir" -o "$uobjpath/forward-jumps-$variant.s" || exit 1
		riscv64-linux-gnu-gcc -static -o "$uobjpath/forward-jumps-$variant" \
			"$uobjpath/forward-jumps-$variant.s" || exit 1
		qemu-riscv64 -L "$RISCV/sysroot" "$uobjpath/forward-jumps-$variant"
		runtime_result="$?"
		[ "$runtime_result" = 42 ] || exit 1
	done
	printf '\033[1;32mpassed\033[0m: Clover forward-jumps pass !\n'
}

check_systemverilog_frontend()
{
	help_output="$(mgc -h)" || exit 1
	printf '%s\n' "$help_output" | grep -Fq 'Supported source frontends:' || exit 1
	printf '%s\n' "$help_output" | grep -Eq 'c[[:space:]]+\.c[[:space:]]+C99 subset' || exit 1
	printf '%s\n' "$help_output" | grep -Eq 'systemverilog[[:space:]]+\.sv[[:space:]]+synthesizable subset' || exit 1
	printf '%s\n' "$help_output" | grep -Fq 'Built-in document backends:' || exit 1
	for backend in html tex md stdout; do
		printf '%s\n' "$help_output" | grep -Eq "^[[:space:]]+$backend[[:space:]]" || exit 1
	done

	mgc --emit-ir '../demo/test/hwdesc/add1.sv' \
		-o "$uobjpath/add1-hard.ir" || exit 1
	mgc --lango=sv --emit-ir '../demo/test/hwdesc/add1.sv' \
		-o "$uobjpath/add1-hard-explicit.ir" || exit 1
	cmp "$uobjpath/add1-hard.ir" "$uobjpath/add1-hard-explicit.ir" || exit 1
	grep -Fq 'module @add1 {' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq 'port @a b32 input' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq 'port @y b32 output' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq '%0 = read b32 @a' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq '%1 = const b32 1' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq '%2 = add b32 %0, %1' "$uobjpath/add1-hard.ir" || exit 1
	grep -Fq 'assign @y, %2' "$uobjpath/add1-hard.ir" || exit 1
	clover-opt "$uobjpath/add1-hard.ir" -o "$uobjpath/add1-hard-roundtrip.ir" || exit 1
	cmp "$uobjpath/add1-hard.ir" "$uobjpath/add1-hard-roundtrip.ir" || exit 1
	if clover-riscv "$uobjpath/add1-hard.ir" \
		-o "$uobjpath/add1-hard.s" > "$uobjpath/add1-hard.out" 2>&1; then exit 1; fi
	grep -Fq 'expected Soft IR module' "$uobjpath/add1-hard.out" || exit 1

	printf '%s\n' \
		'module invalid(input logic a, output logic y);' \
		'  always y = a;' \
		'endmodule' > "$uobjpath/unsupported-systemverilog.sv"
	if mgc --emit-ir "$uobjpath/unsupported-systemverilog.sv" \
		-o "$uobjpath/unsupported-systemverilog.ir" \
		> "$uobjpath/unsupported-systemverilog.out" 2>&1; then exit 1; fi
	grep -Fq 'unsupported SystemVerilog module item' \
		"$uobjpath/unsupported-systemverilog.out" || exit 1
	printf '%s\n' \
		'module duplicate(input logic a, input logic a);' \
		'endmodule' > "$uobjpath/duplicate-systemverilog-port.sv"
	if mgc --emit-ir "$uobjpath/duplicate-systemverilog-port.sv" \
		> "$uobjpath/duplicate-systemverilog-port.out" 2>&1; then exit 1; fi
	grep -Fq 'duplicate SystemVerilog port' \
		"$uobjpath/duplicate-systemverilog-port.out" || exit 1
	printf '%s\n' \
		'module unknown(input logic a, output logic y);' \
		'  assign y = missing + 1;' \
		'endmodule' > "$uobjpath/unknown-systemverilog-signal.sv"
	if mgc --emit-ir "$uobjpath/unknown-systemverilog-signal.sv" \
		> "$uobjpath/unknown-systemverilog-signal.out" 2>&1; then exit 1; fi
	grep -Fq 'SystemVerilog expression references an undefined port' \
		"$uobjpath/unknown-systemverilog-signal.out" || exit 1
	printf '%s\n' \
		'module width(input logic [15:0] a, output logic [31:0] y);' \
		'  assign y = a + 1;' \
		'endmodule' > "$uobjpath/systemverilog-width.sv"
	if mgc --emit-ir "$uobjpath/systemverilog-width.sv" \
		> "$uobjpath/systemverilog-width.out" 2>&1; then exit 1; fi
	grep -Fq 'SystemVerilog expression width does not match assignment target' \
		"$uobjpath/systemverilog-width.out" || exit 1
	printf '%s\n' \
		'module drive_input(input logic a);' \
		'  assign a = 1;' \
		'endmodule' > "$uobjpath/systemverilog-drive-input.sv"
	if mgc --emit-ir "$uobjpath/systemverilog-drive-input.sv" \
		> "$uobjpath/systemverilog-drive-input.out" 2>&1; then exit 1; fi
	grep -Fq 'continuous assignment cannot drive an input port' \
		"$uobjpath/systemverilog-drive-input.out" || exit 1
	if mgc --emit-ir --relocation-model=pic '../demo/test/hwdesc/add1.sv' \
		> "$uobjpath/systemverilog-relocation.out" 2>&1; then exit 1; fi
	grep -Fq -- '--relocation-model is only valid for Soft IR frontends' \
		"$uobjpath/systemverilog-relocation.out" || exit 1
	printf '%s\n' \
		'module @multiple {' \
		'  port @y b1 output' \
		'^entry:' \
		'  %0 = const b1 0' \
		'  assign @y, %0' \
		'  assign @y, %0' \
		'}' > "$uobjpath/multiple-hard-drivers.ir"
	if clover-opt "$uobjpath/multiple-hard-drivers.ir" \
		> "$uobjpath/multiple-hard-drivers.out" 2>&1; then exit 1; fi
	grep -Fq 'hardware port has multiple drivers' \
		"$uobjpath/multiple-hard-drivers.out" || exit 1

	if mgc --emit-ir '../demo/test/clover/c/operators.c' \
		'../demo/test/hwdesc/add1.sv' -o "$uobjpath/mixed-frontends.ir" \
		> "$uobjpath/mixed-frontends.out" 2>&1; then exit 1; fi
	grep -Fq 'input suffixes select different frontends' \
		"$uobjpath/mixed-frontends.out" || exit 1
	printf '\033[1;32mpassed\033[0m: SystemVerilog frontend and Hard IR !\n'
}

cd ../
make magice
cd magic

check_clover_forward_jumps
check_systemverilog_frontend

# C preprocessor
check '../demo/test/clover/c/preprocessor.c' 32
check_fail $'#if 1\nint value;' 'unterminated conditional directive'
check_fail $'#if 1\n#else\n#else\n#endif' 'duplicate #else'
check_fail $'#define VALUE 1\n#define VALUE 2\nint main(void){return VALUE;}' 'incompatible macro redefinition'

# Operators
check '../demo/test/clover/c/operators.c' 83

# Statement
check '../demo/test/clover/c/loop.c' 43
check '../demo/test/clover/c/switch.c' 51
check_fail 'int main(){break;}' 'break statement not within a loop or switch'
check_fail 'int main(){continue;}' 'continue statement not within a loop'

# Declarations and typedef names
check '../demo/test/clover/c/declaration.c' 57
check_relocation_model pic
check_relocation_model pie
check_relocation_model static
check_pic_address_model
check_pic_call_model
check_fail 'typedef int T; T T; int main(void){return 0;}' 'ordinary identifier conflicts with typedef name'

# Array
check '../demo/test/clover/c/array.c' 70
check_fail 'int main(){int values[];return 0;}' 'object has incomplete array type'

# Label and goto
check '../demo/test/clover/c/goto.c' 10
check_fail 'int main(){label:return 1;label:return 2;}' 'redefinition of label'
check_fail 'int main(){goto missing;return 0;}' 'use of undefined label'

# Data model and floating-point ABI
check 'int main() {return sizeof(long)+sizeof(void*);}' 16 '-Fdata-model=lp64'
check 'int main() {return sizeof(long)+sizeof(void*);}' 12 '-Fdata-model=llp64'
check '../demo/test/clover/c/floating.c' 77 '' imfd lp64d

# Integer and character literals
check "int main(){return \"\\x41\"[0]+\"\\101\"[0]+'\u0041';}" 195
check 'int main(){return "\u4E2D"[0]&255;}' 228
check_fail 'int main(){return "\x"[0];}' 'invalid string literal escape'
check_fail 'int main(){return "\uD800"[0];}' 'invalid string literal escape'

# Enumeration
check '../demo/test/clover/c/enum.c' 66
check_fail 'enum E {A}; enum E {B}; int main(){return 0;}' 'redefinition of enum tag'

# Structure and union layout
check '../demo/test/clover/c/struct-layout.c' 254
check 'struct Global{int value;} global_object={7}; int main(){return global_object.value;}' 7
check 'struct Scope{int value;}; int main(){struct Scope outer; outer.value=3; {struct Scope; struct Scope* hidden;} {struct Scope{char value;}; struct Scope inner; inner.value=2;} return sizeof(outer)+outer.value;}' 7
check 'struct Scope{int value;}; int main(){{union Scope{char value;}; union Scope inner; inner.value=2; return sizeof(inner)+inner.value;} return 0;}' 3
check 'struct Tag{int value;}; int main(){{enum Tag{Item}; return sizeof(enum Tag)+Item;} return sizeof(struct Tag);}' 4
check 'int main(){{struct Local{int value;} object; object.value=6; return object.value;}}' 6
check 'int main(){struct Node{int value; struct Node* next;}; struct Node node; node.value=9; return node.value;}' 9
check_fail 'int main(){{struct Incomplete; struct Incomplete object;} return 0;}' 'object has incomplete record type'
check '../demo/test/clover/c/initializer.c' 102
check '../demo/test/clover/c/compound-literal.c' 198
check_fail 'struct X;int main(){return ((struct X){0},0);}' 'compound literal requires a complete object type'

# Storage Area and Lifetime
check '../demo/test/clover/c/static.c' 175 '' imfd lp64d
check '../demo/test/clover/c/static-relocation.c' 61
check_ir 'static int value; static int helper(int x){return x;} int main(){return helper(value);}' 'internal global @value'
check_ir 'static int value; static int helper(int x){return x;} int main(){return helper(value);}' 'internal func @helper'
check_fail 'int first(){{extern int hidden;}return 0;} int second(){return hidden;}' 'use of undeclared identifier'
check_fail 'int main(){extern int value=1;return value;}' 'block-scope extern declaration cannot have an initializer'
check_fail 'int first(){{int hidden(int);}return 0;} int second(){return hidden(1);}' 'call to undeclared function'
check_fail 'int main(){int value;int value(int);return 0;}' 'function declaration conflicts with local object'
check_fail 'int main(){static int invalid(int);return 0;}' 'block-scope function declaration cannot be static'
check_fail 'int source(){return 1;} int main(){static int value=source();return value;}' 'block static initializer must be a constant expression'


echo pass!
