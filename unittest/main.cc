#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <system_error>
#include <iostream>
#include "zlib.h"
#include "../src/zutil.h"

using namespace std;
using CharV = vector<char>;

class ZlibTest : public testing::Test
{
public:
	static void load_file(const string& fn, CharV& v)
	{
		std::ifstream f(fn);
		if (!f.is_open())		throw std::system_error(errno, std::system_category(), "file not found");

		f.seekg(0, ios_base::end);
		auto sz = f.tellg();
		f.seekg(0);
		
		v.resize(sz);
		f.read(&v[0], sz);
		
		if (f.gcount() != sz || f.tellg() != sz)	throw std::system_error(errno, std::system_category(), "read error");

		cout << "loaded " << fn << " size=" << v.size() << endl;
	}
	
	void deflate(const string& name, const CharV& in, CharV& out, bool gzip=false, size_t inblksz=64*1024, size_t outblksz=32*1024)
	{
		cout << endl << name << " deflate in size=" << in.size() << " gzip=" << gzip << endl;
		out.clear();

		uint8_t blk[outblksz];
		z_stream strm;
		strm.zalloc = nullptr;
		strm.zfree = nullptr;
		strm.opaque = nullptr;
		// 8 is the default mem level
		// 15 bits (32k window) is used for zlib data
		// +16 means gzip, and the library will add an empty gzip header
		int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + (gzip ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
 		ASSERT_EQ(ret, Z_OK);
		
		if (gzip)
		{
			gz_header gzh;
			memset(&gzh, 0, sizeof(gzh));
			gzh.text = 1;
			gzh.name = (uint8_t*)name.c_str();
			//gzh.name_max = name.size(); used during inflate to set the max length of the name string
			deflateSetHeader(&strm, &gzh);
		}
		
		size_t idx = 0;
		int flush;
		do
		{
			int nxtblksz = in.size() - idx < inblksz ? in.size() - idx : inblksz;
			strm.avail_in = nxtblksz;
			strm.next_in = (uint8_t*)&in[idx];
			flush = in.size() == idx ? Z_FINISH : Z_NO_FLUSH;
			cout << name << " nextin  avail_in=" << strm.avail_in << " flush=" << (flush == Z_FINISH ? "finish" : "no") << endl;

			do		// loop until the available buffer is not full
			{
				strm.next_out = &blk[0];
				strm.avail_out = outblksz;
				ret = ::deflate(&strm, flush);
				cout << name << " deflate avail_in=" << strm.avail_in << " avail_out=" << strm.avail_out << endl;
				ASSERT_NE(ret, Z_STREAM_ERROR);
				out.insert(out.end(), &blk[0], &blk[outblksz - strm.avail_out]);
			}
			while (strm.avail_out == 0);
			ASSERT_EQ(strm.avail_in, 0);
			idx += nxtblksz;
		}
		while (flush != Z_FINISH);
		ASSERT_EQ(ret, Z_STREAM_END);
		deflateEnd(&strm);
		cout << name << " deflate completed, outsz=" << out.size() << endl;
	}
	
	void inflate(const string& name, const CharV& in, CharV& out, bool gzip=false, size_t inblksz=64*1024, size_t outblksz=32*1024)
	{
		cout << endl << name << " inflate in size=" << in.size() << endl;
		out.clear();

		uint8_t blk[outblksz];
		z_stream strm;
		strm.zalloc = nullptr;
		strm.zfree = nullptr;
		strm.opaque = nullptr;
		strm.avail_in = 0;
		strm.next_in = nullptr;
		// -1 to -15 raw deflate (no zlib header)
		// 1 to 15 zlib data
		// windowsize 0 auto detect windowsize for zlib
		// +16 specify gzip
		// +32 automatic detection of gzip or zlib
		// 
		int ret = inflateInit2(&strm, 32);		// autodetect everything
 		ASSERT_EQ(ret, Z_OK);
		
		gz_header gzh;
		memset(&gzh, 0, sizeof(gzh));
		char hname[256];
		memset(hname, 0, sizeof(hname));
		gzh.name = (uint8_t*)hname;
		gzh.name_max = 256;
		inflateGetHeader(&strm, &gzh);
		
		size_t idx = 0;
		do
		{
			int nxtblksz = in.size() - idx < inblksz ? in.size() - idx : inblksz;
			strm.avail_in = nxtblksz;
			if (strm.avail_in == 0)
			{
				break;
			}
			strm.next_in = (uint8_t*)&in[idx];
			cout << name << " nextin  avail_in=" << strm.avail_in << endl;

			do		// loop until the available buffer is not full
			{
				strm.next_out = &blk[0];
				strm.avail_out = outblksz;
				ret = ::inflate(&strm, Z_NO_FLUSH);
				cout << name << " inflate avail_in=" << strm.avail_in << " avail_out=" << strm.avail_out << endl;
				ASSERT_NE(ret, Z_STREAM_ERROR);
				ASSERT_NE(ret, Z_NEED_DICT);
				ASSERT_NE(ret, Z_DATA_ERROR);
				ASSERT_NE(ret, Z_MEM_ERROR);
				out.insert(out.end(), &blk[0], &blk[outblksz - strm.avail_out]);
			}
			while (strm.avail_out == 0);
			//ASSERT_EQ(strm.avail_in, 0);
			idx += nxtblksz - strm.avail_in;
		}
		while (ret != Z_STREAM_END);
		inflateEnd(&strm);
		cout << name << " inflate completed, outsz=" << out.size() << " gzip_if_1=" << gzh.done << " head.name=" << hname << endl;
		if (gzip)
		{
			ASSERT_EQ(gzh.done, 1);
		}
		else
		{
			ASSERT_EQ(gzh.done, -1);
		}
	}
};

TEST_F(ZlibTest, zlib_deflate_inflate)
{
	CharV orig;
	load_file("../../../scclib/encode/unittest/oscar_wilde_poems_260k.txt", orig);

	CharV out;
	deflate("oscar.zlib", orig, out);
	/*auto f = ofstream("oscar.zlib", ios_base::out|ios_base::trunc);
	f.write(&out[0], out.size());
	f.close();*/

	CharV inf;
	inflate("oscar.zlib", out, inf);
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, gzip_deflate_inflate)
{
	CharV orig;
	load_file("../../../scclib/encode/unittest/oscar_wilde_poems_260k.txt", orig);

	CharV out;
	deflate("oscar.gz", orig, out, true);
	/*auto f.open("oscar.gz", ios_base::out|ios_base::trunc);
	f.write(&out[0], out.size());
	f.close();*/

	CharV inf;
	inflate("oscar.gz", out, inf, true);
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, gzip_cmdline_compressed)
{
	CharV orig;
	load_file("../../../scclib/encode/unittest/oscar_wilde_poems_260k.txt", orig);

	CharV bin;
	load_file("../../../scclib/encode/unittest/oscar_wilde_poems_260k.gz", bin);

	CharV inf;
	inflate("oscar.txt", bin, inf, true);

	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, html_gzip)
{
	CharV bin;
	load_file("../../../scclib/encode/unittest/http_encoded_gzip.gz", bin);

	CharV inf;
	inflate("http_gzip", bin, inf, true);

	cout << string(inf.begin(), inf.end()) << endl;
}

TEST_F(ZlibTest, html_deflate)
{
	CharV bin;
	load_file("../../../scclib/encode/unittest/http_encoded_deflate.zlib", bin);

	CharV inf;
	inflate("http_gzip", bin, inf);

	cout << string(inf.begin(), inf.end()) << endl;
}

int main(int argc, char**argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
