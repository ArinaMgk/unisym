// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output
// Codifiers: @dosconio: 20240412
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
#include "../../../../inc/cpp/Device/GPIO"

namespace uni {
	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	stduint GeneralPurposeInputOutputPort::getPinCount() const {
		const auto* backend = GPIO.getBackend(getID());
		return backend ? backend->pin_count : 0;
	}

	uint32 GeneralPurposeInputOutputPort::getCapabilities() const {
		const auto* backend = GPIO.getBackend(getID());
		return backend ? backend->capabilities : 0;
	}

	bool GeneralPurposeInputOutputPort::Available() const {
		return GPIO.getBackend(getID()) != nullptr;
	}

	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOInterruptTrigger trigger, GPIOInterruptPolarity polarity, Handler_t handler) const {
		const GPIOInterruptConfiguration configuration{ trigger, polarity, 0, false, false };
		return ConfigureInterrupt(configuration, handler);
	}

	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::ConfigureInterrupt(const GPIOInterruptConfiguration& configuration, Handler_t handler) const {
		const auto* backend = ResolveGPIOBackend(self);
		if (!backend || !backend->configure_interrupt || !backend->configure_interrupt(backend->context, getID(), configuration)) return self;
		if (handler && backend->set_interrupt) backend->set_interrupt(backend->context, getID(), handler);
		return self;
	}

	bool GeneralPurposeInputOutputPin::InterruptPending() const {
		const auto* backend = ResolveGPIOBackend(self);
		bool pending = false;
		return backend && backend->interrupt_pending && backend->interrupt_pending(backend->context, getID(), pending) && pending;
	}

	bool GeneralPurposeInputOutputPin::AcknowledgeInterrupt() const {
		const auto* backend = ResolveGPIOBackend(self);
		return backend && backend->acknowledge_interrupt && backend->acknowledge_interrupt(backend->context, getID());
	}

	bool GeneralPurposeInputOutput::Attach(const GeneralPurposeInputOutputPortBackend& backend, stduint& portid) {
		if (!backend.context || !backend.pin_count || !backend.configure_pin) return false;
		if ((backend.capabilities & GeneralPurposeInputOutputCapability_Input) && !backend.read_pin) return false;
		if ((backend.capabilities & GeneralPurposeInputOutputCapability_Output) && !backend.write_pin) return false;
		if ((backend.capabilities & (GeneralPurposeInputOutputCapability_InterruptEdge | GeneralPurposeInputOutputCapability_InterruptLevel)) && !backend.configure_interrupt) return false;
		if ((backend.capabilities & (GeneralPurposeInputOutputCapability_PullUp | GeneralPurposeInputOutputCapability_PullDown)) && !backend.set_pull) return false;
		if ((backend.capabilities & GeneralPurposeInputOutputCapability_Pending) && !backend.interrupt_pending) return false;
		if ((backend.capabilities & GeneralPurposeInputOutputCapability_Mask) && !backend.enable_interrupt) return false;
		if ((backend.capabilities & GeneralPurposeInputOutputCapability_Acknowledge) && !backend.acknowledge_interrupt) return false;
		constexpr stduint MaximumPortCount = 256;
		if (!port_backends) {
			port_backends = new GeneralPurposeInputOutputPortBackend[MaximumPortCount]{};
			if (!port_backends) return false;
			port_backend_capacity = MaximumPortCount;
		}
		for (stduint index = 1; index < port_backend_capacity; ++index) {
			if (port_backends[index].context) continue;
			port_backends[index] = backend;
			portid = index;
			return true;
		}
		return false;
	}

	bool GeneralPurposeInputOutput::Detach(stduint portid, void* context) {
		if (!port_backends || portid >= port_backend_capacity || !port_backends[portid].context) return false;
		if (context && port_backends[portid].context != context) return false;
		port_backends[portid] = {};
		return true;
	}

	const GeneralPurposeInputOutputPortBackend* GeneralPurposeInputOutput::getBackend(stduint portid) const {
		if (!port_backends || portid >= port_backend_capacity || !port_backends[portid].context) return nullptr;
		return port_backends + portid;
	}
	#endif
}
