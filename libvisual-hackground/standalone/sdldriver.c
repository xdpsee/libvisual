#include <SDL/SDL.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sdldriver.h"

#define SDL_NATIVE(obj)					(VISUAL_CHECK_CAST ((obj), SDLNative))


typedef struct _SDLNative SDLNative;

static int native_create (SADisplay *display, VisVideoDepth depth, int width, int height, int resizable);
static int native_lock (SADisplay *display);
static int native_unlock (SADisplay *display);
static int native_fullscreen (SADisplay *display, int fullscreen);
static int native_getvideo (SADisplay *display, VisVideo *screen);
static int native_updaterect (SADisplay *display, VisRectangle *rect);
static int native_drainevents (SADisplay *display, VisEventQueue *eventqueue);

static int sdl_initialized;

struct _SDLNative {
	VisObject object;

	SDL_Surface *screen;
};

SADisplayDriver *sdl_driver_new ()
{
	SADisplayDriver *driver;

	driver = visual_mem_new0 (SADisplayDriver, 1);

	visual_object_initialize (VISUAL_OBJECT (driver), TRUE, NULL);

	driver->create = native_create;
	driver->lock = native_lock;
	driver->unlock = native_unlock;
	driver->fullscreen = native_fullscreen;
	driver->getvideo = native_getvideo;
	driver->updaterect = native_updaterect;
	driver->drainevents = native_drainevents;

	return driver;
}

static int native_create (SADisplay *display, VisVideoDepth depth, int width, int height, int resizable)
{
	SDLNative *native = SDL_NATIVE (display->native);
        const SDL_VideoInfo *videoinfo;
	int videoflags = 0;
	int bpp;

	if (native == NULL) {
		native = visual_mem_new0 (SDLNative, 1);

		visual_object_initialize (VISUAL_OBJECT (native), TRUE, NULL);
	}

	if (native->screen != NULL) {
		SDL_FreeSurface (native->screen);
	}

	videoflags |= resizable ? SDL_RESIZABLE : 0;

	if (sdl_initialized == FALSE) {
		if (SDL_Init (SDL_INIT_VIDEO) < 0) {
			fprintf (stderr, "Unable to init SDL VIDEO: %s\n", SDL_GetError ());

			exit (0);
		}

		sdl_initialized = TRUE;
	}

	if (depth == VISUAL_VIDEO_DEPTH_GL) {
		videoinfo = SDL_GetVideoInfo ();

		if (!videoinfo) {
			return -1;
		}

		videoflags |= SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;

		if (videoinfo->hw_available)
			videoflags |= SDL_HWSURFACE;
		else
			videoflags |= SDL_SWSURFACE;

		if (videoinfo->blit_hw)
			videoflags |= SDL_HWACCEL;

		SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

		bpp = videoinfo->vfmt->BitsPerPixel;
		native->screen = SDL_SetVideoMode (width, height, bpp, videoflags);

	} else {
		native->screen = SDL_SetVideoMode (width, height,
				visual_video_depth_value_from_enum (depth),
				videoflags);
	}

	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	display->native = VISUAL_OBJECT (native);

	return 0;
}

static int native_lock (SADisplay *display)
{
	SDLNative *native = SDL_NATIVE (display->native);
	SDL_Surface *screen = native->screen;

	if (SDL_MUSTLOCK (screen) == SDL_TRUE)
		SDL_LockSurface (screen);

	return 0;
}

static int native_unlock (SADisplay *display)
{
	SDLNative *native = SDL_NATIVE (display->native);
	SDL_Surface *screen = native->screen;

	if (SDL_MUSTLOCK (screen) == SDL_TRUE)
		SDL_UnlockSurface (screen);

	return 0;
}

static int native_fullscreen (SADisplay *display, int fullscreen)
{
	SDLNative *native = SDL_NATIVE (display->native);
	SDL_Surface *screen = native->screen;

	if (fullscreen == TRUE) {
		if (!(screen->flags & SDL_FULLSCREEN))
			SDL_WM_ToggleFullScreen (screen);
	} else {
		if ((screen->flags & SDL_FULLSCREEN))
			SDL_WM_ToggleFullScreen (screen);
	}
}

static int native_getvideo (SADisplay *display, VisVideo *screen)
{
	SDLNative *native = SDL_NATIVE (display->native);
	SDL_Surface *sdlscreen = native->screen;

	visual_video_set_depth (screen, visual_video_depth_enum_from_value (sdlscreen->format->BitsPerPixel));
	visual_video_set_dimension (screen, sdlscreen->w, sdlscreen->h);
	visual_video_set_pitch (screen, sdlscreen->pitch);
	visual_video_set_buffer (screen, sdlscreen->pixels);

	return 0;
}

static int native_updaterect (SADisplay *display, VisRectangle *rect)
{
	SDLNative *native = SDL_NATIVE (display->native);
	SDL_Surface *sdlscreen = native->screen;

	if (sdlscreen->format->BitsPerPixel == 8) {
		SDL_Color colors[256];
		VisPalette *pal = display->screen->pal;

		visual_mem_set (colors, 0, sizeof (colors));

		if (pal != NULL && pal->ncolors <= 256) {
			int i;

			for (i = 0; i < pal->ncolors; i++) {
				colors[i].r = pal->colors[i].r;
				colors[i].g = pal->colors[i].g;
				colors[i].b = pal->colors[i].b;
			}

			SDL_SetColors (sdlscreen, colors, 0, 256);
		}
	}

	SDL_UpdateRect (sdlscreen, rect->x, rect->y, rect->width, rect->height);

	return 0;
}

static int native_drainevents (SADisplay *display, VisEventQueue *eventqueue)
{
	SDL_Event event;

	while (SDL_PollEvent (&event)) {

		switch (event.type) {
			case SDL_KEYUP:
				visual_event_queue_add_keyboard (eventqueue, event.key.keysym.sym, event.key.keysym.mod,
						VISUAL_KEY_UP);
				break;

			case SDL_KEYDOWN:
				visual_event_queue_add_keyboard (eventqueue, event.key.keysym.sym, event.key.keysym.mod,
						VISUAL_KEY_DOWN);
				break;

			case SDL_VIDEORESIZE:
				visual_event_queue_add_resize (eventqueue, display->screen, event.resize.w, event.resize.h);

				native_create (display, display->screen->depth, event.resize.w, event.resize.h, TRUE);
				break;

			case SDL_MOUSEMOTION:
				visual_event_queue_add_mousemotion (eventqueue, event.motion.x, event.motion.y);
				break;

			case SDL_MOUSEBUTTONDOWN:
				visual_event_queue_add_mousebutton (eventqueue, event.button.button, VISUAL_MOUSE_DOWN,
						event.button.x, event.button.y);
				break;

			case SDL_MOUSEBUTTONUP:
				visual_event_queue_add_mousebutton (eventqueue, event.button.button, VISUAL_MOUSE_UP,
						event.button.x, event.button.y);
				break;

			case SDL_QUIT:
				break;

			default:
				break;
		}
	}
}