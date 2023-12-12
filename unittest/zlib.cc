/*
BSD 3-Clause License

Copyright (c) 2022, Stable Cloud Computing, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <system_error>
#include <iostream>
#include <sstream>
#include <zlib.h>
#include <util/fs.h>
#include <cstdlib>

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::vector;
using std::ios_base;
using std::system_error;
using std::stringstream;
using fs = scc::util::Filesystem;

class ZlibTest : public testing::Test
{
	string curdir;
public:
	ZlibTest()
	{
		system_error err;
		curdir = fs::get_current_dir();

cout << "current dir: " << curdir << endl;

		fs::remove_all("sandbox", &err);
		fs::create_dir("sandbox");
	}
	virtual ~ZlibTest()
	{
		fs::change_dir(curdir);
		system_error err;
		fs::remove_all("sandbox", &err);
	}

	void load_file(const string& fn, vector<char>& v)
	{
		// detect the bazel workspace environment, and create a relative path to the data files
		auto sd = getenv("TEST_SRCDIR");
		if (sd)
		{
			cout << "TEST_SRCDIR=" << sd << endl;
		}
		auto wd = getenv("TEST_WORKSPACE");
		if (wd)
		{
			cout << "TEST_WORKSPACE=" << wd << endl;
		}

		stringstream fname;

		if (sd && wd)
		{
			fname << sd << "/" << wd << "/unittest/" << fn;
		}
		else
		{
			fname << fn;
		}

		cout << "loading file " << fname.str() << endl;

		ifstream f(fname.str());
		if (!f.is_open())
		{
			throw system_error(errno, std::system_category(), fn);
		}

		f.seekg(0, ios_base::end);
		auto sz = f.tellg();
		f.seekg(0);
		
		v.resize(sz);
		f.read(&v[0], sz);
		
		if (f.gcount() != sz || f.tellg() != sz)
		{
			throw system_error(errno, std::system_category(), "read error");
		}

		cout << "loaded " << fn << " size=" << v.size() << endl;
	}
	
	void deflate(const string& name, const vector<char>& in, vector<char>& out, int window_size, size_t inblksz=64*1024, size_t outblksz=32*1024)
	{
		cout << endl << name << " deflate name=" << name << " window size=" << window_size << " in size=" << in.size() << " ";
		
		if (window_size < 0)
		{
			cout << "raw deflate";
		}
		else if (window_size < 16)
		{
			cout << "zlib header";
		}
		else
		{
			cout << "gzip header";
		}
		cout << endl;

		out.clear();

		uint8_t blk[outblksz];
		z_stream strm;
		strm.zalloc = nullptr;
		strm.zfree = nullptr;
		strm.opaque = nullptr;
		
		// 8 is the default mem level

		// window size
		// -1 to -15 raw deflate
		// 1 to 15 zlib data
		// windowsize 0 auto detect windowsize for zlib
		// +16 means gzip, and the library will add an empty gzip header

		// 15 bits (32k window) is used for zlib data
		
		int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_size, 8, Z_DEFAULT_STRATEGY);		// mem size 8 is default
 		ASSERT_EQ(ret, Z_OK);
		
		gz_header gzh;
		memset(&gzh, 0, sizeof(gzh));
		gzh.text = 1;
		gzh.name = (uint8_t*)name.c_str();

		if (window_size > 16)
		{
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
		cout << name << " deflate completed, outsz=" << out.size() << " gzh.done=" << gzh.done << " gzh.name=" << gzh.name << endl;
	}
	
	void inflate(const string& name, const vector<char>& in, vector<char>& out, int window_size = 32, size_t inblksz=64*1024, size_t outblksz=32*1024)
	{
		cout << endl << name << " inflate name=" << name << " window size=" << window_size << " in size=" << in.size() << endl;
		out.clear();

		uint8_t blk[outblksz];
		z_stream strm;
		strm.zalloc = nullptr;
		strm.zfree = nullptr;
		strm.opaque = nullptr;
		strm.avail_in = 0;
		strm.next_in = nullptr;

		// window size
		// -1 to -15 raw deflate (no zlib header)
		// 1 to 15 zlib data
		// windowsize 0 auto detect windowsize for zlib
		// +16 specify gzip
		// +32 automatic detection of gzip or zlib

		int ret = inflateInit2(&strm, window_size);
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
		cout << name << " inflate completed, outsz=" << out.size() << " gzh.done=" << gzh.done << " gzh.name=" << gzh.name << endl;

		if (gzh.done == 1)
		{
			ASSERT_EQ(string((char*)gzh.name), name);
		}
	}
};

TEST_F(ZlibTest, raw_test)
{
	vector<char> orig;
	load_file("pg1524.txt", orig);

	vector<char> out;
	deflate("hamlet.deflate", orig, out, -15);		// maximum raw compression

	vector<char> inf;
	inflate("hamlet.deflate", out, inf, -15);		// raw inflate must have the same window size
	
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, zlib_test)
{
	vector<char> orig;
	load_file("pg1524.txt", orig);

	vector<char> out;
	deflate("hamlet.zlib", orig, out, 15);		// maximum zlib compression

	vector<char> inf;
	inflate("hamlet.zlib", out, inf, 32);		// automatic detection
	
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, gzip_test)
{
	vector<char> orig;
	load_file("pg1524.txt", orig);

	vector<char> out;
	deflate("hamlet.gzip", orig, out, 15+16);	// maximum gzip compression

	vector<char> inf;
	inflate("hamlet.gzip", out, inf, 32);		// automatic detection
	
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, gzip_ext_inflate)
{
	vector<char> orig;
	load_file("pg1524.txt", orig);

	vector<char> def;
	load_file("pg1524.txt.gz", def);

	vector<char> inf;
	inflate("pg1524.txt", def, inf, 32);		// automatic detection
	
	ASSERT_EQ(orig, inf);
}

TEST_F(ZlibTest, zlib_ext_inflate)
{
	vector<char> orig;
	load_file("pg1524.txt", orig);

	vector<char> def;
	load_file("pg1524.txt.zlib", def);

	vector<char> inf;
	inflate("pg1524.txt", def, inf, 32);		// automatic detection
	
	ASSERT_EQ(orig, inf);
}
