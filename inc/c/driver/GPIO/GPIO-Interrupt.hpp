#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
static Request_t GPIO_Request_list[16] = {
	IRQ_EXTI0, IRQ_EXTI1, IRQ_EXTI2, IRQ_EXTI3,
	IRQ_EXTI4, IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI9_5,
	IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI15_10, IRQ_EXTI15_10,
	IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10
};
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) const {
	FUNC_EXTI[getID()] = fn;
}
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const {
	NVIC.setPriority(GPIO_Request_list[getID()], preempt, sub_priority);
}
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const {
	if (getID() < numsof(GPIO_Request_list))
		NVIC.setAble(GPIO_Request_list[getID()], enable);
}

#else

void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) const { _TODO(void) fn; }
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const { _TODO(void) preempt; (void)sub_priority; }
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const { _TODO(void) enable; }

#endif
