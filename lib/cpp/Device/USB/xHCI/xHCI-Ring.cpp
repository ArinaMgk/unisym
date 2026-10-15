#if defined(_MCCA) && _MCCA == 0x8664
#include "../../../../../inc/c/msgface.h"
#include "../../../../../inc/cpp/Device/USB/xHCI/xHCI-registers.hpp"
#include "../../../../../inc/cpp/Device/USB/xHCI/xHCI.hpp"
#include "../../../../../inc/cpp/ISO_IEC_STD/algorithm"



namespace uni::device::SpaceUSB3 {
	Ring::~Ring() {
		if (buf_ != nullptr) {
			uni_hostenv_allocator->deallocate(buf_);
			buf_ = nullptr;
		}
	}

	Error Ring::Initialize(size_t buf_size) {
		if (buf_size < 16 || buf_size > 4096) {
			return MAKE_ERROR(Error::InvalidControllerCapability);
		}
		if (buf_ != nullptr) {
			uni_hostenv_allocator->deallocate(buf_);
			// plogwarn(">> %[x]", buf_);
		}

		cycle_bit_ = true;
		write_index_ = 0;
		buf_size_ = buf_size;

		buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
		if (buf_ == nullptr) {
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		MemSet(buf_, 0, buf_size_ * sizeof(TRB));
		return MAKE_ERROR(Error::Success);
	}

	void Ring::CopyToLast(const uni::Array<uint32_t, 4>& data) {
		for (int i = 0; i < 3; ++i) {
		  // data[0..2] must be written prior to data[3].
			buf_[write_index_].data[i] = data[i];
		}
		#if defined(_MCCA) && (_MCCA & 0xFF00) == 0x8600
		// _ASM("sfence" ::: "memory");
		#endif
		buf_[write_index_].data[3]
			= (data[3] & 0xfffffffeu) | static_cast<uint32_t>(cycle_bit_);
	}

	TRB* Ring::Push(const uni::Array<uint32_t, 4>& data) {
		auto trb_ptr = &buf_[write_index_];
		CopyToLast(data);

		++write_index_;
		if (write_index_ == buf_size_ - 1) {
			LinkTRB link{ buf_ };
			link.bits.toggle_cycle = true;
			link.bits.chain_bit = (data[3] & (1u << 4)) != 0;
			CopyToLast(link.data);

			write_index_ = 0;
			cycle_bit_ = !cycle_bit_;
		}

		return trb_ptr;
	}

	TRB* Ring::PushDeferred(const uni::Array<uint32_t, 4>& data) {
		auto* trb_ptr = &buf_[write_index_];
		const bool producer_cycle_state = cycle_bit_;
		cycle_bit_ = !cycle_bit_;
		CopyToLast(data);
		cycle_bit_ = producer_cycle_state;

		++write_index_;
		if (write_index_ == buf_size_ - 1) {
			LinkTRB link{ buf_ };
			link.bits.toggle_cycle = true;
			link.bits.chain_bit = (data[3] & (1u << 4)) != 0;
			CopyToLast(link.data);
			write_index_ = 0;
			cycle_bit_ = !cycle_bit_;
		}
		return trb_ptr;
	}

	void Ring::Commit(TRB* trb, bool cycle_state) {
		if (trb == nullptr) return;
		#if defined(_MCCA) && (_MCCA & 0xFF00) == 0x8600
		_ASM("sfence" ::: "memory");
		#endif
		trb->data[3] = (trb->data[3] & 0xfffffffeu) |
			static_cast<uint32_t>(cycle_state);
	}

	TRB* Ring::NextTransferTRB(const TRB* trb) const {
		if (buf_ == nullptr || buf_size_ < 2 || trb == nullptr) return nullptr;
		const uintptr_t address = reinterpret_cast<uintptr_t>(trb);
		const uintptr_t begin = reinterpret_cast<uintptr_t>(buf_);
		const uintptr_t end = reinterpret_cast<uintptr_t>(buf_ + buf_size_ - 1);
		if (address < begin || address >= end ||
			(address - begin) % sizeof(TRB) != 0) {
			return nullptr;
		}
		auto* next = reinterpret_cast<TRB*>(address + sizeof(TRB));
		return next == buf_ + buf_size_ - 1 ? buf_ : next;
	}

	EventRing::~EventRing() {
		Release();
	}

	void EventRing::Release() {
		if (erst_ != nullptr) {
			uni_hostenv_allocator->deallocate(erst_);
			erst_ = nullptr;
		}
		for (auto* segment : segments_) {
			if (segment != nullptr) uni_hostenv_allocator->deallocate(segment);
		}
		segments_.Clear();
		segment_size_ = 0;
		total_size_ = 0;
		dequeue_segment_index_ = 0;
		interrupter_ = nullptr;
	}

	Error EventRing::Initialize(size_t total_trbs, uint8 segment_count,
		InterrupterRegisterSet* interrupter) {
		if (total_trbs < 16 || total_trbs > 32768 || segment_count == 0 ||
			segment_count > 8 || total_trbs < size_t(segment_count) * 16u ||
			interrupter == nullptr) {
			return MAKE_ERROR(Error::InvalidControllerCapability);
		}
		Release();

		cycle_bit_ = true;
		dequeue_segment_index_ = 0;
		segment_size_ = (total_trbs + segment_count - 1u) / segment_count;
		if (segment_size_ > 4096) {
			Release();
			return MAKE_ERROR(Error::InvalidControllerCapability);
		}
		total_size_ = segment_size_ * segment_count;
		interrupter_ = interrupter;

		erst_ = AllocArray<EventRingSegmentTableEntry>(segment_count, 64, 64 * 1024);
		if (erst_ == nullptr) {
			Release();
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		MemSet(erst_, 0, segment_count * sizeof(EventRingSegmentTableEntry));
		for (uint8 index = 0; index < segment_count; ++index) {
			auto* segment = AllocArray<TRB>(segment_size_, 64, 64 * 1024);
			if (segment == nullptr || !segments_.Append(segment)) {
				if (segment != nullptr) uni_hostenv_allocator->deallocate(segment);
				Release();
				return MAKE_ERROR(Error::NoEnoughMemory);
			}
			MemSet(segment, 0, segment_size_ * sizeof(TRB));
			erst_[index].bits.ring_segment_base_address =
				reinterpret_cast<uint64_t>(segment);
			erst_[index].bits.ring_segment_size = segment_size_;
		}

		ERSTSZ_t erstsz = interrupter_->ERSTSZ.Read();
		erstsz.SetSize(segment_count);
		interrupter_->ERSTSZ.Write(erstsz);

		ERSTBA_t erstba = interrupter_->ERSTBA.Read();
		erstba.SetPointer(reinterpret_cast<uint64_t>(erst_));
		interrupter_->ERSTBA.Write(erstba);

		WriteDequeuePointer(segments_[0]);

		return MAKE_ERROR(Error::Success);
	}

	void EventRing::WriteDequeuePointer(TRB* p) {
		auto erdp = interrupter_->ERDP.Read();
		erdp.SetPointer(reinterpret_cast<uint64_t>(p));
		erdp.bits.dequeue_erst_segment_index = dequeue_segment_index_;
		erdp.bits.event_handler_busy = true;
		interrupter_->ERDP.Write(erdp);
	}

	void EventRing::Pop() {
		auto p = ReadDequeuePointer() + 1;
		TRB* segment_begin = segments_[dequeue_segment_index_];
		TRB* segment_end = segment_begin + segment_size_;

		if (p == segment_end) {
			dequeue_segment_index_++;
			if (dequeue_segment_index_ == segments_.Count()) {
				dequeue_segment_index_ = 0;
				cycle_bit_ = !cycle_bit_;
			}
			p = segments_[dequeue_segment_index_];
		}

		WriteDequeuePointer(p);
	}
}

#endif
