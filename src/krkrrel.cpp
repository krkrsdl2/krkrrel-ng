#include "tjsCommHead.h"
#include "TextStream.h"
#include "UtilStreams.h"
#include "StorageImpl.h"
#include "CharacterSet.h"
#include "DebugIntf.h"
#include <algorithm>
#include <zlib.h>
#ifndef _WIN32
#include <dirent.h>
#endif

extern void TVPLoadMessage();

struct TKrkrRelConfig
{
	bool CompressIndex;
	bool CompressLimit;
	bool Executable;
	bool OVBookShare;
	bool Protect;
	bool UseXP3EncDLL;
	bool WriteDefaultRPF;
	uint64_t CompressLimitSize;
	tjs_string OutputFileName;
	tjs_string ProjFolder;
	std::vector<tjs_string> DiscardExt;
	std::vector<tjs_string> CompressExt;
	std::vector<tjs_string> FileList;
};


struct TFileSegment
{
	uint32_t Flags;
	uint64_t Offset;
	uint64_t StoreSize;
	uint64_t OrgSize;
};

struct TFileInfo
{
	uint32_t Flags;
	uint64_t FileSize;
	uint64_t StoreSize;
	uint32_t Adler32; // adler32 running checksum
	std::vector<TFileSegment> Segments;
	tjs_string LocalFileName;
	tjs_string FileName;
};

static tjs_string NoExtName = TJS_W("(No extension)");
static tjs_string ProtectWarnFileName1 =
	TJS_W("$$$ This is a protected archive. ")
	TJS_W("$$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 ");
static tjs_string ProtectWarnFileName2 =
	TJS_W("$$$ Warning! Extracting this archive may infringe on author's rights. ")
	TJS_W("警告 このアーカイブを展開することにより、あなたは著作者の権利を侵害するおそれがあります。")
	TJS_W(".txt");

#define OV_MATCH_BUF_LEN 32768
#define OV_MATCH_MIN 128
#define OV_MATCH_DIF_MAX 256
#define OV_MATCH_HOLD 16

static bool DetectMatch(uint8_t *ref, int reflen, uint8_t *buf, int buflen,
	std::vector<int> &refstart, std::vector<int> &bufstart, std::vector<int> &len)
{
	if(reflen <= OV_MATCH_MIN) return false;
	if(buflen <= OV_MATCH_MIN) return false;

	bool anymatched = false;
	int bufpos;
	int refpos;
	for(bufpos = 0; bufpos<buflen-OV_MATCH_MIN;)
	{
		bool matched = false;
		int matchlen;
		int refmax = bufpos + OV_MATCH_DIF_MAX;
		if(refmax >= reflen - OV_MATCH_MIN) refmax =  reflen - OV_MATCH_MIN;
		for(refpos = (bufpos<OV_MATCH_DIF_MAX?0:(bufpos-OV_MATCH_DIF_MAX));
			refpos<refmax; refpos++)
		{
			if(buf[bufpos] == ref[refpos])
			{
				// first character matched
				int remain =
					buflen-bufpos > reflen-refpos ? reflen-refpos :  buflen-bufpos;
				uint8_t *rp = ref + refpos;
				uint8_t *bp = buf + bufpos;
				while(*rp == *bp && remain) rp++, bp++, remain--;
				matchlen = rp - (ref + refpos);

				if(matchlen >= OV_MATCH_MIN)
				{
					// matched
					refstart.push_back(refpos);
					bufstart.push_back(bufpos);
					len.push_back(matchlen);
					matched = true;
					break;
				}
			}
		}

		if(matched)
			bufpos += matchlen, anymatched = true;
		else
			bufpos++;
	}

	return anymatched;
}

