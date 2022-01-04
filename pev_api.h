#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pevulib.h>

#define NUMBER_OF_CHANNELS	12
#define ADDRESS_SET_REF_C	175
#define ADDRESS_SET_REF_Q	144
#define ADDRESS_ILOAD_C		153
#define ADDRESS_ILOAD_Q		148
#define BASE_CHANNEL_OFFSET 0x100
#define EVENT_ID            0x40
#define EVENT_ID_BITS		0xf0
#define EVENT_CHANNEL_BITS	0x0f
#define REGISTER_OPERATION_WRITE	0x0080000000000000ULL	// b56 == "w/!r"
#define REGISTER_OPERATION_READ		0x0000000000000000ULL	// b56 == "w/!r"
#define REGISTER_LINK_PS_SHIFT		46
#define TSR_ERROR					0x8000

#define PEV_OK				0
#define PEV_ERR_CHANNEL 	1
#define PEV_ERR_REGISTER	2

enum
{
	REGISTER_PRIORITY_WRITE	= 0,
	REGISTER_NORMAL_WRITE,
	REGISTER_NORMAL_READ,
	REGISTER_WAVEFORM_WRITE,
	REGISTER_WAVEFORM_READ,
	NUMBER_OF_IO_REGISTERS,	/*Reserved*/
	REGISTER_MSR,
	REGISTER_TSR,
	NUMBER_OF_REGISTERS
};

typedef uint64_t u64;
typedef uint32_t u32;
typedef int64_t  i64;
typedef int32_t  i32;

typedef struct
{
	uint64_t registers[NUMBER_OF_REGISTERS];
} channel_t;

static channel_t* channels;
static struct pev_ioctl_map_pg map;
static struct pev_node *node;
static struct pev_ioctl_evt *event;
static void	*base;

static void pev_initialize();
static void pev_cleanup(int code);
static int  pev_read(u32 channel, u64 address, u64 ps, float* value);
static int  pev_write(u32 channel, u64 address, u64 ps, float value);


