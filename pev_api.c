#include "pev_api.h"

void pev_initialize()
{
	int status;

	node = pev_init(0);
	if(!node) 
	{
		printf("[psc][pev] Initialization failed");
		return;
	}
	if(node->fd < 0) 
	{
		printf("[psc][pev] Can't find PEV1100 interface");
		return;
	}

	/*
	 * Map PSC registers
	 */
	map.rem_addr = 0;
	map.mode  = MAP_ENABLE | MAP_ENABLE_WR | MAP_SPACE_USR1;
	map.flag  = 0;
	map.sg_id = MAP_MASTER_32;
	map.size  = 0x400000;
	status    = pev_map_alloc(&map);

	base = pev_mmap(&map);
	channels = (channel_t*)(base + BASE_CHANNEL_OFFSET);

	/*
	 * Allocate event queue, register channel event, and enable it
	 */
	event = pev_evt_queue_alloc(0);
	if(!event)
	{ 
		printf("[psc][pev] Can't allocate event queue");
		return;
	}
	event->wait = -1;

	int c = 0;
	for(c = 0; c < NUMBER_OF_CHANNELS; c++)
	{
		status = pev_evt_register (event, EVENT_ID + c);
		if(status)
		{
			printf("[psc][pev] Event queue register failed");
			return;
		}	
		status = pev_evt_queue_enable(event);
		if(status)
		{
			printf("[psc][pev] Event queue enable failed");
			return;
		}
	}
}

void pev_cleanup(int code)
{
	uint32_t channel;

	pev_evt_read(event, 1);
	pev_evt_unmask(event, event->src_id);
	if(base)
		pev_map_free(&map);

	if(event)
	{
		pev_evt_queue_disable(event);
		for (channel = 0; channel < NUMBER_OF_CHANNELS; channel++)
			pev_evt_unregister (event, EVENT_ID+channel);

		pev_evt_queue_free(event);
	}

	pev_exit(node);
	exit(code);
}

int pev_read(u32 channel, u64 address, u64 ps, float* value)
{
	u32 raw, status;

	channels[channel].registers[REGISTER_NORMAL_READ]  = REGISTER_OPERATION_READ  | ps | address << 32 | address;
	pev_evt_read(event, -1);
	pev_evt_unmask(event, event->src_id);
	if ((event->src_id & EVENT_ID_BITS) != EVENT_ID)
		return PEV_ERR_CHANNEL;

	channel = event->src_id & EVENT_CHANNEL_BITS;
	status = (uint32_t)channels[channel].registers[REGISTER_TSR];
	if (status & TSR_ERROR)
		return PEV_ERR_REGISTER;

	raw = (uint32_t)(channels[channel].registers[REGISTER_NORMAL_READ] & 0x00000000ffffffffULL);
	*value = *(float*) &raw;
	return PEV_OK;
}

int pev_write(u32 channel, u64 address, u64 ps, float value)
{
	u32 raw = *(u32*) &value;
	u32 status;

	channels[channel].registers[REGISTER_NORMAL_WRITE] = REGISTER_OPERATION_WRITE | ps | address << 32 | (uint64_t) raw;
	pev_evt_read(event, -1);
	pev_evt_unmask(event, event->src_id);
	if ((event->src_id & EVENT_ID_BITS) != EVENT_ID)
		return PEV_ERR_CHANNEL;

	channel = event->src_id & EVENT_CHANNEL_BITS;
	status = (uint32_t)channels[channel].registers[REGISTER_TSR];
	if (status & TSR_ERROR)
		return PEV_ERR_REGISTER;

	return PEV_OK;
}
