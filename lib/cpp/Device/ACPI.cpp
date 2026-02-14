// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Advanced Configuration and Power Interface - Power Management (ACPI)
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#include "../../../inc/cpp/Device/ACPI.hpp"
#include "../../../inc/cpp/string"

#if defined(_MCCA) && (_MCCA & 0xFF00) == 0x8600

bool uni::ACPI::Assert(const RSDP& rsdp) {
	if (!rsdp.isValid()) {
		plogerro("RSDP is not valid\n");
		return false;
	}
	const XSDT& xsdt = *reinterpret_cast<const XSDT*>(rsdp.xsdt_address);
	if (!xsdt.header.isValid("XSDT")) {
		plogerro("XSDT is not valid\n");
		return false;
	}
	fadt = nullptr;
	for (int i = 0; i < xsdt.Count(); ++i) {
		const auto& entry = xsdt[i];
		if (entry.isValid("FACP")) { // FACP is the signature of FADT (rekishi reason)
			fadt = reinterpret_cast<const FADT*>(&entry);
			break;
		}
	}
	if (fadt == nullptr) {
		plogerro("FADT is not found\n");
		return false;
	}
	return true;
}
void uni::ACPI::Delay_ms(unsigned long msec) {
	const unsigned pm_timer_bits = ((fadt->flags >> 8) & 1) ? 32 : 24;
	const uint32 start = innpd(fadt->pm_tmr_blk);
	uint32 end = start + PMTimerFrequency * msec / 1000;
	if (pm_timer_bits != 32) {
		end &= 0x00ffffffu;
	}
	if (end < start) { // overflow
		while (innpd(fadt->pm_tmr_blk) >= start);
	}
	while (innpd(fadt->pm_tmr_blk) < end);
}
const uni::ACPI::FADT* uni::ACPI::fadt = nullptr;


static uint8 SumBytes(const void* ptr, size_t size) {
    uint8 sum = 0;
	for0(i, size) sum += uni::cast<uint8*>(ptr)[i];
    return sum;
}

bool uni::ACPI::RSDP::isValid() const {
	if (StrCompareN(this->signature, "RSD PTR ", 8) != 0) {
		plogerro("invalid signature: %.8s, at %[x]", this->signature, this);
		return false;
	}
	if (this->revision != 2) {
		plogerro("ACPI revision must be 2: %d", this->revision);
		return false;
	}
	if (auto sum = SumBytes(this, 20); sum != 0) {
		plogerro("sum of 20 bytes must be 0: %d", sum);
		return false;
	}
	if (auto sum = SumBytes(this, 36); sum != 0) {
		plogerro("sum of 36 bytes must be 0: %d", sum);
		return false;
	}
	return true;
}

bool uni::ACPI::DescriptionHeader::isValid(const char* expected_signature) const {
	if (StrCompareN(this->signature, expected_signature, 4) != 0) {
		plogerro("invalid signature: %.4s\n", this->signature);
		return false;
	}
	if (auto sum = SumBytes(this, this->length); sum != 0) {
		plogerro("sum of %u bytes must be 0: %d\n", this->length, sum);
		return false;
	}
	return true;
}

const uni::ACPI::DescriptionHeader& uni::ACPI::XSDT::operator[](size_t i) const {
	auto entries = reinterpret_cast<const uint64_t*>(&this->header + 1);
	return *reinterpret_cast<const DescriptionHeader*>(entries[i]);
}

size_t uni::ACPI::XSDT::Count() const {
	return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64);
}


#endif
