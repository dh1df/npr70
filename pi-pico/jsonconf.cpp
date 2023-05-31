#include "mbed.h"
#include "../source/global_variables.h"

enum type {
	TYPE_BOOL,
	TYPE_STRING15,
	TYPE_INT8,
};
struct config {
	const char *name;
	enum type type;
	void *dest;
	const void *deflt;
} config[]={
	{"is_master",TYPE_BOOL,&is_TDMA_master,(const void *)false},
	{"callsign",TYPE_STRING15,CONF_radio_my_callsign,"N0CALL-1       "},
	{"modulation",TYPE_INT8,CONF_radio_my_callsign,(const void *)22},
};