struct TOVCodeBookInfo
{
	uint64_t Offset[OV_MATCH_HOLD]; // offset in archive file
	int HeadersLen[OV_MATCH_HOLD]; // byte length in Headers
	uint8_t Headers[OV_MATCH_HOLD][OV_MATCH_BUF_LEN];
		// first OV_MATCH_BUF_LEN of the ogg vorbis flie
};
//---------------------------------------------------------------------------
static void Copy(tTJSBinaryStream *dest, tTJSBinaryStream *src,
	uint64_t length)
{
	// copy stream

	if(length == 0) return;

	uint64_t remain = length;
	uint8_t *buf = new uint8_t[1024*1024];
	try
	{
		while(remain)
		{
			int onesize =
				remain > 1*1024*1024 ? 1*1024*1024 : (int)remain;
			src->ReadBuffer(buf, onesize);
			dest->WriteBuffer(buf, onesize);
			remain -= onesize;
		}
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}
	delete [] buf;
}
//---------------------------------------------------------------------------
static uint32_t GetFileCheckSum(tTJSBinaryStream * src)
{
	// compute file checksum via adler32.
	// src's file pointer is to be reset to zero.
	uint32_t adler32sum = adler32(0L, Z_NULL, 0);

	uint8_t *buf = new uint8_t[1024*1024];
	try
	{
		while(true)
		{
			int onesize = src->Read(buf, 1024*1024);
			if(onesize == 0) break;
			adler32sum = adler32(adler32sum, buf, onesize);
		}
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}
	delete [] buf;

	src->SetPosition(0);

	return adler32sum;
}
//---------------------------------------------------------------------------
static bool CompareFile(tjs_string file1, tjs_string file2)
{
	// compare two files;
	// returns true if all matched.
	tTVPStreamHolder f1(file1, TJS_BS_READ);
	tTVPStreamHolder f2(file2, TJS_BS_READ);

	uint8_t *buf1 = NULL;
	uint8_t *buf2 = NULL;
	bool matched = true;
	try
	{
		buf1 = new uint8_t[1024*1024];
		buf2 = new uint8_t[1024*1024];
		while(true)
		{
			int onesize1 = f1->Read(buf1, 1024*1024);
			int onesize2 = f2->Read(buf2, 1024*1024);
			if(onesize1 != onesize2)
			{
				matched = false;
				break;
			}
			if(onesize1 == 0) break;
			if(memcmp(buf1, buf2, onesize1))
			{
				matched = false;
				break;
			}
		}
	}
	catch(...)
	{
		if(buf1) delete [] buf1;
		if(buf2) delete [] buf2;
		throw;
	}
	if(buf1) delete [] buf1;
	if(buf2) delete [] buf2;

	return matched;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void CreateArchive(TKrkrRelConfig *config)
{
#if 0
	// setup input filter
	// XXX: we don't support encryption yet
	if(XP3EncDLLAvailable && config->UseXP3EncDLL)
		XP3ArchiveAttractFilter_v2 = XP3ArchiveAttractFilter_v2_org;
	else
		XP3ArchiveAttractFilter_v2 = NULL;
#endif

	// information vector
	std::vector<TFileInfo> fileinfo;

	// ogg vorbis code books
	int ovcb_index = 0;
	std::unique_ptr<TOVCodeBookInfo> ovcb(new TOVCodeBookInfo);
	for(int i = 0; i< OV_MATCH_HOLD; i++)
	{
		ovcb->HeadersLen[i] = 0;
	}

	// copy executable file or write a stub
#if 0
	// XXX: we don't support executable prepending right now
	if(config->Executable)
	{
		ConfMainFrame->SetSourceAndTargetFileName(GetKrKrFileName(), config->OutputFileName);
		ConfMainFrame->CopyExe();
	}
	else
#endif
	{
		// just touch
		tTVPStreamHolder stream(config->OutputFileName, TJS_BS_WRITE);
	}

	// open target file
	tTVPStreamHolder stream(config->OutputFileName, TJS_BS_UPDATE);


	uint64_t offset = stream->GetPosition();  // real xp3 header start

	uint64_t compsizelimit = (!config->CompressLimit) ?
		(int64_t)-1 : config->CompressLimitSize * (uint64_t )1024;

	// count total files
	int totalcount = 0;
	for (size_t i = 0; i < config->FileList.size(); i += 1)
	{
		ttstr ext = TVPExtractStorageExt(config->FileList.at(i));
		ext.ToLowerCase();
		if(ext.length() == 0) ext = NoExtName;
		if(std::find(config->DiscardExt.begin(), config->DiscardExt.end(), ext.AsStdString()) != config->DiscardExt.end())
			continue;
		totalcount ++;
	}

	// write a header
	stream->WriteBuffer("XP3\r\n \n\x1a ", 8);
	stream->WriteBuffer("\x8b\x67\x01", 3);
	int64_t offset_to_cushion_header = 11+4+8;
	stream->WriteBuffer(&offset_to_cushion_header, 8); // to cushion header
	stream->WriteBuffer("\x01\x00\x00\x00", 4); // header minor version

	// cushion header
	stream->WriteBuffer("\x80", 1); // continue
	stream->WriteBuffer("\0\0\0\0\0\0\0\0", 8); // index size = 0
	uint64_t index_pointer_pos = stream->GetPosition();
	stream->WriteBuffer("        ", 8); // to real header

	// write protection warning
	if(config->Protect)
	{
		static uint8_t dummy_png [] =
		"\x89\x50\x4e\x47\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00"
		"\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53\xde"
		"\x00\x00\x00\xa5\x74\x45\x58\x74\x57\x61\x72\x6e\x69\x6e\x67\x00"
		"\x57\x61\x72\x6e\x69\x6e\x67\x3a\x20\x45\x78\x74\x72\x61\x63\x74"
		"\x69\x6e\x67\x20\x74\x68\x69\x73\x20\x61\x72\x63\x68\x69\x76\x65"
		"\x20\x6d\x61\x79\x20\x69\x6e\x66\x72\x69\x6e\x67\x65\x20\x6f\x6e"
		"\x20\x61\x75\x74\x68\x6f\x72\x27\x73\x20\x72\x69\x67\x68\x74\x73"
		"\x2e\x20\x8c\x78\x8d\x90\x20\x3a\x20\x82\xb1\x82\xcc\x83\x41\x81"
		"\x5b\x83\x4a\x83\x43\x83\x75\x82\xf0\x93\x57\x8a\x4a\x82\xb7\x82"
		"\xe9\x82\xb1\x82\xc6\x82\xc9\x82\xe6\x82\xe8\x81\x41\x82\xa0\x82"
		"\xc8\x82\xbd\x82\xcd\x92\x98\x8d\xec\x8e\xd2\x82\xcc\x8c\xa0\x97"
		"\x98\x82\xf0\x90\x4e\x8a\x51\x82\xb7\x82\xe9\x82\xa8\x82\xbb\x82"
		"\xea\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb7\x81\x42\x4b\x49\x44"
		"\x27\x00\x00\x00\x0c\x49\x44\x41\x54\x78\x9c\x63\xf8\xff\xff\x3f"
		"\x00\x05\xfe\x02\xfe\x0d\xef\x46\xb8\x00\x00\x00\x00\x49\x45\x4e"
		"\x44\xae\x42\x60\x82\x89\x50\x4e\x47\x0a\x1a\x0a\x00\x00\x00\x0d"
		"\x49\x48\x44\x52\x00\x00\x01\xef\x00\x00\x00\x13\x01\x03\x00\x00"
		"\x00\x83\x60\x17\x58\x00\x00\x00\x06\x50\x4c\x54\x45\x00\x00\x00"
		"\xff\xff\xff\xa5\xd9\x9f\xdd\x00\x00\x02\x4f\x49\x44\x41\x54\x78"
		"\xda\xd5\xd3\x31\x6b\xdb\x40\x14\x07\x70\x1d\x0a\x55\xb3\x44\x6d"
		"\xb2\xc4\x20\xac\x14\x9b\x78\x15\xf1\x12\x83\xf1\x0d\x1d\x4a\x21"
		"\x44\x1f\xa2\xa1\x5a\xd3\x78\x49\xc0\x44\x86\x14\xb2\x04\xec\xc4"
		"\x53\x40\xf8\xbe\x8a\x4c\x42\x6a\x83\xf0\x7d\x05\xb9\x15\xba\x55"
		"\xe8\x2d\x3e\x10\x7a\x3d\x25\xf9\x06\x19\x4a\x6f\x38\x74\x9c\x7e"
		"\xf7\x7f\x8f\xe3\x34\xc4\x37\x8c\x52\x7b\x8b\xfe\xe7\x3c\xe3\x8b"
		"\xd7\xef\x02\x45\x06\x99\xae\x99\x02\x11\x10\x39\xa2\x2c\x7d\x2e"
		"\x68\x3b\xf7\x53\x1f\x27\x65\x17\xd6\xba\x44\x51\xed\x31\x79\xcd"
		"\xd4\xff\xbc\xd4\x62\xa2\x78\x3c\xb0\x48\xb5\xcc\x00\xc0\xe1\x82"
		"\xc4\x7d\x89\xbc\xf1\xc2\x0f\xb5\x33\x3f\xbd\x34\xc7\x4e\x02\x12"
		"\xd4\xd9\x04\x0a\xe3\x56\xf1\xdb\x67\x9e\x6d\x0e\x6d\xc4\xb5\x2b"
		"\x15\x5f\x92\xe1\xa9\xae\xbd\x27\x80\x00\x06\xdf\xc4\x70\xd7\x20"
		"\x73\xb3\x9d\xea\x5a\xb1\xbf\x51\x24\xc3\x33\xbd\x33\x27\x10\xd2"
		"\xe5\xc6\xfa\x88\xfc\x0c\x1d\x5d\xf1\x46\x47\x15\x9e\x7b\xf7\x18"
		"\x9b\x4c\x8a\xdc\x55\xe9\xaa\xc0\x1d\x9c\xd5\x54\x7a\x9f\x73\x1a"
		"\x25\x2c\x91\xed\xe1\x87\xa6\x00\x45\x85\x00\xc6\x6e\x56\x26\xbb"
		"\x79\x4e\x2f\xbf\xaa\x3a\x15\x9f\xb0\x82\xdb\x72\x55\xf5\x6e\xcc"
		"\x70\xdd\x47\xde\xc1\xac\x77\x11\xba\x5d\x32\x9b\x6a\xb5\xf6\x66"
		"\x37\x59\xc5\x44\xa2\x31\x03\x4e\x03\xd9\xd2\x83\xb0\x6e\x56\xbd"
		"\x6b\x26\x62\xea\x4d\xc2\x6e\x64\xcb\xc7\x03\x97\x2e\xbd\x00\x25"
		"\x54\x3c\xb6\x04\xe3\xf4\x41\x5c\x09\x68\x6f\x9f\x9f\x3c\x3a\x52"
		"\xdb\xf2\x82\xec\x50\xb7\xe4\x3e\x09\x8a\x82\xbf\x5e\x9c\x48\xbd"
		"\x6b\x2c\x16\x0c\xe6\x19\xd9\x3b\xf6\x7a\x1e\xbc\xf0\x23\xc5\x0f"
		"\x35\x8f\x0d\xfb\x07\xda\x6e\x73\x9e\xeb\x58\x7a\xbd\x6f\x8c\x5a"
		"\xb0\xbf\xf1\xc2\xd7\x46\x48\x91\xa7\x1e\x43\xc9\x19\x64\xf9\x8e"
		"\xc3\x8d\x27\x57\x40\xcf\xec\xe0\x3c\xba\x9e\x44\xbd\x8e\x98\x6e"
		"\xf5\x0f\xb6\x4f\x93\x0c\x5a\x21\x35\x9e\x3e\x4f\x2f\x2d\x68\xde"
		"\x04\x71\x69\xd6\x55\x3a\xa7\x9c\x27\x82\x56\x5c\x24\xf9\x97\x3d"
		"\x57\xdc\xb9\x22\x1f\x3c\x48\x16\x2d\x1a\xad\xc5\x2e\x11\x57\xe1"
		"\x59\x7f\x6c\x15\x09\x8c\x38\x15\x77\x15\x6f\x77\xa3\x22\xa2\xcb"
		"\x63\x95\xce\x55\xba\xa3\x26\xe0\x8c\xab\x2e\x1c\xce\xc7\xbc\x95"
		"\x0f\x16\xc0\x17\xf7\x9f\x5a\x2b\xb3\x23\xd8\xf2\xdc\xbb\x1b\x14"
		"\x02\x5a\x2a\x6a\xfc\x30\xf5\x83\x66\xf7\x5e\x46\x6c\x7a\xac\x49"
		"\xa1\x8a\x8f\x2f\xea\x3e\xa6\x36\xb3\xb3\xe6\xc7\xe6\xc8\x9e\xc5"
		"\xa7\xb5\x77\xf6\x2f\xf1\x9b\x8e\xb2\x13\x9f\x08\x16\x0e\x46\x63"
		"\x6b\x9d\x39\x3f\x42\x6a\xcf\x12\x6a\x4c\xbf\x5f\x36\xfe\xac\x4a"
		"\x5a\x57\xe9\xff\xf1\x8b\x7b\x1b\xff\x0b\x28\x8d\x8d\xf8\xb3\xe9"
		"\xa1\xdf\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82";
		const int dummy_png_text_start = 0x30;
		const int dummy_png_text_length = 157;

		uint64_t pwarnstart = stream->GetPosition() - offset;
		int size = dummy_png_text_length;

		stream->WriteBuffer( // dummy PNG header
			dummy_png, sizeof(dummy_png));
		pwarnstart += dummy_png_text_start;

		TFileInfo info;
		info.FileName =
			ProtectWarnFileName1 +
			ProtectWarnFileName1 +
			ProtectWarnFileName1 +
			ProtectWarnFileName2;
		info.LocalFileName = TJS_W("");
		info.Flags = 0;
		info.FileSize = size;
		info.StoreSize = size;

		uint32_t adler32sum = adler32(0L, Z_NULL, 0);
		adler32sum = adler32(adler32sum, dummy_png + dummy_png_text_start, size);

		info.Adler32 = adler32sum;

		TFileSegment seg;
		seg.Offset = pwarnstart;
		seg.StoreSize = size;
		seg.OrgSize = size;
		seg.Flags = 0;
		info.Segments.push_back(seg);

		fileinfo.push_back(info);
	}

	// write each flie
	int currentno = 0;
	for (size_t i = 0; i < config->FileList.size(); i += 1)
	{
		ttstr ext = TVPExtractStorageExt(config->FileList.at(i));
		ext.ToLowerCase();
		if(ext.length() == 0) ext = NoExtName;
		if(std::find(config->DiscardExt.begin(), config->DiscardExt.end(), ext.AsStdString()) != config->DiscardExt.end()) continue;

		tjs_string storefilename = config->FileList.at(i);
		tjs_string filename = config->ProjFolder + storefilename;

		TVPAddLog(ttstr(config->FileList.at(i)) + ttstr(TJS_W(" (")) +
			ttstr((tjs_int)currentno + 1) + ttstr(TJS_W("/")) + ttstr((tjs_int)totalcount) + ttstr(TJS_W(")")));
//		Sleep(500);

#if 0
		Application->ProcessMessages();
		if(Aborted) throw Exception(InterruptedNameLabel->Caption);
#endif

		// open input flie
		uint64_t filesize;
		uint32_t adler32sum;
		bool avoidstore = false;

		{
			tTVPStreamHolder infile(filename, TJS_BS_READ);

			{
				filesize = infile->GetSize();
			}


			// check checksum
			adler32sum = GetFileCheckSum(infile.Get());
			for(std::vector<TFileInfo>::iterator i = fileinfo.begin();
				i != fileinfo.end(); i++)
			{
				if(i->Adler32 == adler32sum && i->FileSize == filesize &&
					i->LocalFileName != TJS_W(""))
				{
					// two files may be identical
					if(CompareFile(filename, i->LocalFileName))
					{
						// files are identical
						avoidstore = true;
						TFileInfo info;
						info = *i; // copy from reference file info
						info.LocalFileName = filename;
						info.FileName = storefilename;
						fileinfo.push_back(info);
						break;
					}
				}
			}
		}

		// reopen input file
		tTVPStreamHolder infile(filename, TJS_BS_READ);
#if 0
		// XXX: we don't support encryption yet
		tTVPLocalFileStream infile(filename.c_str(), TVP_ST_READ, true, adler32sum);
#endif

		if(!avoidstore)
		{
			// compress
			uint8_t *outbuf;
			uint64_t storesize;

			bool isov = config->OVBookShare && ext == TJS_W(".ogg"); // ogg vorbis

			if(std::find(config->CompressExt.begin(), config->CompressExt.end(), ext.AsStdString()) == config->CompressExt.end() ||
				(compsizelimit != (uint64_t)-1 && filesize >= compsizelimit) ||
					filesize >= ((uint64_t)2*1024*1024*1024-1) || isov)
			{
				// avoid compress
				outbuf = NULL;
				storesize = filesize;
			}
			else
			{
				uint8_t *src = new uint8_t[(int64_t)filesize];
				try
				{
					uLongf compsize = 0;
					outbuf = new uint8_t[compsize =
						(uLongf)(filesize + filesize /100 + 1024)];
					infile->Read(src, (unsigned int )filesize);
					infile->Seek(0, TJS_BS_SEEK_SET);

					int res;
					int64_t srclen = (int64_t) filesize;
					try
					{
						// compress with zlib deflate
						res = compress2(outbuf, &compsize, src, srclen,
							Z_BEST_COMPRESSION);
						if(res != Z_OK) compsize = -1;
					}
					catch(...)
					{
						compsize = -1;
					}

					if(compsize == (int64_t)-1)
					{
						delete [] outbuf;
						outbuf = NULL;
						storesize = filesize;
					}
					else
					{
						storesize = compsize;
					}
				}
				catch(...)
				{
					delete [] src;
					throw;
				}
				delete [] src;
			}



			// write
			try
			{
				TFileInfo info;
				info.FileSize = filesize;
				info.StoreSize = storesize;
				uint32_t flags = config->Protect ? (1<<31)/*protected*/:0;
				info.Flags = flags;
				info.Adler32 = adler32sum;
				info.LocalFileName = filename;

				// align to 8bytes
/*
				int padding = 8 - ((int)stream->GetPosition() & 0x07);
				if(padding != 8)
				{
					stream->WriteBuffer("padding-", padding);
				}
*/

				// write
				if(outbuf)
				{
					// write the compressed image
					TFileSegment seg;
					seg.Offset = stream->GetPosition() - offset;
					seg.StoreSize = storesize;
					seg.OrgSize = filesize;
					seg.Flags = 1; // compressed segment
					info.Segments.push_back(seg);
					stream->WriteBuffer(outbuf, storesize);
					delete [] outbuf;
					outbuf = NULL;
				}
				else
				{
					// copy whole file

					// share ogg vorbis codebooks
					bool segment_detected = false;
					std::vector<int> seg_refstart;
					std::vector<int> seg_bufstart;
					std::vector<int> seg_length;
					uint64_t ref_offset;
					if(isov)
					{
						// is ogg vorbis
						ovcb->HeadersLen[ovcb_index] =
							infile->Read(ovcb->Headers[ovcb_index], 
								OV_MATCH_BUF_LEN);
						ovcb->Offset[ovcb_index] = stream->GetPosition();
						infile->SetPosition(0);

						// detect match
						for(int i = 0; i<OV_MATCH_HOLD; i++)
						{
							if(i == ovcb_index) continue;
							int reflen;
							if((reflen = ovcb->HeadersLen[i]) != 0)
							{
								uint8_t *ref = ovcb->Headers[i];
								int buflen = ovcb->HeadersLen[ovcb_index];
								uint8_t *buf = ovcb->Headers[ovcb_index];

								seg_refstart.clear();
								seg_bufstart.clear();
								seg_length.clear();

								bool b = DetectMatch(ref, reflen, buf, buflen,
									seg_refstart, seg_bufstart, seg_length);
								if(b)
								{
									segment_detected = true;
									ref_offset = ovcb->Offset[i];
									break;
								}
							}
						}

						if(segment_detected)
						{
							ovcb->HeadersLen[ovcb_index] = 0;
						}
						else
						{
							// to next ...
							ovcb_index ++;
							if(ovcb_index == OV_MATCH_HOLD) ovcb_index = 0;
						}
					}

					if(segment_detected)
					{
						std::vector<int>::iterator rs, bs, l;
						TFileSegment seg;
						int pos = 0;
						rs = seg_refstart.begin();
						bs = seg_bufstart.begin();
						l = seg_length.begin();
						while(rs != seg_refstart.end())
						{
							int blen = *bs - pos;
							if(blen != 0)
							{
								seg.Flags = 0;
								seg.Offset = stream->GetPosition() - offset;
								Copy(stream.Get(), infile.Get(), blen);
								seg.OrgSize = seg.StoreSize = blen;
								info.Segments.push_back(seg);
							}

							seg.Flags = 0;
							seg.Offset = *rs + ref_offset - offset;
							seg.OrgSize = seg.StoreSize = *l;
							info.Segments.push_back(seg);
							infile->SetPosition(infile->GetPosition() + *l);

							pos = *bs + *l;

							rs++;
							bs++;
							l++;
						}

						seg.Offset = stream->GetPosition() - offset;
						uint64_t in_pos = infile->GetPosition();
						seg.OrgSize = seg.StoreSize = storesize - in_pos;
						if(seg.OrgSize)
						{
							info.Segments.push_back(seg);
							Copy(stream.Get(), infile.Get(), seg.OrgSize);
						}
					}
					else
					{
						TFileSegment seg;
						seg.Flags = 0;
						seg.Offset = stream->GetPosition() - offset;
						seg.OrgSize = seg.StoreSize = storesize;

						info.Segments.push_back(seg);
						Copy(stream.Get(), infile.Get(), storesize);
					}
				}
				info.FileName = storefilename;
				fileinfo.push_back(info);
			}
			catch(...)
			{
				if(outbuf) delete [] outbuf;
				throw;
			}
		}

		currentno ++;
	}


	// compute index size
	int64_t index_size = 0;

	std::vector<TFileInfo>::iterator it;
	for(it = fileinfo.begin(); it != fileinfo.end(); it++)
	{
		ttstr str = (it->FileName);

		// for each file
		index_size += 4 + 8 + 4 + 8 + 4 + 8 + 8 + 2 + str.length()*2 + 4 + 8 +
			4 + 8 + 4 +
			it->Segments.size()*28;
	}

	// write index pos
	{
		uint64_t pos_save = stream->GetPosition();
		stream->SetPosition(index_pointer_pos);
		uint64_t index_pos = pos_save - offset;
		stream->WriteBuffer(&index_pos, sizeof(index_pos));
		stream->SetPosition(pos_save);
	}

	// write index
	TVPAddLog(TJS_W("Writing index ..."));
#if 0
	Application->ProcessMessages();
	if(Aborted) throw Exception(InterruptedNameLabel->Caption);
#endif

	uint8_t *index_buf_alloc_p = new uint8_t [index_size];
	uint8_t *index_buf = index_buf_alloc_p;
	uint8_t *index_compressed = NULL;

	try
	{

		it = fileinfo.begin();
		for(it = fileinfo.begin(); it != fileinfo.end(); it++)
		{
			ttstr str = (it->FileName);

			// for each file
			memcpy(index_buf, "File", 4);
			index_buf += 4;
			*(int64_t*)(index_buf) = 4 + 8 + 4 + 8 + 8 + 2 + str.length()*2 + 4 + 8 +
				4 + 8 + 4 +	it->Segments.size()*28;
			index_buf += 8;

			// write "info"
			memcpy(index_buf, "info", 4);
			index_buf += 4;
			*(int64_t*)(index_buf) = 4 + 8 + 8  + 2 + str.length()*2;
			index_buf += 8;
			*(int32_t*)(index_buf) = it->Flags;
			index_buf += 4;
			*(int64_t*)(index_buf) = it->FileSize;
			index_buf += 8;
			*(int64_t*)(index_buf) = it->StoreSize;
			index_buf += 8;
			*(int16_t*)(index_buf) = str.length();
			index_buf += 2;

			tjs_char *tstr = new tjs_char[str.length() +1];
			try
			{
				// change path delimiter to '/'
				TJS_strcpy(tstr, str.c_str());
				tjs_char *p = tstr;
				while(*p)
				{
					if(*p == '\\') *p = '/';
					p++;
				}
				memcpy(index_buf, tstr, str.length() * 2);
				index_buf += str.length() * 2;
			}
			catch(...)
			{
				delete [] tstr;
				throw;
			}
			delete [] tstr;

			// write "segm"
			memcpy(index_buf, "segm", 4);
			index_buf += 4;
			*(int64_t*)(index_buf) = it->Segments.size() * 28;
			index_buf += 8;
			std::vector<TFileSegment>::iterator sit;
			for(sit = it->Segments.begin(); sit != it->Segments.end(); sit++)
			{
				*(int32_t*)(index_buf) = sit->Flags;
				index_buf += 4;
				*(int64_t*)(index_buf) = sit->Offset;
				index_buf += 8;
				*(int64_t*)(index_buf) = sit->OrgSize;
				index_buf += 8;
				*(int64_t*)(index_buf) = sit->StoreSize;
				index_buf += 8;
			}

			// write "adlr" (adler32 check sum)
			memcpy(index_buf, "adlr", 4);
			index_buf += 4;
			*(int64_t*)(index_buf) = 4;
			index_buf += 8;
			*(uint32_t*)(index_buf) = it->Adler32;
			index_buf += 4;

		}

		uLongf comp_size;
		if(config->CompressIndex)
		{
			index_compressed = new uint8_t [comp_size =
				index_size + index_size/100 + 1024];

			int res;
			int64_t srclen = (int64_t) index_size;
			try
			{
				// compress with zlib deflate
				res = compress2(index_compressed, &comp_size, index_buf_alloc_p, srclen,
					Z_BEST_COMPRESSION);
				if(res != Z_OK) comp_size = -1;
			}
			catch(...)
			{
				comp_size = -1;
			}

			if(comp_size == (uLongf)-1)
			{
				delete [] index_compressed;
				index_compressed = NULL;
			}
		}

		if(index_compressed)
		{
			tjs_uint8 flag = 1;
			stream->WriteBuffer(&flag, 1);
			stream->WriteBuffer(&comp_size, 8);
			stream->WriteBuffer(&index_size, 8);
			stream->WriteBuffer(index_compressed, comp_size);
		}
		else
		{
			tjs_uint8 flag = 0;
			stream->WriteBuffer(&flag, 1);
			stream->WriteBuffer(&index_size, 8);
			stream->WriteBuffer(index_buf_alloc_p, index_size);
		}

	}
	catch(...)
	{
		delete [] index_buf_alloc_p;
		if(index_compressed) delete [] index_compressed;
		throw;
	}

	delete [] index_buf_alloc_p;
	if(index_compressed) delete [] index_compressed;
}
//---------------------------------------------------------------------------

void SetFileList(TKrkrRelConfig *config)
{ 
#ifdef _WIN32
	tjs_string wname = config->ProjFolder;
	size_t baselen = wname.length();

	std::vector<tjs_string> dirs;
	dirs.push_back(wname);

	while (!dirs.empty())
	{
		tjs_string name = dirs.back();
		dirs.pop_back();
		WIN32_FIND_DATA find_data;
		HANDLE find_handle = FindFirstFile(name.c_str(), &find_data);

		if (find_handle == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		do
		{
			tjs_string file_name = find_data.cFileName;
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (file_name == TJS_W(".") || file_name == TJS_W(".."))
				{
					continue;
				}
				if (file_name == TJS_W("CVS"))
				{
					tjs_string wpath = name + file_name + TJS_W("\\Repository");
					if (TVPCheckExistentLocalFile(wpath))
					{
						continue;
					}
				}
				dirs.push_back(name + file_name + TJS_W("\\"));
			}
			else
			{
				config->FileList.push_back(tjs_string(name.c_str() + baselen) + file_name + TJS_W("\\"));
			}
		}
		while (FindNextFile(find_handle, &find_data));

		FindClose(find_handle);
	}
#else
	std::string nname;
	tjs_string wname = config->ProjFolder;
	TVPUtf16ToUtf8(nname, wname);
	size_t baselen = nname.length();

	std::vector<std::string> dirs;
	dirs.push_back(nname);

	while (!dirs.empty())
	{
		std::string name = dirs.back();
		dirs.pop_back();
		DIR *dir = opendir(name.c_str());
		if (!dir)
		{
			continue;
		}
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string file_name = entry->d_name;
			if (entry->d_type == DT_DIR)
			{
				if (file_name == "." || file_name == "..")
				{
					continue;
				}
				if (file_name == "CVS")
				{
					std::string npath = name + file_name + "/Repository";
					tjs_string wpath;
					TVPUtf8ToUtf16(wpath, npath);
					if (TVPCheckExistentLocalFile(wpath))
					{
						continue;
					}
				}
				dirs.push_back(name + file_name + "/");
			}
			else
			{
				std::string npath = std::string(name.c_str() + baselen) + file_name;
				tjs_string wpath;
				TVPUtf8ToUtf16(wpath, npath);
				config->FileList.push_back(wpath);
			}
		}
		closedir(dir);
	}
#endif
	// Sort files to ensure filenames are consistent
	std::sort(config->FileList.begin(), config->FileList.end());
} 

