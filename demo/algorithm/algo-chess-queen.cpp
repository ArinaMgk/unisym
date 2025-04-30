#include <cpp/unisym>
#include <c/consio.h>
#include <cpp/nodes>
#include <stdio.h>

using namespace uni;

bool NQueenMatch(unsigned* state, stduint index) {
	stduint n = index - 1;
	while (n > 0)
	{
		// straight
		if (state[index] == state[n])
			return false;// conflict
		// lean
		if (ABS(state[index], state[n]) == ABS(index, n))
			return false;// conflict
		n--;
	}
	return true;
}

bool NQueen(Chain& chain, stduint size) {
	if (size <= 3) return false;
	const stduint&& stateSize = (1 + size) * byteof(unsigned);
	unsigned* state = (unsigned*)zalc(stateSize);
	// Azusa: the number of state is always increasing (consider invalid digits zero) ..20250426
	int i = 1, j = 1;
	while (i > 0) {
		if (j > size) {
			i--;
			if (i > 0) j = state[i] + 1;
			continue;
		}
		state[i] = j;
		bool good = NQueenMatch(state, i);
#ifdef _A
		Console.OutFormat("%u%u%s", i, j, good ? "T, " : "F, ");
#endif
		if (good) {
			i++;
			j = 1;
			if (i > size) {
				// find a solution
				unsigned* newnod = (unsigned*)zalc(stateSize);
				MemCopyN(newnod, state, stateSize);
				chain.Append((pureptr_t)newnod);
				i--;
				j = state[i] + 1;
			}
		}
		else {
			j++;
		}
	}
	mfree(state);
	return true;
}

int main() {
	unsigned size;
	Console.OutFormat("Eight queens problem, at 1848\n");
	while (true) {
		Chain chain;
		Console.OutFormat("Please Input the size of the chess (0 for exit):");
		scanf_s("%u", &size);
		if (!size) break;
		NQueen(chain, size);
		for (auto var = chain.Root(); var; var = var->next) {
			Letvar(state, unsigned*, var->offs);
			Console.OutFormat("\n[");
			for1(i, size) Console.OutFormat("%u, ", state[i]);
			Console.OutFormat("]");
		}
		Console.OutFormat("\n");
	}// exit and free arena
	return malc_count;
}
