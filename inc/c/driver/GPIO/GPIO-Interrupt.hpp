#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
static Request_t GPIO_Request_list[16] = {
	IRQ_EXTI0, IRQ_EXTI1, IRQ_EXTI2, IRQ_EXTI3,
	IRQ_EXTI4, IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI9_5,
	IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI15_10, IRQ_EXTI15_10,
	IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10
};
#elif defined(_MPU_STM32MP13)
static Request_t GPIO_Request_list[16] = {
	IRQ_EXTI0, IRQ_EXTI1, IRQ_EXTI2, IRQ_EXTI3,
	IRQ_EXTI4, IRQ_EXTI5, IRQ_EXTI6, IRQ_EXTI7,
	IRQ_EXTI8, IRQ_EXTI9, IRQ_EXTI10, IRQ_EXTI11,
	IRQ_EXTI12, IRQ_EXTI13, IRQ_EXTI14, IRQ_EXTI15
};
#endif


void GeneralPurposeInputOutputPin::setInterrupt(Handler_t f) const {
#if defined(_MCU_STM32)
	FUNC_EXTI[getID()] =
#else
	(void)
#endif
		f;
}

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const {
	NVIC.setPriority(GPIO_Request_list[getID()], preempt, sub_priority);
}
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const {
	if (getID() < numsof(GPIO_Request_list))
		NVIC.setAble(GPIO_Request_list[getID()], enable);
}

#elif defined(_MPU_STM32MP13)
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const {
	GIC.setPriority(GPIO_Request_list[getID()], preempt);// preempt ?
	(void)sub_priority;
}
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const {
	if (getID() < numsof(GPIO_Request_list))
		GIC.enInterrupt(GPIO_Request_list[getID()], enable);
}
#else
void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) const { _TODO(void) preempt; (void)sub_priority; }
void GeneralPurposeInputOutputPin::enInterrupt(bool enable) const { _TODO(void) enable; }

#endif
