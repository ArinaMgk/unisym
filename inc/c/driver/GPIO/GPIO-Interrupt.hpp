#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) const {
	FUNC_EXTI[bitposi] = fn;
}
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const {
	NVIC.setPriority(GPIO_Request_list[bitposi], preempt, sub_priority);
}
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const {
	if (bitposi < numsof(GPIO_Request_list))
		NVIC.setAble(GPIO_Request_list[bitposi], enable);
}

#else

void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) const { _TODO(void) fn; }
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const { _TODO(void) preempt; (void)sub_priority; }
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const { _TODO(void) enable; }

#endif