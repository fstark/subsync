//
//  main.cpp
//  SubSync
//
//  Created by Frederic Stark on 16/10/16.
//  Copyright © 2016 Fred. All rights reserved.
//

#include <iostream>
#include <chrono>
#include <string>
#include <set>
#include <algorithm>
#include <map>
#include <vector>
#include <fstream>

using namespace std;
using namespace std::chrono;

bool read_time( const string str, milliseconds &time )
{
	int h, m, s, ms;
	if (	sscanf( str.c_str(), "%02d:%02d:%02d,%03d", &h, &m, &s, &ms )!=4
		&&	sscanf( str.c_str(), "%02d:%02d:%02d.%03d", &h, &m, &s, &ms )!=4
		)
		return false;
	time = milliseconds{ ms+s*1000+m*60*1000+h*60*60*1000 };
	return true;
}

///	Just get the upercase text/numbers
char clean_char( char c )
{
	if (c>='a' && c<='z')
		c -= 'a'-'A';
	switch (c)
	{
		case '_':
		case '<':
		case '>':
		case ',':
		case '.':
		case '!':
		case '?':
		case '/':
		case '"':
		case '\'':
		case '[':
		case ']':
		case '{':
		case '}':
		case '*':
		case '%':
		case '$':
		case '@':
		case '-':
		case '+':
		case '\n':
		case '\r':
		case '\t':
			c = ' ';
	}
	return c;
}

string removing_suffix( const string &word, const string &suffix )
{
	if (auto pos = word.rfind(suffix))
	{
		if (pos!=string::npos && pos==word.length()-suffix.length())
			return word.substr(0,word.length()-suffix.length());
	}
	return word;
}

string remove_suffix( const string &word )
{
	auto result = removing_suffix( word, "S" );
	result = removing_suffix( result, "ING" );
	if (result=="OKAY") { result = "OK"; }
	return result;
}

vector<string> list_words( const string &str )
{
	vector<string> result;
	string s = str;
	transform( s.begin(), s.end(), s.begin(), clean_char );
	
	//	split in words and add
	//	yo
	string w;
	for (auto c:s)
	{
		if (c==' ' && w.size()!=0)
		{
			result.push_back( remove_suffix( w ) );
			w = "";
		}
		else if (c!=' ')
			w += c;
	}
	if (w.size()!=0)
	{
		result.push_back( remove_suffix( w ) );
	}

	return result;
}

vector<string> unique_sorted( const vector<string> &strings )
{
	set<string> s;
	s.insert( strings.begin(), strings.end() );
	vector<string> r;
	r.insert( r.begin(), s.begin(), s.end() );
	return r;
}

/** A text corpus, with word frequencies */
class CCorpus
{
	map<string,int> words;
	int total = 0;

	void add_word( const string &word )
	{
		words[word]++;
		total++;
	}
public:
	float frequency( string w )
	{
		auto f = words.find( w );
		if (f!=words.end())
			return f->second/(float)total;
		return 0;
	}

	void add( const string &s )
	{
		for (auto s:list_words(s))
			add_word( s );
	}
};


class CLine
{
	milliseconds start;
	milliseconds duration;
	string text_;
	vector<string> sorted_words_;
public:
	CLine( const milliseconds start=milliseconds{0}, const milliseconds duration=milliseconds{0}, const string text="" ) :
	start( start ),
	duration( duration )
	{
		set_text( text );
	}
	
	long start_ms() { return start.count(); }
	
	void set_text( const string &text )
	{
		text_ = text;
		sorted_words_ = unique_sorted( list_words( text ) );
	}
	
	const string &text() const { return text_; }
	
	static bool read( istream &in, CLine &line )
	{
		int n;
		in >> n;

		milliseconds start;
		milliseconds stop;
		
		string s;
		in >> s;
		if (!read_time( s, start ))
			return false;
		in >> s;	//	-->
		in >> s;
		if (!read_time( s, stop ))
			return false;
		line = CLine{ start, stop-start, "" };
		getline( in, s );

		string text;
		while (getline( in, s ) && s!="\r" && s!="")
		{
			text += s;
		}
		line.set_text( text );

		return true;
	}

	friend float dot( const CLine &l0, const CLine &l1, function<float ( const string & )> );
};

float dot( const CLine &l0, const CLine &l1, function<float ( const string & )> freq )
{
	auto i0 = l0.sorted_words_.begin();
	auto e0 = l0.sorted_words_.end();
	auto i1 = l1.sorted_words_.begin();
	auto e1 = l1.sorted_words_.end();
	
	float d = 0;
	float total = 0;	//	The total we could have had with words from both strings

	while (i0!=e0 && i1!=e1)
		if (*i0<*i1)
		{
			total += freq( *i0 );
			i0++;
		}
	else
		if (*i0==*i1)
		{
			auto f = freq( *i0 );
			total += f;
			i0++;
			i1++;
			d += f;
		}
	else
//		if (*i0>*i1)
	{
		total += freq( *i1 );
		i1++;
	}

	while (i0!=e0) { total += freq( *i0 ); i0++; }
	while (i1!=e1) { total += freq( *i1 ); i1++; }

	return d/total;
//	return t?d/t:0;
}

int main(int argc, const char * argv[])
{
	CCorpus c;
//	c.add( "images" );
	
	auto input1 = ifstream{ "/Users/fred/Development/C++/SubSync/EN1.srt", iostream::binary };
	if (!input1.good())
	{
		input1.close();
		exit( EXIT_FAILURE );
	}

	vector<CLine> v;
	CLine l;
	while (CLine::read( input1, l ))
	{	v.push_back( l );
		c.add( l.text() );
	}
	input1.close();

	
	
	
	auto input2 = ifstream{ "/Users/fred/Development/C++/SubSync/EN-FR.srt", iostream::binary };
	if (!input2.good())
	{
		input2.close();
		exit( EXIT_FAILURE );
	}

	vector<CLine> v2;
	while (CLine::read( input2, l ))
	{	v2.push_back( l );
	}
	input2.close();

	
	
	
	//	CLine l( "00:00:23,020" );
//	CLine ll1{ milliseconds{0}, milliseconds{0}, "I like hanging out with other vampires." };
	int i=0;
	for (auto &ll1:v2)
	{
		float max = 0;
		int pos = 0;
		int max_pos = 0;
		
		auto freq = [&]( const string &s )
		{
			auto f = c.frequency( s );
			if (f==0) return (float)0;
			return 1/f;
		};
		
		CLine ll2{ milliseconds{0}, milliseconds{0}, "Awaken!" };
		
		for (auto &l:v)
		{
			auto m = dot( ll1, l, freq );
			if (m>max)
			{
				max = m;
				max_pos = pos;
//				cout << max_pos << " (" << max << ") : " << v[max_pos].text() << endl << endl;
			}
			pos++;
		}
		if (max>0.9)
		{
//			cout << ll1.text() << " @"<< i << endl;
//			cout << "> " << v[max_pos].text() << " @" << max_pos << " (" << max << ")" << endl << endl;
			cout << ll1.start_ms() << ";" << v[max_pos].start_ms() << endl;
		}
		i++;
	}
		
	return 0;
}