void SetExtList(TKrkrRelConfig *config)
{
	std::vector<tjs_string> ExtList;
	for (size_t i = 0; i < config->FileList.size(); i += 1)
	{
		ttstr ext = TVPExtractStorageExt(config->FileList.at(i));
		ext.ToLowerCase();
		if (std::find(ExtList.begin(), ExtList.end(), ext.AsStdString()) != ExtList.end())
		{
			continue;
		}
		ExtList.push_back(ext.AsStdString());
	}
	for (size_t i = 0; i < ExtList.size(); i += 1)
	{
		tjs_string ext = ExtList.at(i);
		if (
			ext == TJS_W(".xpk") ||
			ext == TJS_W(".xp3") ||
			ext == TJS_W(".exe") ||
			ext == TJS_W(".bat") ||
			ext == TJS_W(".tmp") ||
			ext == TJS_W(".db")  ||
			ext == TJS_W(".sue") ||
			ext == TJS_W(".vix") ||
			ext == TJS_W(".ico") ||
			ext == TJS_W(".aul") ||
			ext == TJS_W(".aue") ||
			ext == TJS_W(".rpf") ||
			ext == TJS_W(".bak") ||
			ext == TJS_W(".log") ||
			ext == TJS_W(".kep") ||
			ext == TJS_W(".cf")  ||
			ext == TJS_W(".cfu") ||
			ext == TJS_W("") || !TJS_strncmp(ext.c_str(), TJS_W(".~"), 2)
			)
		{
			if (ext == TJS_W(""))
			{
				ext = NoExtName;
			}
			config->DiscardExt.push_back(ext);
		}
		else if (
			ext == TJS_W(".wav") ||
			ext == TJS_W(".dll") ||
			ext == TJS_W(".tpi") ||
			ext == TJS_W(".spi") ||
			ext == TJS_W(".txt") ||
			ext == TJS_W(".mid") ||
			ext == TJS_W(".smf") ||
			ext == TJS_W(".swf") ||
			ext == TJS_W(".ks")  ||
			ext == TJS_W(".tjs") ||
			ext == TJS_W(".ma")  ||
			ext == TJS_W(".asq") ||
			ext == TJS_W(".asd") ||
			ext == TJS_W(".ttf") ||
			ext == TJS_W(".ttc") ||
			ext == TJS_W(".bmp") ||
			ext == TJS_W(".tft") ||
			ext == TJS_W(".cks") 
			)
		{
			config->CompressExt.push_back(ext);
		}
	}
}

