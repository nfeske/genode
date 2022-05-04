/*
 * \brief  C-API for audio (work in progress)
 * \author Sebastian Sumpf
 * \date   2022-05-31
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <genode_c_api/base.h>

#ifdef __cplusplus
extern "C" {
#endif

struct genode_audio_packet
{
	short        *data;
	unsigned long samples;
};

enum Jack_mode {
	HEADPHONE,
	HEADSET,
	MICROPHONE,
	DEFAULT = HEADSET
};


static const char *const jack_mode_labels[] = {
	[HEADPHONE]  = "headphone",
	[HEADSET]    = "headset",
	[MICROPHONE] = "microphone",
};


enum Ctrl_type
{
	CTRL_INVALID, CTRL_BOOL, CTRL_INTEGER, CTRL_ENUMERATED
};


struct genode_mixer_control
{
	enum Ctrl_type type;
	char const *type_label;
	unsigned    id;
	unsigned    value_count;
	unsigned    values[2];
	char const *name;
	unsigned    min, max;
	unsigned    enum_count;
	char      **enum_strings;
};


struct genode_mixer_controls
{
	unsigned count;
	struct genode_mixer_control control[64];
};


struct genode_device
{
	bool  valid;
	char *direction;
	char *node;
	char *name;
};


struct genode_devices
{
	struct genode_device device[64][2];
};


void genode_audio_init(struct genode_env *env_ptr,
                       struct genode_allocator *alloc_ptr);

struct genode_audio_packet genode_audio_record(void);
void   genode_audio_play(struct genode_audio_packet packet);

unsigned long genode_audio_samples_per_period(void);

void genode_mixer_report_controls(struct genode_mixer_controls *controls);
void genode_mixer_update_controls(struct genode_mixer_controls *controls);
bool genode_mixer_update(void);
enum Jack_mode genode_jack_mode(void);

void genode_devices_report(struct genode_devices *devices);

#ifdef __cplusplus
}
#endif

