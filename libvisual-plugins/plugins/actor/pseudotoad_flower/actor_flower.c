/* Libvisual-plugins - Standard plugins for libvisual
 * 
 * Copyright (C) 2004, 2005 Antti Silvast <asilvast@iki.fi>
 *
 * Authors: Antti Silvast <asilvast@iki.fi>
 *	    Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <libvisual/libvisual.h>

#include "main.h"
#include "notch.h"

typedef struct {
	VisTimer t;
	FlowerInternal flower;
	NOTCH_FILTER notch;
	VisRandomContext *rcxt;
} FlowerPrivate;

int lv_flower_init (VisPluginData *plugin);
int lv_flower_cleanup (VisPluginData *plugin);
int lv_flower_requisition (VisPluginData *plugin, int *width, int *height);
int lv_flower_dimension (VisPluginData *plugin, VisVideo *video, int width, int height);
int lv_flower_events (VisPluginData *plugin, VisEventQueue *events);
VisPalette *lv_flower_palette (VisPluginData *plugin);
int lv_flower_render (VisPluginData *plugin, VisVideo *video, VisAudio *audio);

/* Main plugin stuff */
const VisPluginInfo *get_plugin_info (int *count)
{
	static const VisActorPlugin actor[] = {{
		.requisition = lv_flower_requisition,
		.palette = lv_flower_palette,
		.render = lv_flower_render,
		.depth = VISUAL_VIDEO_DEPTH_GL
	}};

	static const VisPluginInfo info[] = {{
		.struct_size = sizeof (VisPluginInfo),
		.api_version = VISUAL_PLUGIN_API_VERSION,
		.type = VISUAL_PLUGIN_TYPE_ACTOR,

		.plugname = "lv_flower",
		.name = "libvisual Pseudotoad flower, yellow rose of texas",
		.author = "Original by: Antti Silvast <asilvast@iki.fi>, Port by: Dennis Smit <ds@nerds-incorporated.org>",
		.version = "0.1",
		.about = "The Libvisual yellow rose of texas port",
		.help =  "This renders an awesome responsive flower.",

		.init = lv_flower_init,
		.cleanup = lv_flower_cleanup,
		.events = lv_flower_events,

		.plugin = VISUAL_OBJECT (&actor[0])
	}};
	
	*count = sizeof (info) / sizeof (*info);

	return info;
}

int lv_flower_init (VisPluginData *plugin)
{
	FlowerPrivate *priv;

	priv = visual_mem_new0 (FlowerPrivate, 1);
	visual_object_set_private (VISUAL_OBJECT (plugin), priv);

	priv->rcxt = visual_plugin_get_random_context (plugin);

	visual_random_context_float (priv->rcxt);
	init_flower (&priv->flower);

	priv->flower.rotx = visual_random_context_float (priv->rcxt) * 360.0;
	priv->flower.roty = visual_random_context_float (priv->rcxt) * 360.0;

	priv->flower.tension = (visual_random_context_float (priv->rcxt) - 0.5) * 8.0;
	priv->flower.continuity = (visual_random_context_float (priv->rcxt) - 0.5) * 16.0;
	
	return 0;
}

int lv_flower_cleanup (VisPluginData *plugin)
{
	FlowerPrivate *priv = visual_object_get_private (VISUAL_OBJECT (plugin));

	visual_mem_free (priv);

	return 0;
}

int lv_flower_requisition (VisPluginData *plugin, int *width, int *height)
{
	int reqw, reqh;

	reqw = *width;
	reqh = *height;

	if (reqw < 1)
		reqw = 1;

	if (reqh < 1)
		reqh = 1;

	*width = reqw;
	*height = reqh;

	return 0;
}

int lv_flower_dimension (VisPluginData *plugin, VisVideo *video, int width, int height)
{
	FlowerPrivate *priv = visual_object_get_private (VISUAL_OBJECT (plugin));
	GLfloat ratio;
	
	visual_video_set_dimension (video, width, height);

	ratio = (GLfloat) width / (GLfloat) height;

	glViewport (0, 0, (GLsizei) width, (GLsizei) height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	gluPerspective (45.0, ratio, 0.1, 100.0);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	priv->flower.width = width;
	priv->flower.height = height;

	return 0;
}

int lv_flower_events (VisPluginData *plugin, VisEventQueue *events)
{
	FlowerPrivate *priv = visual_object_get_private (VISUAL_OBJECT (plugin));
	VisEvent ev;
	VisParamEntry *param;

	while (visual_event_queue_poll (events, &ev)) {
		switch (ev.type) {
			case VISUAL_EVENT_RESIZE:
				lv_flower_dimension (plugin, ev.resize.video,
						ev.resize.width, ev.resize.height);
				break;

			default: /* to avoid warnings */
				break;
		}
	}

	return 0;
}

VisPalette *lv_flower_palette (VisPluginData *plugin)
{
	return NULL;
}

int lv_flower_render (VisPluginData *plugin, VisVideo *video, VisAudio *audio)
{
	FlowerPrivate *priv = visual_object_get_private (VISUAL_OBJECT (plugin));

	/* Activate the effect change timer */
	if (visual_timer_is_active (&priv->t) == FALSE)
		visual_timer_start (&priv->t);

	/* At 15 secs, do with new settings, reset timer */
	if (visual_timer_has_passed_by_values (&priv->t, 15, 0)) {
		priv->flower.tension_new = (-visual_random_context_float (priv->rcxt)) * 12.0;
		priv->flower.continuity_new = (visual_random_context_float (priv->rcxt) - 0.5) * 32.0;

		visual_timer_start (&priv->t);
	}

	/* Activate global timer */
	if (visual_timer_is_active (&priv->flower.timer) == FALSE)
		visual_timer_start (&priv->flower.timer);

	
	{
#define HEIGHT 1.0
#define D 0.45
#define TAU 0.25
#define DIF 5.0
		float lk=2.0;
		float l0=2.0;
		int nof_bands = 32;

		float scale = HEIGHT / ( log((1 - D) / D) * 2 );
		float x00 = D*D*1.0/(2 * D - 1);
		float y00 = -log(-x00) * scale;
		float y;
		int i;
		for (i=0; i<nof_bands; i++) {
			y=audio->freqnorm[2][i * 8];
			y=y*(i*lk+l0);

			y = ( log(y - x00) * scale + y00 ); /* Logarithmic amplitude */

			y = ( (DIF-2.0)*y +
					(i==0       ? 0 : audio->freqnorm[2][(i * 8) - 1]) +
					(i==31 ? 0 : audio->freqnorm[2][(i * 8) + 1])) / DIF;

			y=((1.0-TAU)*priv->flower.audio_bars[i]+TAU*y) / 100.00;
			priv->flower.audio_bars[i]=y;
		}
	}


	priv->flower.roty += (priv->flower.audio_bars[15]) * 0.6;
	priv->flower.rotx += (priv->flower.audio_bars[1]) * 0.7;
	priv->flower.posz = +1;

	render_flower_effect (&priv->flower);

	return 0;
}
