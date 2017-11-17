#include "g2core.h"
#include "hardware.h"
#include "gpio.h"
#include "xio.h"
#include "dynamic_registry.h"
#include "MotateTimers.h"
#include "canonical_machine.h"
#include "board_can.h"

#ifndef CAN_H_ONCE
#define CAN_H_ONCE

const int canIDLength = 11;
const int canIDTypeBits = 7;
const int canPhysicalNodeMax = (1 << (canIDLength - canIDTypeBits));
const int canHeartbeat = 62;
const int canHeartbeatIDStart = canHeartbeat * canPhysicalNodeMax;
const int canPoll = 63;
const int canPollIDStart = canPoll * canPhysicalNodeMax;
const int canHeartbeatTimeout = 100;
extern bool canTrueBool;

typedef union {
	uint64_t value;
	struct {
		uint32_t low;
		uint32_t high;
	};
	struct {
		uint16_t s0;
		uint16_t s1;
		uint16_t s2;
		uint16_t s3;
	};
	uint8_t bytes[8];
	uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

typedef struct CanFrame {
	uint32_t id;		// EID if id set, SID otherwise
	uint32_t fid;		// family ID
	uint8_t rtr;		// Remote Transmission Request
	uint8_t priority;	// Priority but only important for TX frames and then only for special uses.
	uint8_t extended;	// Extended ID flag
	uint16_t time;      // CAN timer value when mailbox message was received.
	uint8_t length;		// Number of data bytes
	BytesUnion data;	// 64 bits - lots of ways to access it.
	
	uint32_t nodeId () {
		return this->id & (1<<canPhysicalNodeMax - 1);
	}
} CanFrame;

class CanNode;
void can_send_frame (CanFrame frame);
void can_init();

typedef enum {
	CAN_ENDPOINT_INITALIZING,
	CAN_ENPOINT_RUNNING,
	CAN_ENDPOINT_ERROR
} CanEndpointState;

class CanEndpoint {
	public:
	CanNode *node;
	bool processMessage(CanFrame *frame) {}; // Processes a message on the bus and decides if a message should continue to propagate (return)
	DynamicRegistry<CanEndpoint*>::EntryType endpointEntry = {this, nullptr};
	DynamicRegistry<bool*>::EntryType initInterlock = {&canTrueBool, nullptr};
	CanEndpointState state = CAN_ENDPOINT_INITALIZING;
	
	CanEndpoint (CanNode *_node);
	~CanEndpoint ();
};

class CanNode {
	public:
	DynamicRegistry<CanEndpoint*> endpoints;
	
	int	nodeId;
	DynamicRegistry<CanNode*>::EntryType nodeEntry = {this, nullptr};
	
	int heartbeatCounter = canHeartbeatTimeout;
	bool heartbeatInterlockBool = true;
	DynamicRegistry<bool*>::EntryType heartbeatInterlock = {&heartbeatInterlockBool, nullptr};
	
	Motate::SysTickEvent heartbeatCheck {[&] {
		heartbeatCounter++;
		heartbeatInterlockBool = (heartbeatCounter > canHeartbeatTimeout);
	}, nullptr};
	
	CanNode (int _nodeId);
	~CanNode();
	
	void poll() {
		CanFrame frame;
		frame.id = canPollIDStart + this->nodeId;
		frame.length = 0;
		can_send_frame(frame); // Poll the node
	}
	
	bool processFrame (CanFrame *frame);
};

class CanDigitalInput : public CanEndpoint, public ioDigitalInput {
	public:
	int canId;

	bool processMessage(CanFrame *frame);
	
	void reset () {
		ioDigitalInput::reset();
		this->CanEndpoint::state = CAN_ENDPOINT_INITALIZING;
		this->node->poll();
	}
	
	CanDigitalInput(int _canId, ioMode _mode = IO_MODE_DISABLED, inputAction _action = INPUT_ACTION_NONE, inputFunc _function = INPUT_FUNCTION_NONE, uint16_t _lockout_ms = 0);
};

extern DynamicRegistry<CanNode*> canNodes;

#endif
