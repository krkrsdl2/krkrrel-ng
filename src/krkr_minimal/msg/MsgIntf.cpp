//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Definition of Messages and Message Related Utilities
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsError.h"
//---------------------------------------------------------------------------
// import message strings
//---------------------------------------------------------------------------

#define TVP_MSG_DECL(name, msg) tTJSMessageHolder name(TJS_W(#name), msg);
#define TVP_MSG_DECL_CONST(name, msg) tTJSMessageHolder name(TJS_W(#name), msg, false);
#define TVP_MSG_DECL_NULL(name) tTJSMessageHolder name(TJS_W(#name), NULL, false);

#include "MsgIntf.h"

//---------------------------------------------------------------------------
// TVPFormatMessage
//---------------------------------------------------------------------------
/*
	these functions do :
	replace each %%, %1, %2 into %, p1, p2.
	%1 must appear only once in the message string, otherwise internal
	buffer will overflow. ( %2 must also so )
*/
ttstr TVPFormatMessage(const tjs_char *msg, const ttstr & p1)
{
	tjs_char *p;
	tjs_char * buf = new tjs_char[TJS_strlen(msg) + p1.GetLen() + 1];
	p = buf;
	for(;*msg;msg++,p++)
	{
		if(*msg == TJS_W('%'))
		{
			if(msg[1] == TJS_W('%'))
			{
				// %%
				*p = TJS_W('%');
				msg++;
				continue;
			}
			else if(msg[1] == TJS_W('1'))
			{
				// %1
				TJS_strcpy(p, p1.c_str());
				p += p1.GetLen();
				p--;
				msg++;
				continue;
			}
		}
		*p = *msg;
	}

	*p = 0;

	ttstr ret(buf);
	delete [] buf;
	return ret;
}
ttstr TVPFormatMessage(const tjs_char *msg, const ttstr & p1, const ttstr & p2)
{
	tjs_char *p;
	tjs_char * buf =
		new tjs_char[TJS_strlen(msg) + p1.GetLen() + p2.GetLen() + 1];
	p = buf;
	for(;*msg;msg++,p++)
	{
		if(*msg == TJS_W('%'))
		{
			if(msg[1] == TJS_W('%'))
			{
				// %%
				*p = TJS_W('%');
				msg++;
				continue;
			}
			else if(msg[1] == TJS_W('1'))
			{
				// %1
				TJS_strcpy(p, p1.c_str());
				p += p1.GetLen();
				p--;
				msg++;
				continue;
			}
			else if(msg[1] == TJS_W('2'))
			{
				// %2
				TJS_strcpy(p, p2.c_str());
				p += p2.GetLen();
				p--;
				msg++;
				continue;
			}
		}
		*p = *msg;
	}

	*p = 0;

	ttstr ret(buf);
	delete [] buf;
	return ret;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPThrowExceptionMessage
//---------------------------------------------------------------------------
void TVPThrowExceptionMessage(const tjs_char *msg)
{
	throw eTJSError(msg);
}
void TVPThrowExceptionMessage(const tjs_char *msg, const ttstr & p1, tjs_int num)
{
	throw eTJSError(TVPFormatMessage(msg, p1, ttstr(num)));
}
void TVPThrowExceptionMessage(const tjs_char *msg, const ttstr & p1)
{
	throw eTJSError(TVPFormatMessage(msg, p1));
}
void TVPThrowExceptionMessage(const tjs_char *msg, const ttstr & p1,
	const ttstr & p2)
{
	throw eTJSError(TVPFormatMessage(msg, p1, p2));
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// version retrieving
//---------------------------------------------------------------------------
tjs_int TVPVersionMajor;
tjs_int TVPVersionMinor;
tjs_int TVPVersionRelease;
tjs_int TVPVersionBuild;
//---------------------------------------------------------------------------
/*
#ifdef _WIN32
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#else
#define WIDEN2(x) u ## x
#define WIDEN(x) WIDEN2(x)
#endif
*/
const tjs_char* TVPCompileDate = WIDEN(__DATE__);
const tjs_char* TVPCompileTime = WIDEN(__TIME__);

//---------------------------------------------------------------------------
// version information related functions
//---------------------------------------------------------------------------
ttstr TVPGetAboutString(void)
{
	return ttstr();
}
//---------------------------------------------------------------------------
ttstr TVPGetLicenseString(void)
{
	return ttstr();
}
//---------------------------------------------------------------------------
ttstr TVPGetVersionInformation(void)
{
	return ttstr();
}
//---------------------------------------------------------------------------
ttstr TVPGetVersionString()
{
	return ttstr();
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// Versoin retriving
//---------------------------------------------------------------------------
void TVPGetSystemVersion(tjs_int &major, tjs_int &minor,
	tjs_int &release, tjs_int &build)
{
	major = 0;
	minor = 0;
	release = 0;
	build = 0;
}
//---------------------------------------------------------------------------
void TVPGetTJSVersion(tjs_int &major, tjs_int &minor,
	tjs_int &release)
{
	major = TJSVersionMajor;
	minor = TJSVersionMinor;
	release = TJSVersionRelease;
}
//---------------------------------------------------------------------------



