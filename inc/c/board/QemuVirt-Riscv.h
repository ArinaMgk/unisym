// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Board) Qemu Virt-Riscv
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

#ifndef _INC_Board_QEMUVIRT_RISCV
#define _INC_Board_QEMUVIRT_RISCV

/*
- [qemu/hw/riscv/virt.c at master · qemu/qemu · GitHub](https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c) 

(get at 20250913)
```c
static const MemMapEntry virt_memmap[] = {
    [VIRT_DEBUG] =        {        0x0,         0x100 },
    [VIRT_MROM] =         {     0x1000,        0xf000 }, /// Bootloader 0x1000 ~ 0xFFFF
    [VIRT_TEST] =         {   0x100000,        0x1000 },
    [VIRT_RTC] =          {   0x101000,        0x1000 },
    [VIRT_CLINT] =        {  0x2000000,       0x10000 },
    [VIRT_ACLINT_SSWI] =  {  0x2F00000,        0x4000 },
    [VIRT_PCIE_PIO] =     {  0x3000000,       0x10000 },
    [VIRT_IOMMU_SYS] =    {  0x3010000,        0x1000 },
    [VIRT_PLATFORM_BUS] = {  0x4000000,     0x2000000 },
    [VIRT_PLIC] =         {  0xc000000, VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2) },
    [VIRT_APLIC_M] =      {  0xc000000, APLIC_SIZE(VIRT_CPUS_MAX) },
    [VIRT_APLIC_S] =      {  0xd000000, APLIC_SIZE(VIRT_CPUS_MAX) },
    [VIRT_UART0] =        { 0x10000000,         0x100 },
    [VIRT_VIRTIO] =       { 0x10001000,        0x1000 },
    [VIRT_FW_CFG] =       { 0x10100000,          0x18 },
    [VIRT_FLASH] =        { 0x20000000,     0x4000000 },
    [VIRT_IMSIC_M] =      { 0x24000000, VIRT_IMSIC_MAX_SIZE },
    [VIRT_IMSIC_S] =      { 0x28000000, VIRT_IMSIC_MAX_SIZE },
    [VIRT_PCIE_ECAM] =    { 0x30000000,    0x10000000 },
    [VIRT_PCIE_MMIO] =    { 0x40000000,    0x40000000 },
    [VIRT_DRAM] =         { 0x80000000,           0x0 },  /// Kernel 0x8000_0000 ~ 0x8800_0000+
};
```
*/

#define ADDR_VIRT_DEBUG 0x00000000
#define ADDR_VIRT_MROM  0x00001000
#define ADDR_VIRT_TEST  0x00100000
#define ADDR_VIRT_RTC   0x00101000
#define ADDR_VIRT_CLINT 0x02000000
#define ADDR_VIRT_ACLINT_SSWI 0x02F00000
#define ADDR_VIRT_PCIE_PIO 0x03000000
#define ADDR_VIRT_IOMMU_SYS 0x03010000
#define ADDR_VIRT_PLATFORM_BUS 0x04000000
#define ADDR_VIRT_PLIC    _IMM(0x0C000000)
#define ADDR_VIRT_APLIC_M 0x0C000000
#define ADDR_VIRT_APLIC_S 0x0D000000
#define ADDR_VIRT_UART0 0x10000000
#define ADDR_VIRT_VIRTIO 0x10001000
#define ADDR_VIRT_FW_CFG 0x10100000
#define ADDR_VIRT_FLASH 0x20000000
#define ADDR_VIRT_IMSIC_M 0x24000000
#define ADDR_VIRT_IMSIC_S 0x28000000
#define ADDR_VIRT_PCIE_ECAM 0x30000000
#define ADDR_VIRT_PCIE_MMIO 0x40000000
#define ADDR_VIRT_DRAM 0x80000000

#define ADDR_CLINT_BASE 0x2000000L
#define ADDR_CLINT_MSIP(hartid) (ADDR_CLINT_BASE + 4 * (hartid))
#define ADDR_CLINT_MTIMECMP(hartid) (ADDR_CLINT_BASE + 0x4000 + 8 * (hartid))
#define ADDR_CLINT_MTIME (ADDR_CLINT_BASE + 0xBFF8) // cycles since boot.

// ---- ---- CLINT ---- ---- //
// https://gitee.com/qemu/qemu/blob/master/include/hw/riscv/sifive_clint.h
#define CLINT_TIMEBASE_FREQ 10000000


#endif // _INC_Board_QEMUVIRT_RISCV
