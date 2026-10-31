#ifndef _XHCI_TEMPLATE_HPP_
#define _XHCI_TEMPLATE_HPP_

// x86_64 part of USB Driver is borrowed from uchan-nos/mikanos
// - https://github.com/uchan-nos/mikanos
// cited sincerely.

namespace ext {

	template <typename T>
	struct ArrayLength {};

	template <typename T, size_t N>
	struct ArrayLength<T[N]> {
		static const size_t value = N;
	};

	/**
	 * MemMapRegister is a wrapper for a memory mapped register.
	 *
	 * MemMapRegister forces user program to read/write the underlying register
	 * with specific bit width. The bit width will be deduced from the type of
	 * T::data. T is the template parameter. T::data should be an array.
	 */
	// 20260528: use inline assembly to avoid over OPTIMIZATION by GCC 16.1.1+
	#if defined(__GNUC__) || defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	#endif
	template <typename T>
	class MemMapRegister {
	public:
		T Read() const {
			T tmp;
			if constexpr (sizeof(T) == 1) {
				uint8_t val;
				asm volatile ("movb %1, %0" : "=r"(val) : "m"(*(volatile uint8_t*)&value_) : "memory");
				*(uint8_t*)&tmp = val;
			} else if constexpr (sizeof(T) == 2) {
				uint16_t val;
				asm volatile ("movw %1, %0" : "=r"(val) : "m"(*(volatile uint16_t*)&value_) : "memory");
				*(uint16_t*)&tmp = val;
			} else if constexpr (sizeof(T) == 4) {
				uint32_t val;
				asm volatile ("movl %1, %0" : "=r"(val) : "m"(*(volatile uint32_t*)&value_) : "memory");
				*(uint32_t*)&tmp = val;
			} else if constexpr (sizeof(T) == 8) {
				uint64_t val;
				if constexpr (sizeof(uintptr_t) == 8) {
					asm volatile ("movq %1, %0" : "=r"(val) : "m"(*(volatile uint64_t*)&value_) : "memory");
				} else {
					auto* words = reinterpret_cast<const volatile uint32_t*>(&value_);
					uint32_t low, high;
					asm volatile ("movl %1, %0" : "=r"(low) : "m"(words[0]) : "memory");
					asm volatile ("movl %1, %0" : "=r"(high) : "m"(words[1]) : "memory");
					val = uint64_t(low) | (uint64_t(high) << 32);
				}
				*(uint64_t*)&tmp = val;
			} else {
				for (size_t i = 0; i < len_; ++i) {
					tmp.data[i] = value_.data[i];
				}
			}
			return tmp;
		}

		void Write(const T& value) {
			if constexpr (sizeof(T) == 1) {
				uint8_t val = *(uint8_t*)&value;
				asm volatile ("movb %0, %1" : : "r"(val), "m"(*(volatile uint8_t*)&value_) : "memory");
			} else if constexpr (sizeof(T) == 2) {
				uint16_t val = *(uint16_t*)&value;
				asm volatile ("movw %0, %1" : : "r"(val), "m"(*(volatile uint16_t*)&value_) : "memory");
			} else if constexpr (sizeof(T) == 4) {
				uint32_t val = *(uint32_t*)&value;
				asm volatile ("movl %0, %1" : : "r"(val), "m"(*(volatile uint32_t*)&value_) : "memory");
			} else if constexpr (sizeof(T) == 8) {
				uint64_t val = *(uint64_t*)&value;
				if constexpr (sizeof(uintptr_t) == 8) {
					asm volatile ("movq %0, %1" : : "r"(val), "m"(*(volatile uint64_t*)&value_) : "memory");
				} else {
					// xHCI 1.2 section 5.1: two DWORD writes must be low first, then high.
					auto* words = reinterpret_cast<volatile uint32_t*>(&value_);
					asm volatile ("movl %1, %0" : "=m"(words[0]) : "r"(uint32_t(val)) : "memory");
					asm volatile ("movl %1, %0" : "=m"(words[1]) : "r"(uint32_t(val >> 32)) : "memory");
				}
			} else {
				for (size_t i = 0; i < len_; ++i) {
					value_.data[i] = value.data[i];
				}
			}
		}

	private:
		volatile T value_;
		static const size_t len_ = ArrayLength<decltype(T::data)>::value;
	};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

	template <typename T>
	struct DefaultBitmap {
		T data[1];

		DefaultBitmap& operator =(const T& value) {
			data[0] = value;
		}
		operator T() const { return data[0]; }
	};

	/*
	 * Design: container-like classes.
	 *
	 * Container-like classes, such as PortArray and DeviceContextArray,
	 * should have Size() method and Iterator type.
	 * Size() should return the number of elements, and iterators
	 * of that type should iterate all elements.
	 *
	 * Each element may have a flag indicating availableness of the element.
	 * For example each port has "Port Enabled/Disabled" bit.
	 * Size() and iterators should not skip disabled elements.
	 */

	template <typename T>
	class ArrayWrapper {
	public:
		using ValueType = T;
		using Iterator = ValueType*;
		using ConstIterator = const ValueType*;

		ArrayWrapper(uintptr_t array_base_addr, size_t size)
			: array_(reinterpret_cast<ValueType*>(array_base_addr)),
			size_(size) {
		}

		size_t Size() const { return size_; }

		// begin, end, cbegin, cend must be lower case names
		// to be used in rage-based for statements.
		Iterator begin() { return array_; }
		Iterator end() { return array_ + size_; }
		ConstIterator cbegin() const { return array_; }
		ConstIterator cend() const { return array_ + size_; }

		ValueType& operator [](size_t index) { return array_[index]; }

	private:
		ValueType* const array_;
		const size_t size_;
	};

}

// ---- ----


#endif
