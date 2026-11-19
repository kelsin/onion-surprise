#include <SDL/SDL.h>

int main(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 0;
	SDL_ShowCursor(SDL_DISABLE);
	SDL_Surface *video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE);
	if (video) {
		SDL_FillRect(video, NULL, SDL_MapRGB(video->format, 32, 32, 48));
		SDL_Flip(video);
		SDL_Delay(1000);
	}
	SDL_Quit();
	return 0;
}
