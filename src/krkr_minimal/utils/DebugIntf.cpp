//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Utilities for Debugging
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#if 1
#include "CharacterSet.h"
#include <stdio.h>
#endif

#include <deque>
#include <algorithm>
#if 0
#include <time.h>
#endif
#include "DebugIntf.h"
#include "MsgIntf.h"
#include "StorageIntf.h"
#if 0
#include "SysInitIntf.h"
#include "SysInitImpl.h"
#ifdef ENABLE_DEBUGGER
#include "tjsDebug.h"
#endif // ENABLE_DEBUGGER

#include "Application.h"
#include "SystemControl.h"
#include "NativeFile.h"
//---------------------------------------------------------------------------
// global variables
//---------------------------------------------------------------------------
struct tTVPLogItem
{
	ttstr Log;
	ttstr Time;
	tTVPLogItem(const ttstr &log, const ttstr &time)
	{
		Log = log;
		Time = time;
	}
};
static std::deque<tTVPLogItem> *TVPLogDeque = NULL;
static tjs_uint TVPLogMaxLines = 2048;

bool TVPAutoLogToFileOnError = true;
bool TVPAutoClearLogOnError = false;
bool TVPLoggingToFile = false;
static tjs_uint TVPLogToFileRollBack = 100;
static ttstr *TVPImportantLogs = NULL;
static ttstr TVPLogLocation;
tjs_char TVPNativeLogLocation[MAX_PATH];
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static bool TVPLogObjectsInitialized = false;
static void TVPEnsureLogObjects()
{
	if(TVPLogObjectsInitialized) return;
	TVPLogObjectsInitialized = true;

	TVPLogDeque = new std::deque<tTVPLogItem>();
	TVPImportantLogs = new ttstr();
}
//---------------------------------------------------------------------------
static void TVPDestroyLogObjects()
{
	if(TVPLogDeque) delete TVPLogDeque, TVPLogDeque = NULL;
	if(TVPImportantLogs) delete TVPImportantLogs, TVPImportantLogs = NULL;
}
//---------------------------------------------------------------------------
tTVPAtExit TVPDestroyLogObjectsAtExit
	(TVP_ATEXIT_PRI_CLEANUP, TVPDestroyLogObjects);
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
void (*TVPOnLog)(const ttstr & line) = NULL;
	// this function is invoked when a line is logged
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPSetOnLog
//---------------------------------------------------------------------------
void TVPSetOnLog(void (*func)(const ttstr & line))
{
	TVPOnLog = func;
}
//---------------------------------------------------------------------------
static std::vector<tTJSVariantClosure> TVPLoggingHandlerVector;

static void TVPCleanupLoggingHandlerVector()
{
	// eliminate empty
	std::vector<tTJSVariantClosure>::iterator i;
	for(i = TVPLoggingHandlerVector.begin();
		i != TVPLoggingHandlerVector.end();
		i++)
	{
		if(!i->Object)
		{
			i->Release();
			i = TVPLoggingHandlerVector.erase(i);
		}
		else
		{
			i++;
		}
	}
}
static void TVPDestroyLoggingHandlerVector()
{
	TVPSetOnLog(NULL);
	std::vector<tTJSVariantClosure>::iterator i;
	for(i = TVPLoggingHandlerVector.begin();
		i != TVPLoggingHandlerVector.end();
		i++)
	{
		i->Release();
	}
	TVPLoggingHandlerVector.clear();
}

static tTVPAtExit TVPDestroyLoggingHandlerAtExit
	(TVP_ATEXIT_PRI_PREPARE, TVPDestroyLoggingHandlerVector);
