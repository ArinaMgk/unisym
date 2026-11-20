// ASCII C++ TAB4 LF
// ModuTitle: Common filesystem helpers
// Copyright: UNISYM, under Apache License 2.0.
#include "../../../inc/cpp/System/Filesys.hpp"

namespace uni {
	bool BlockChangeSet::Read(stduint block, void* destination) {
		if (sealed || failed || !destination || !block_size || device.Block_Size != block_size || block >= device.getUnits()) { failed = true; return false; }
		for (stduint i = changes.Count(); i; --i) {
			if (changes[i - 1]->block != block) continue;
			MemCopyN(destination, changes[i - 1]->after.get(), block_size);
			return true;
		}
		if (device.Read(block, destination)) return true;
		failed = true;
		return false;
	}

	bool BlockChangeSet::Edit(stduint block, byte*& image) {
		image = nullptr;
		if (sealed || failed || !block_size || device.Block_Size != block_size) { failed = true; return false; }
		for (stduint i = changes.Count(); i; --i) {
			auto& change = changes[i - 1];
			if (change->phase != phase) break;
			if (change->block == block) { image = change->after.get(); return true; }
		}
		auto change = make_shared<Change>();
		if (!change) { failed = true; return false; }
		change->before = make_unique<byte[]>(block_size);
		change->after = make_unique<byte[]>(block_size);
		if (!change->before || !change->after || !Read(block, change->before.get())) { failed = true; return false; }
		MemCopyN(change->after.get(), change->before.get(), block_size);
		change->block = block;
		change->phase = phase;
		if (!changes.Append(change)) { failed = true; return false; }
		image = change->after.get();
		return true;
	}

	bool BlockChangeSet::NextPhase() {
		if (sealed || failed || phase == ~stduint(0)) { failed = true; return false; }
		++phase;
		return true;
	}

	bool BlockChangeSet::Write(stduint block, const void* source) {
		byte* image = nullptr;
		if (!source || !Edit(block, image)) { failed = true; return false; }
		MemCopyN(image, source, block_size);
		return NextPhase();
	}

	BlockCommitResult BlockChangeSet::Commit() {
		if (sealed) return result;
		sealed = true;
		if (failed || !block_size || device.Block_Size != block_size) return BlockCommitResult::FailedRestored;
		for (stduint i = 0; i < changes.Count(); ++i) {
			auto& change = changes[i];
			if (device.Write(change->block, change->after.get())) continue;
			bool restored = true;
			// A failed write may have modified its block, so restore it before all earlier attempts.
			for (stduint n = i + 1; n; --n) {
				auto& previous = changes[n - 1];
				if (!device.Write(previous->block, previous->before.get())) restored = false;
			}
			return result = restored ? BlockCommitResult::FailedRestored : BlockCommitResult::FailedUncertain;
		}
		return result = BlockCommitResult::Success;
	}

	FilesysDirectoryState FilesysTrait::InspectDirectory(void* handler) {
		FilesysDirectoryState result = FilesysDirectoryState::Unsupported;
		if (!proper(handler, stduint(FilesysCmd::FS_CMD_INSPECT_DIRECTORY), &result)) return FilesysDirectoryState::Unsupported;
		return result;
	}

	bool EmitDirectoryEntry(FilesysEnumState* state, _tocall_ft callback, bool directory, rostr name) {
		if (!callback) return false;
		if (!state) {
			callback((void*)stduint(directory), (void*)name);
			return true;
		}
		if (state->finished || state->full()) return false;
		if (state->should_emit()) {
			callback((void*)stduint(directory), (void*)name);
			state->emit_one();
			return !state->full();
		}
		state->skip_one();
		return true;
	}

	// Consume separators around a component without resolving its filesystem meaning.
	bool NextPathComponent(rostr& cursor, FilesysPathComponent& component) {
		component = {};
		if (!cursor) return false;
		while (*cursor == '/') ++cursor;
		if (!*cursor) return false;
		component.data = cursor;
		while (*cursor && *cursor != '/') ++cursor;
		component.length = cursor - component.data;
		while (*cursor == '/') ++cursor;
		return true;
	}

	// Keep trailing-separator information so each operation can apply its existing directory rules.
	bool SplitParentAndName(rostr path, FilesysPathParts& result) {
		result = {};
		if (!path || !*path) return false;
		result.absolute = *path == '/';
		const char* end = path + StrLength(path);
		result.trailing_separator = end[-1] == '/';
		while (end > path && end[-1] == '/') --end;
		if (end == path) return false;
		const char* name = end;
		while (name > path && name[-1] != '/') --name;
		const char* parent_end = name;
		while (parent_end > path && parent_end[-1] == '/') --parent_end;
		if (result.absolute && parent_end == path) ++parent_end;
		result.parent.data = path;
		result.parent.length = parent_end - path;
		result.name.data = name;
		result.name.length = end - name;
		return true;
	}
}
