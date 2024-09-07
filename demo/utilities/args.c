// ASCII C99 TAB4 CRLF
// Docutitle: (Module) Basic Hosted-Env Template
// Codifiers: @dosconio: RFZ10 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
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

//: included by epic
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[], char* envp[]) {
	int i;
	printf("argc = %d;\n", argc);
	for (i = 0; i < argc; i++) {
		printf("argv[%d] = @\"%s\";\n", i, argv[i]);
	}
	if (!strcmp(argv[0], argv[1])) for (i = 0; envp[i] != NULL; i++) {
		printf("envp[%d] = @\"\"\"%s\"\"\";\n", i, envp[i]);
	}
	return 0;
}
