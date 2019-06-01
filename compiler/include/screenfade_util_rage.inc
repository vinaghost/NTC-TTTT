#if defined _screenfade_util_included
	#endinput
#endif
#define _screenfade_util_included

#include <rage>

#define FFADE_IN			0x0000		// Just here so we don't pass 0 into the function
#define FFADE_OUT			0x0001		// Fade out (not in)
#define FFADE_MODULATE		0x0002		// Modulate (don't blend)
#define FFADE_STAYOUT		0x0004		// ignores the duration, stays faded out until new ScreenFade message received

stock UTIL_ScreenFade(id, Float:color[3]={0.0,0.0,0.0},Float:fadeTime=0.0,Float:fadeHold=0.0,alpha=0,flags=FFADE_IN)
{
	if( id )
	{
		if( !is_user_connected(id) ) return;
		static RageFunc:__FuncUTIL_ScreenFade;
		if( !__FuncUTIL_ScreenFade ) __FuncUTIL_ScreenFade = RageGetFunction("UTIL_ScreenFade");
			
		RageCall(__FuncUTIL_ScreenFade, id, color, fadeTime, fadeHold, alpha, flags);
	}
	else
	{
		static RageFunc:__FuncUTIL_ScreenFadeAll;
		if( !__FuncUTIL_ScreenFadeAll ) __FuncUTIL_ScreenFadeAll = RageGetFunction("UTIL_ScreenFadeAll");

		RageCall(__FuncUTIL_ScreenFadeAll, color, fadeTime, fadeHold, alpha, flags);
	}
}

stock UTIL_FadeToBlack(id,Float:fadeTime=3.0)
{
	UTIL_ScreenFade(id, _, fadeTime, fadeTime, 255, FFADE_OUT|FFADE_STAYOUT);
}

// if player offset m_flFlashedUntil (514 win32) is greater than gametime() and offset m_iFlashAlpha (518 win32) is 255, better to set bFullFlashed on true
stock UTIL_Flash(id, Float:fadeTime, Float:holdTime, bool:bFullFlashed)
{
	UTIL_ScreenFade(id, Float:{255.0,255.0,255.0}, fadeTime, holdTime, bFullFlashed ? 255 : 200);
}