//---------------------------------------------------------------------------
static bool TVPInDeliverLoggingEvent = false;
static void _TVPDeliverLoggingEvent(const ttstr & line) // internal
{
	if(!TVPInDeliverLoggingEvent)
	{
		TVPInDeliverLoggingEvent = true;
		try
		{
			if(TVPLoggingHandlerVector.size())
			{
				bool emptyflag = false;
				tTJSVariant vline(line);
				tTJSVariant *pvline[] = { &vline };
				for(tjs_uint i = 0; i < TVPLoggingHandlerVector.size(); i++)
				{
					if(TVPLoggingHandlerVector[i].Object)
					{
						tjs_error er;
						try
						{
							er =
								TVPLoggingHandlerVector[i].FuncCall(0, NULL, NULL, NULL, 1, pvline, NULL);
						}
						catch(...)
						{
							// failed
							TVPLoggingHandlerVector[i].Release();
							TVPLoggingHandlerVector[i].Object =
							TVPLoggingHandlerVector[i].ObjThis = NULL;
							throw;
						}
						if(TJS_FAILED(er))
						{
							// failed
							TVPLoggingHandlerVector[i].Release();
							TVPLoggingHandlerVector[i].Object =
							TVPLoggingHandlerVector[i].ObjThis = NULL;
							emptyflag = true;
						}
					}
					else
					{
						emptyflag = true;
					}
				}

				if(emptyflag)
				{
					// the array has empty cell
					TVPCleanupLoggingHandlerVector();
				}
			}

			if(!TVPLoggingHandlerVector.size())
			{
				TVPSetOnLog(NULL);
			}
		}
		catch(...)
		{
			TVPInDeliverLoggingEvent = false;
			throw;
		}
		TVPInDeliverLoggingEvent = false;
	}
}
//---------------------------------------------------------------------------
static void TVPAddLoggingHandler(tTJSVariantClosure clo)
{
	std::vector<tTJSVariantClosure>::iterator i;
	i = std::find(TVPLoggingHandlerVector.begin(),
		TVPLoggingHandlerVector.end(), clo);
	if(i == TVPLoggingHandlerVector.end())
	{
		clo.AddRef();
		TVPLoggingHandlerVector.push_back(clo);
		TVPSetOnLog(&_TVPDeliverLoggingEvent);
	}
}
//---------------------------------------------------------------------------
static void TVPRemoveLoggingHandler(tTJSVariantClosure clo)
{
	std::vector<tTJSVariantClosure>::iterator i;
	i = std::find(TVPLoggingHandlerVector.begin(),
		TVPLoggingHandlerVector.end(), clo);
	if(i != TVPLoggingHandlerVector.end())
	{
		i->Release();
		i->Object = i->ObjThis = NULL;
	}

	if(!TVPInDeliverLoggingEvent)
	{
		TVPCleanupLoggingHandlerVector();
		if(!TVPLoggingHandlerVector.size())
		{
			TVPSetOnLog(NULL);
		}
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// log stream holder
//---------------------------------------------------------------------------
class tTVPLogStreamHolder
{
	NativeFile Stream;
	bool Alive;
	bool OpenFailed;

public:
	tTVPLogStreamHolder() { Alive = true; OpenFailed = false; }
	~tTVPLogStreamHolder() { Stream.Close(); Alive = false; }

private:
	void Open(const tjs_char *mode);

public:
	void Clear(); // clear log stream
	void Log(const ttstr & text); // log given text

	void Reopen() { Stream.Close(); Alive = false; OpenFailed = false; } // reopen log stream

} static TVPLogStreamHolder;
//---------------------------------------------------------------------------
static const tjs_char *WDAY[] = {
		TJS_W("Sunday"), TJS_W("Monday"), TJS_W("Tuesday"), TJS_W("Wednesday"),
		TJS_W("Thursday"), TJS_W("Friday"), TJS_W("Saturday")
};
//---------------------------------------------------------------------------
static const tjs_char *MDAY[] = {
		TJS_W("January"), TJS_W("February"), TJS_W("March"), TJS_W("April"), TJS_W("May"), TJS_W("June"),
		TJS_W("July"), TJS_W("August"), TJS_W("September"), TJS_W("October"), TJS_W("November"),
		TJS_W("December")
};
//---------------------------------------------------------------------------
void tTVPLogStreamHolder::Open(const tjs_char *mode)
{
	if(OpenFailed) return; // no more try

	try
	{
		tjs_char filename[MAX_PATH];
		if(TVPLogLocation.GetLen() == 0)
		{
			Stream.Close();
			OpenFailed = true;
		}
		else
		{
			// no log location specified
			TJS_strcpy(filename, TVPNativeLogLocation);
			TJS_strcat(filename, TJS_W("/krkr.console.log"));
			TVPEnsureDataPathDirectory();
			Stream.Open( filename, mode );
			if(!Stream.IsOpen()) OpenFailed = true;
		}

		if(Stream.IsOpen())
		{
			Stream.Seek(0, SEEK_END);
			if(Stream.Tell() == 0)
			{
				// write BOM
				Stream.Write( TJS_N("\xff\xfe"), 2 ); // indicate unicode text
			}

#ifdef TJS_TEXT_OUT_CRLF
			ttstr separator( TVPSeparatorCRLF );
#else
			ttstr separator( TVPSeparatorCR );
#endif
			Log(separator);


			static tjs_char timebuf[80];
			{
				time_t timer;
				timer = time(&timer);
				tm* t = localtime(&timer);
				TJS_snprintf( timebuf, 79, TJS_W("%s, %s %02d, %04d %02d:%02d:%02d"), WDAY[t->tm_wday], MDAY[t->tm_mon], t->tm_mday, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec );
			}

			Log(ttstr(TJS_W("Logging to ")) + ttstr(filename) + TJS_W(" started on ") + timebuf);

		}
	}
	catch(...)
	{
		OpenFailed = true;
	}
}
//---------------------------------------------------------------------------
void tTVPLogStreamHolder::Clear()
{
	// clear log text
	Stream.Close();

	Open(TJS_W("wb"));
}
//---------------------------------------------------------------------------
void tTVPLogStreamHolder::Log(const ttstr & text)
{
	if(!Stream.IsOpen()) Open(TJS_W("ab"));

	try
	{
		if(Stream.IsOpen())
		{
			size_t len = text.GetLen() * sizeof(tjs_char);
			if( len != Stream.Write( text.c_str(), len) )
			{
				// cannot write
				Stream.Close();
				OpenFailed = true;
				return;
			}
#ifdef TJS_TEXT_OUT_CRLF
			Stream.Write(TJS_W("\r\n"), 2 * sizeof(tjs_char) );
#else
			Stream.Write(TJS_W("\n"), 1 * sizeof(tjs_char) );
#endif

			// flush
			Stream.Flush();
		}
	}
	catch(...)
	{
		try
		{
			Stream.Close();
		}
		catch(...)
		{
		}

		OpenFailed = true;
	}
}
//---------------------------------------------------------------------------
#endif







//---------------------------------------------------------------------------
// TVPAddLog
//---------------------------------------------------------------------------
void TVPAddLog(const ttstr &line, bool appendtoimportant)
{
	// add a line to the log.
	// exceeded lines over TVPLogMaxLines are eliminated.
	// this function is not thread-safe ...

#if 0
	TVPEnsureLogObjects();
	if(!TVPLogDeque) return; // log system is shuttingdown
	if(!TVPImportantLogs) return; // log system is shuttingdown

	static time_t prevlogtime = 0;
	static ttstr prevtimebuf;
	static tjs_char timebuf[40];

	tm *struct_tm;
	time_t timer;
	timer = time(&timer);

	if(prevlogtime != timer)
	{
		struct_tm = localtime(&timer);
		TJS_snprintf( timebuf, 39, TJS_W("%02d:%02d:%02d"), struct_tm->tm_hour, struct_tm->tm_min, struct_tm->tm_sec );
		prevlogtime = timer;
		prevtimebuf = timebuf;
	}

	TVPLogDeque->push_back(tTVPLogItem(line, prevtimebuf));

	if(appendtoimportant)
	{
#ifdef TJS_TEXT_OUT_CRLF
		*TVPImportantLogs += ttstr(timebuf) + TJS_W(" ! ") + line + TJS_W("\r\n");
#else
		*TVPImportantLogs += ttstr(timebuf) + TJS_W(" ! ") + line + TJS_W("\n");
#endif
	}
	while(TVPLogDeque->size() >= TVPLogMaxLines+100)
	{
		std::deque<tTVPLogItem>::iterator i = TVPLogDeque->begin();
		TVPLogDeque->erase(i, i+100);
	}

	tjs_int timebuflen = (tjs_int)TJS_strlen(timebuf);
	ttstr buf((tTJSStringBufferLength)(timebuflen + 1 + line.GetLen()));
	tjs_char * p = buf.Independ();
	TJS_strcpy(p, timebuf);
	p += timebuflen;
	*p = TJS_W(' ');
	p++;
	TJS_strcpy(p, line.c_str());
	if(TVPOnLog) TVPOnLog(buf);
#ifdef ENABLE_DEBUGGER
	if( TJSEnableDebugMode ) TJSDebuggerLog(buf,appendtoimportant);
	//OutputDebugStringW( buf.c_str() );
	//OutputDebugStringW( TJS_W("\n") );
#endif	// ENABLE_DEBUGGER

#ifdef TVP_LOG_TO_COMMANDLINE_CONSOLE
	Application->PrintConsole( buf.c_str(), buf.length(), appendtoimportant );
#endif

	if(TVPLoggingToFile) TVPLogStreamHolder.Log(buf);
#endif
#ifdef _WIN32
	HANDLE hStdOutput = ::GetStdHandle(STD_ERROR_HANDLE);
	if (hStdOutput > 0)
	{
		DWORD mode;
		if (GetConsoleMode(hStdOutput, &mode))
		{
			// 実コンソール
			DWORD wlen;
			::WriteConsoleW( hStdOutput, line.c_str(), line.length(), &wlen, NULL );
			::WriteConsoleW( hStdOutput, TJS_W("\n"), 1, &wlen, NULL );
		}
	}
#else
	std::string nline;
	TVPUtf16ToUtf8(nline, line.AsStdString());
	fprintf(stderr, "%s\n", nline.c_str());
#endif
}
//---------------------------------------------------------------------------
void TVPAddLog(const ttstr &line)
{
	TVPAddLog(line, false);
}
#if 0
//---------------------------------------------------------------------------
void TVPAddImportantLog(const ttstr &line)
{
	TVPAddLog(line, true);
}
//---------------------------------------------------------------------------
ttstr TVPGetImportantLog()
{
	if(!TVPImportantLogs) return ttstr();
	return *TVPImportantLogs;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPGetLastLog : get last n lines of the log ( each line is spearated with '\n'/'\r\n' )
//---------------------------------------------------------------------------
ttstr TVPGetLastLog(tjs_uint n)
{
	TVPEnsureLogObjects();
	if(!TVPLogDeque) return TJS_W(""); // log system is shuttingdown

	tjs_uint len = 0;
	tjs_uint size = (tjs_uint)TVPLogDeque->size();
	if(n > size) n = size;
	if(n==0) return ttstr();
	std::deque<tTVPLogItem>::iterator i = TVPLogDeque->end();
	i-=n;
	tjs_uint c;
	for(c = 0; c<n; c++, i++)
	{
#ifdef TJS_TEXT_OUT_CRLF
		len += i->Time.GetLen() + 1 +  i->Log.GetLen() + 2;
#else
		len += i->Time.GetLen() + 1 +  i->Log.GetLen() + 1;
#endif
	}

	ttstr buf((tTJSStringBufferLength)len);
	tjs_char *p = buf.Independ();

	i = TVPLogDeque->end();
	i -= n;
	for(c = 0; c<n; c++)
	{
		TJS_strcpy(p, i->Time.c_str());
		p += i->Time.GetLen();
		*p = TJS_W(' ');
		p++;
		TJS_strcpy(p, i->Log.c_str());
		p += i->Log.GetLen();
#ifdef TJS_TEXT_OUT_CRLF
		*p = TJS_W('\r');
		p++;
		*p = TJS_W('\n');
		p++;
#else
		*p = TJS_W('\n');
		p++;
#endif
		i++;
	}
	return buf;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPStartLogToFile
//---------------------------------------------------------------------------
void TVPStartLogToFile(bool clear)
{
	TVPEnsureLogObjects();
	if(!TVPImportantLogs) return; // log system is shuttingdown

	if(TVPLoggingToFile) return; // already logging
	if(clear) TVPLogStreamHolder.Clear();

	// log last lines

	TVPLogStreamHolder.Log(*TVPImportantLogs);

#ifdef TJS_TEXT_OUT_CRLF
	ttstr separator(TJS_W("\r\n")
TJS_W("------------------------------------------------------------------------------\r\n"
				));
#else
	ttstr separator(TJS_W("\n")
TJS_W("------------------------------------------------------------------------------\n"
				));
#endif

	TVPLogStreamHolder.Log(separator);

	ttstr content = TVPGetLastLog(TVPLogToFileRollBack);
	TVPLogStreamHolder.Log(content);

	//
	TVPLoggingToFile = true;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPOnError
//---------------------------------------------------------------------------
void TVPOnError()
{
	if(TVPAutoLogToFileOnError) TVPStartLogToFile(TVPAutoClearLogOnError);
	// TVPOnErrorHook();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPSetLogLocation
//---------------------------------------------------------------------------
void TVPSetLogLocation(const ttstr &loc)
{
	TVPLogLocation = TVPNormalizeStorageName(loc);

	ttstr native = TVPGetLocallyAccessibleName(TVPLogLocation);
	if(native.IsEmpty())
	{
		TVPNativeLogLocation[0] = 0;
		TVPLogLocation.Clear();
	}
	else
	{
		TJS_strcpy(TVPNativeLogLocation, native.AsStdString().c_str());
		if(TVPNativeLogLocation[TJS_strlen(TVPNativeLogLocation)-1] != TJS_W('\\'))
			TJS_strcat(TVPNativeLogLocation, TJS_W("\\"));
	}

	TVPLogStreamHolder.Reopen();

	// check force logging option
	tTJSVariant val;
	if(TVPGetCommandLine(TJS_W("-forcelog"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes"))
		{
			TVPLoggingToFile = false;
			TVPStartLogToFile(false);
		}
		else if(str == TJS_W("clear"))
		{
			TVPLoggingToFile = false;
			TVPStartLogToFile(true);
		}
	}
	if(TVPGetCommandLine(TJS_W("-logerror"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("no"))
		{
			TVPAutoClearLogOnError = false;
			TVPAutoLogToFileOnError = false;
		}
		else if(str == TJS_W("clear"))
		{
			TVPAutoClearLogOnError = true;
			TVPAutoLogToFileOnError = true;
		}
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Debug
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Debug::ClassID = -1;
tTJSNC_Debug::tTJSNC_Debug() : tTJSNativeClass(TJS_W("Debug"))
{
	TJS_BEGIN_NATIVE_MEMBERS(Debug)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Debug)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Debug)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/message)
{
	if(numparams<1) return TJS_E_BADPARAMCOUNT;

	if(numparams == 1)
	{
		TVPAddLog(*param[0]);
	}
	else
	{
		// display the arguments separated with ", "
		ttstr args;
		for(int i = 0; i<numparams; i++)
		{
			if(i != 0) args += TJS_W(", ");
			args += ttstr(*param[i]);
		}
		TVPAddLog(args);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/message)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/notice)
{
	if(numparams<1) return TJS_E_BADPARAMCOUNT;

	if(numparams == 1)
	{
		TVPAddImportantLog(*param[0]);
	}
	else
	{
		// display the arguments separated with ", "
		ttstr args;
		for(int i = 0; i<numparams; i++)
		{
			if(i != 0) args += TJS_W(", ");
			args += ttstr(*param[i]);
		}
		TVPAddImportantLog(args);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/notice)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/startLogToFile)
{
	bool clear = false;

	if(numparams >= 1)
		clear = param[0]->operator bool();

	TVPStartLogToFile(clear);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/startLogToFile)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/logAsError)
{
	TVPOnError();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/logAsError)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/addLoggingHandler)
{
	// add function to logging handler list

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();

	TVPAddLoggingHandler(clo);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/addLoggingHandler)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/removeLoggingHandler)
{
	// remove function from logging handler list

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();

	TVPRemoveLoggingHandler(clo);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/removeLoggingHandler)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getLastLog)
{
	tjs_uint lines = TVPLogMaxLines + 100;

	if(numparams >= 1) lines = (tjs_uint)param[0]->AsInteger();

	if(result) *result = TVPGetLastLog(lines);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/getLastLog)
//---------------------------------------------------------------------------

//-- properies

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(logLocation)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPLogLocation;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPSetLogLocation(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(logLocation)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(logToFileOnError)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPAutoLogToFileOnError;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPAutoLogToFileOnError = param->operator bool();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(logToFileOnError)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(clearLogFileOnError)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPAutoClearLogOnError;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPAutoClearLogOnError = param->operator bool();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(clearLogFileOnError)
//----------------------------------------------------------------------

//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS

	// put version information to DMS
	TVPAddImportantLog(TVPGetVersionInformation());
	TVPAddImportantLog(ttstr(TVPVersionInformation2));
} // end of tTJSNC_Debug::tTJSNC_Debug
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Debug::CreateNativeInstance()
{
	return NULL;
}
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Debug()
{
	tTJSNativeClass *cls = new tTJSNC_Debug();
	return cls;
}
//---------------------------------------------------------------------------
#endif




//---------------------------------------------------------------------------
// TJS2 Console Output Gateway
//---------------------------------------------------------------------------
class tTVPTJS2ConsoleOutputGateway : public iTJSConsoleOutput
{
	void ExceptionPrint(const tjs_char *msg)
	{
		TVPAddLog(msg);
	}

	void Print(const tjs_char *msg)
	{
		TVPAddLog(msg);
	}
} static TVPTJS2ConsoleOutputGateway;
//---------------------------------------------------------------------------



#if 0
//---------------------------------------------------------------------------
// TJS2 Dump Output Gateway
//---------------------------------------------------------------------------
static ttstr TVPDumpOutFileName;
static NativeFile TVPDumpOutFile; // use traditional output routine
//---------------------------------------------------------------------------
class tTVPTJS2DumpOutputGateway : public iTJSConsoleOutput
{
	void ExceptionPrint(const tjs_char *msg) { Print(msg); }

	void Print(const tjs_char *msg)
	{
		if(TVPDumpOutFile.IsOpen())
		{
			TVPDumpOutFile.Write( msg, TJS_strlen(msg)*sizeof(tjs_char) );
#ifdef TJS_TEXT_OUT_CRLF
			TVPDumpOutFile.Write( TJS_W("\r\n"), 2 * sizeof(tjs_char) );
#else
			TVPDumpOutFile.Write( TJS_W("\n"), 1 * sizeof(tjs_char) );
#endif
		}
	}
} static TVPTJS2DumpOutputGateway;
//---------------------------------------------------------------------------
void TVPTJS2StartDump()
{
#if 0
	tjs_char filename[MAX_PATH];
#ifndef ANDROID
	TJS_strcpy(filename, ExePath().c_str());
#else
	TJS_strcpy(filename, Application->GetInternalDataPath().c_str());
#endif
	TJS_strcat(filename, TJS_W(".dump.txt"));
	TVPDumpOutFileName = filename;
	TVPDumpOutFile.Open(filename, TJS_W("wb+"));
	if(TVPDumpOutFile.IsOpen())
	{
		// TODO: 32-bit unicode support
		TVPDumpOutFile.Write( TJS_N("\xff\xfe"), 2 ); // indicate unicode text
	}
#endif
}
//---------------------------------------------------------------------------
void TVPTJS2EndDump()
{
#if 0
	if(TVPDumpOutFile.IsOpen())
	{
		TVPDumpOutFile.Close();
		TVPAddLog(ttstr(TJS_W("Dumped to ")) + TVPDumpOutFileName);
	}
#endif
}
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// console interface retrieving functions
//---------------------------------------------------------------------------
iTJSConsoleOutput *TVPGetTJS2ConsoleOutputGateway()
{
	return & TVPTJS2ConsoleOutputGateway;
}
#if 0
//---------------------------------------------------------------------------
iTJSConsoleOutput *TVPGetTJS2DumpOutputGateway()
{
	return & TVPTJS2DumpOutputGateway;
}
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
// on-error hook
//---------------------------------------------------------------------------
void TVPOnErrorHook()
{
	if(TVPMainForm) TVPMainForm->NotifySystemError();
}
//---------------------------------------------------------------------------
*/
#endif
