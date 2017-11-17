#include "can_bus.h"
#include "settings/settings_can.h"

#ifndef _CAN_BUS_C
#define _CAN_BUS_C

DynamicRegistry<CanNode*> canNodes;
bool canTrueBool = true;

// SystickEvent for handling can heartbeat (must be registered before it is active)
Motate::SysTickEvent can_heartbeat_check {[&] {
	
}, nullptr};

CanEndpoint::CanEndpoint (CanNode *_node) {
	this->node = node;
	this->node->endpoints.addEntry(&this->endpointEntry);
	cm.saftey_interlock_list.addEntry(&this->initInterlock);
	this->CanEndpoint::state = CAN_ENDPOINT_INITALIZING;
}

CanEndpoint::~CanEndpoint () {
	this->node->endpoints.removeEntry(&this->endpointEntry);
}

bool CanDigitalInput::processMessage(CanFrame *frame) {
	if (frame->id == canId) {
		ioDigitalInput::updateValue(frame->data.bytes[0]);
		this->CanEndpoint::state = CAN_ENPOINT_RUNNING;
		return true;
		} else {
		return false;
	}
}

CanDigitalInput::CanDigitalInput(int _canId, ioMode _mode, inputAction _action, inputFunc _function, uint16_t _lockout_ms) : CanEndpoint(this->node), ioDigitalInput(_canId, _mode, _action, _function, _lockout_ms) {
	this->canId = _canId;
	canNodes.iterateOver([&](CanNode *_node) {
		if (_node->nodeId == _canId & (1<<canPhysicalNodeMax - 1)) {
			_node->endpoints.addEntry(&this->endpointEntry);
			this->node = _node;
			return true;
		} else return false;
	});
}

CanNode::CanNode (int _nodeId) {
	Motate::SysTickTimer.registerEvent(&this->heartbeatCheck);
	cm.saftey_interlock_list.addEntry(&this->heartbeatInterlock);
	canNodes.addEntry(&this->nodeEntry);
	this->nodeId = _nodeId;
}
	
CanNode::~CanNode() {
	Motate::SysTickTimer.unregisterEvent(&this->heartbeatCheck);
	cm.saftey_interlock_list.removeEntry(&this->heartbeatInterlock);
}

bool CanNode::processFrame(CanFrame *frame) {
	if (this->nodeId == frame->nodeId()) {
		this->heartbeatCounter = 0;
		
		this->endpoints.iterateOver([&] (CanEndpoint *endpoint) {
			return endpoint->processMessage(frame);
		});
		
		return true;
	} else return false;
}

void can_init() {
	hw_can_init();
}

void can_message_received (CanFrame *frame) {
	if (frame->id == 0) cm_shutdown(STAT_SHUTDOWN, "CAN Estop"); // Shutdown State (EStop)
	else canNodes.iterateOver([&] (CanNode *node) {
		return node->processFrame(frame);
	}); // If it's not an EStop, give it to the registry to process
}

void can_send_frame (CanFrame frame) {
	Can0.sendFrame(frame);
}

#endif