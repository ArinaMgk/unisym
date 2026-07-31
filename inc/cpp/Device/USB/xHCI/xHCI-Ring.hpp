
namespace uni::device::SpaceUSB3 {
	/** @brief Class representing a Command/Transfer Ring. */
	class Ring {
	public:
		Ring() = default;
		Ring(const Ring&) = delete;
		~Ring();
		Ring& operator=(const Ring&) = delete;

		/** @brief Allocate memory for the ring and initialize members. */
		Error Initialize(size_t buf_size);

		/** @brief Append a TRB to the end of the ring with the cycle bit set.
			 *
			 * @return Pointer to the appended TRB (on the ring).
			 */
		template <typename TRBType>
		TRB* Push(const TRBType& trb) {
			return Push(trb.data);
		}

		TRB* Buffer() const { return buf_; }

	private:
		TRB* buf_ = nullptr;
		size_t buf_size_ = 0;

		/** @brief Bit representing the producer cycle state. */
		bool cycle_bit_;
		/** @brief Next write position on the ring. */
		size_t write_index_;

		/** @brief Write a TRB to the end of the ring with the cycle bit set.
			 *
			 * write_index_ is not changed.
			 */
		void CopyToLast(const std::array<uint32_t, 4>& data);

		/** @brief Append a TRB to the end of the ring with the cycle bit set.
			 *
			 * Increments write_index_. If write_index_ reaches the end of the ring,
			 * places a LinkTRB appropriately, resets write_index_ to 0,
			 * and toggles the cycle bit.
			 *
			 * @return Pointer to the appended TRB (on the ring).
			 */
		TRB* Push(const std::array<uint32_t, 4>& data);
	};

	union EventRingSegmentTableEntry {
		std::array<uint32_t, 4> data;
		struct {
			uint64_t ring_segment_base_address;  // 64-byte alignment

			uint32_t ring_segment_size : 16;
			uint32_t : 16;

			uint32_t : 32;
		} __attribute__((packed)) bits;
	};

	class EventRing {
	public:
		Error Initialize(size_t buf_size, InterrupterRegisterSet* interrupter);

		TRB* ReadDequeuePointer() const {
			return reinterpret_cast<TRB*>(interrupter_->ERDP.Read().Pointer());
		}

		void WriteDequeuePointer(TRB* p);

		bool HasFront() const {
			return Front()->bits.cycle_bit == cycle_bit_;
		}

		TRB* Front() const {
			return ReadDequeuePointer();
		}

		void Pop();

	private:
		TRB* buf_;
		size_t buf_size_;

		bool cycle_bit_;
		EventRingSegmentTableEntry* erst_;
		InterrupterRegisterSet* interrupter_;
	};
}