std::vector<tjs_string> split_string(tjs_string str, tjs_string token)
{
    std::vector<tjs_string> result;
    while (str.size())
    {
        auto index = str.find(token);
        if (index != tjs_string::npos)
        {
            result.push_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if (str.size() == 0)
            {
            	result.push_back(str);
            }
        }
        else
        {
            result.push_back(str);
            str = TJS_W("");
        }
    }
    return result;
}

void LoadProfile(TKrkrRelConfig *config, tjs_string path)
{
#if 0
	iTJSTextReadStream * stream = TVPCreateTextStreamForReadByEncoding(path, TJS_W(""), TJS_W("UTF-8"));

	ttstr buffer;
	try 
	{
		stream->Read(buffer, 0);
	}
	catch (...) 
	{
		stream->Destruct();
		throw;
	}
	stream->Destruct();
	// Do stuff with buffer here…
#endif
}

void SaveProfile(TKrkrRelConfig *config, tjs_string path)
{
#if 0
	tTVPStreamHolder stream(path, TJS_BS_WRITE);
	stream->Write(buf, size);
#endif
}

int _argc;
std::vector<tjs_string> _argv;

#ifdef UNICODE
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	TVPLoadMessage();

	try
	{
		for (int i = 0; i < argc; i += 1)
		{
			tjs_string warg;
#ifdef UNICODE
			warg = argv[i];
#else
			TVPUtf8ToUtf16(warg, argv[i]);
#endif
			_argv.push_back(warg);
		}

		ttstr projfolder;
		ttstr outputfile;
		ttstr rpffile;
		bool write_rpf = true;
		for (int i = 1; i < argc; i += 1)
		{
			if (_argv[i][0] != TJS_W('-'))
			{
				// This is the project folder
				projfolder = _argv[i];
			}
			else if (_argv[i] == TJS_W("-go"))
			{
				// Since we don't have a GUI, we won't do anything here
			}
			else if (_argv[i] == TJS_W("-nowriterpf"))
			{
				write_rpf = false;
			}
			else if (_argv[i] == TJS_W("-out") && i + 1 < argc)
			{
				i += 1;
				outputfile = _argv[i];
			}
			else if (_argv[i] == TJS_W("-rpf") && i + 1 < argc)
			{
				i += 1;
				rpffile = _argv[i];
			}
		}
#ifdef _WIN32
		if (projfolder.GetLastChar() != TJS_W('\\'))
		{
			projfolder += TJS_W("\\");
		}
#else
		if (projfolder.GetLastChar() != TJS_W('/'))
		{
			projfolder += TJS_W("/");
		}
#endif
		TKrkrRelConfig config;
		config.CompressIndex = true;
		config.CompressLimit = true;
		config.Executable = false; // we don't support win32 exe self embedding at the moment
		config.OVBookShare = true;
		config.Protect = false;
		config.UseXP3EncDLL = false; // we don't support XP3 encryption at the moment
		config.WriteDefaultRPF = write_rpf;
		config.CompressLimitSize = 1024;
		config.OutputFileName = outputfile.AsStdString();
		config.ProjFolder = projfolder.AsStdString();
		SetFileList(&config);
		SetExtList(&config);
#if 0
		if (rpffile.length() != 0)
		{
			LoadProfile(&config, rpffile.AsStdString());
		}
		else if (TVPCheckExistentLocalFile(config.ProjFolder + TJS_W("default.rpf")))
		{
			LoadProfile(&config, config.ProjFolder + TJS_W("default.rpf"));
		}
#endif

		CreateArchive(&config);
#if 0
		// save current profile to the project folder
		if (config.WriteDefaultRPF)
		{
			try
			{
				SaveProfile(&config, config.ProjFolder + TJS_W("default.rpf"));
			}
			catch(...)
			{
				// ignode errors
			}
		}
#endif
	}
	catch (eTJSError &e)
	{
		TVPAddLog(ttstr("Error: ") + e.GetMessage());
	}
	return 0;
}


