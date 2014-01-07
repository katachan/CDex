#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>
#include "in_vorbis_tags.h"
#include "in_vorbis_util.h"

#define MAX_STANDARD_STRING_SIZE 25

char VORBIS_STANDARD_TAGS[][MAX_STANDARD_STRING_SIZE] = 
{
	VORBIS_TAG_TITLE,
	VORBIS_TAG_ARTIST,
	VORBIS_TAG_ALBUM,
	VORBIS_TAG_DATE,
	VORBIS_TAG_COMMENT,
	VORBIS_TAG_TRACKNUMBER

};

BOOL in_vorbis_tags_is_standard_tag( const PCHAR pszTag )
{
	BOOL bReturn = FALSE;
	DWORD	nTag = 0;

	for ( nTag = 0; nTag < sizeof( VORBIS_STANDARD_TAGS ) / sizeof( VORBIS_STANDARD_TAGS[ 0 ] ); nTag++ )
	{
		if ( 0 == _strnicmp( pszTag,  VORBIS_STANDARD_TAGS[ nTag ], strlen( VORBIS_STANDARD_TAGS[ nTag ] ) ) )
		{
			bReturn = TRUE;
		}
	}

	return bReturn;
}



#if 0

#ifdef TAGZ_UNICODE

#define _TX(X)      L##X
#define t_strdup    wcsdup
#define t_strlen    wcslen
#define t_strnicmp  wcsnicmp
#define t_atoi(x)   wcstol(x,0,10)
#define t_stricmp   wcsicmp
#define t_strstr    wcsstr
#define sprintf     swprintf

#else

#define _TX(X)      X
#define t_strdup    strdup
#define t_strlen    strlen
#define t_strnicmp  strnicmp
#define t_atoi      atoi
#define t_stricmp   stricmp
#define t_strstr    strstr

#endif

#define TABSIZE(x) (sizeof(x)/sizeof(x[0]))

/*
class T_String
{
private:
	TCHAR * data;
	UINT size,used;
public:
	T_String() {data=0;size=0;used=0;}
	void AddChar(TCHAR c)
	{
		if (!data)
		{
			data=(TCHAR*)malloc((size=512)*sizeof(TCHAR));
			used=0;
		}
		else if (size==used)
		{
			size<<=1;
			data=(TCHAR*)realloc((char*)data,size*sizeof(TCHAR));
		}
		if (data) data[used++]=c;		
	}
	void AddInt(int i)
	{
		TCHAR foo[16];
		sprintf(foo,_TX("%i"),i);
		AddString(foo);
	}
	void AddString(const TCHAR * z)
	{
		while(*z) {AddChar(*z);z++;}
	}
	void AddString(T_String & s)
	{
		AddString(s.Peek());
	}
	~T_String()
	{
		if (data) free(data);
	}
	TCHAR * GetBuf()
	{
		if (!data) return ::t_strdup(_TX(""));
		TCHAR * r=(TCHAR*)realloc(data,(used+1)*sizeof(TCHAR));
		r[used]=0;
		data=0;
		return r;
	}
	TCHAR operator[](UINT i)
	{
		if (!data || i>=used) return 0;
		else return data[i];
	}
	UINT Len() {return data ? used : 0;}
	void Reset()
	{
		if (data) {free(data);data=0;}		
	}
	const TCHAR * Peek()
	{
		AddChar(0);
		used--;
		return data;
	}
	TCHAR * strdup()
	{
		return ::t_strdup(Peek());
	}
};

*/


static int separator( TCHAR x )
{
	if (!x || x==' ') return 1;
	if (x=='\'' || x=='_') return 0;
#ifdef TAGZ_UNICODE
	return !iswalnum(x);
#else
	return !isalnum(x);
#endif
}

static int sepcmp(TCHAR* src,TCHAR* val)
{
	UINT l=t_strlen(val);
	return !t_strnicmp(src,val,l) && separator(src[l]);
}

static char roman_num[]=
{
	'I','V','X','L','C','D','M'
};


static int is_roman(TCHAR * ptr)//could be more smart i think
{
	if (ptr[0]==']' && ptr[1]=='[' && separator(ptr[2])) return 1;
	while(!separator(*ptr))
	{
		UINT n;
		bool found=0;
		for(n=0;n<TABSIZE(roman_num);n++)
		{
			if (*ptr==roman_num[n]) {found=1;break;}
		}
		if (!found) return 0;
		ptr++;
	}
	return 1;
}

static int need_full(TCHAR* ptr)
{
	if (is_roman(ptr)) return 1;
	if (sepcmp(ptr,_TX("RPG"))) return 1;
	while(!separator(*ptr))
	{
		if (*ptr<'0' || *ptr>'9') return 0;
		ptr++;
	}
	return 1;
}

typedef bool (*TEXTFUNC)(UINT n_src,TCHAR **src,UINT*,T_String &out);

#define MAKEFUNC(X) static bool X(UINT n_src,TCHAR ** src,UINT *found_src,T_String &out)


MAKEFUNC(If)
{
	if (n_src!=3) return false;

	out.AddString(src[found_src[0] ? 1 : 2]);
    return true;
}

MAKEFUNC(If2)
{
	if (n_src!=2) return false;

	out.AddString(src[found_src[0] ? 0 : 1]);
    return true;
}


MAKEFUNC(Iflonger)
{
	if (n_src!=4) return false;

	out.AddString(src[(int)t_strlen(src[0])>t_atoi(src[1]) ? 2 : 3]);
    return true;
}

MAKEFUNC(Ifgreater)
{
	if (n_src!=4) return false;

	out.AddString(src[t_atoi(src[0])>t_atoi(src[1]) ? 2 : 3]);
    return true;
}

MAKEFUNC(Upper)
{
	if (n_src!=1) return false;

	TCHAR * s=src[0];

	while(*s)
		out.AddChar(toupper(*(s++)));

    return true;
}

MAKEFUNC(Lower)
{
	if (n_src!=1) return false;

	TCHAR * s=src[0];

	while(*s)
		out.AddChar(tolower(*(s++)));

    return true;
}

MAKEFUNC(Pad)
{
	if (n_src<2 || n_src>3) return false;

	TCHAR *fill=_TX(" ");
	if (n_src==3 && src[2][0])
        fill = src[2];

	int num = t_atoi(src[1]);
	TCHAR *p = src[0];

    while (*p) { out.AddChar(*(p++)); num--; }

	UINT fl = t_strlen(fill);
	while (num>0)
        out.AddChar(fill[(--num)%fl]);

    return true;
}

MAKEFUNC(Cut)
{
	if (n_src!=2) return false;

	UINT num = t_atoi(src[1]);
	TCHAR *p = src[0];

	while (*p && num>0) {out.AddChar(*(p++));num--;}

    return true;
}

MAKEFUNC(PadCut)
{
	if (n_src<2 || n_src>3) return false;

	TCHAR *fill = _TX(" ");
	if (n_src==3 && src[2][0])
        fill = src[2];

	int num = t_atoi(src[1]);
	TCHAR *p = src[0];

	while(*p && num>0) {out.AddChar(*(p++));num--;}

	UINT fl=t_strlen(fill);
	while (num>0)
        out.AddChar(fill[(--num)%fl]);

    return true;
}

// abbr(string)
// abbr(string,len)
MAKEFUNC(Abbr)
{
	if (n_src==0 || n_src>2) return false;


	if (n_src==2 && (int)t_strlen(src[0])<t_atoi(src[1]))
	{
		out.AddString(src[0]);
		return true;
	}

	TCHAR * meta=src[0];
	bool w=0, r=0;

	while(*meta)
	{
		bool an=!separator(*meta) || *meta==']' || *meta=='[';

		if (w && !an)
			w=0;
		else if (!w && an)
		{
			w=1;
			r=need_full(meta)?1:0;
			out.AddChar(*meta);
		}
		else if (w && r)
			out.AddChar(*meta);
		meta++;
	}

    return true;
}



MAKEFUNC(Caps)
{
	if (n_src!=1) return false;

	TCHAR* sp=src[0];
	int sep = 1;

	while(*sp)
	{
		TCHAR c=*(sp++);
		int s = separator(c);
		if (!s && sep)
			c=toupper(c);
		else if (!sep) c=tolower(c);
		sep=s;
		out.AddChar(c);
	}

    return true;
}

MAKEFUNC(Caps2)
{
	if (n_src!=1) return false;

	TCHAR* sp=src[0];
	int sep=1;

	while(*sp)
	{
		TCHAR c=*(sp++);
		int s = separator(c);
		if (!s && sep)
			c=toupper(c);
		sep=s;
		out.AddChar(c);
	}

    return true;
}

MAKEFUNC(Longest)
{
	TCHAR *ptr=0;
	UINT n, m=0;

	for(n=0;n<n_src;n++)
	{
		UINT l=t_strlen(src[n]);
		if (l>m) {m=l;ptr=src[n];}
	}

	if (ptr) out.AddString(ptr);
    return true;
}

MAKEFUNC(Shortest)
{
	TCHAR * ptr=0;
	UINT n,m=(UINT)(-1);

	for(n=0;n<n_src;n++)
	{
		UINT l=t_strlen(src[n]);
		if (l<m) {m=l;ptr=src[n];}
	}

	if (ptr) out.AddString(ptr);
    return true;
}

MAKEFUNC(Num)
{
	if (n_src!=2) return false;

	TCHAR tmp[16];
	TCHAR tmp1[16];
	sprintf(tmp1,_TX("%%0%uu"),t_atoi(src[1]));
	sprintf(tmp,tmp1,t_atoi(src[0]));
	out.AddString(tmp);

    return true;
}

MAKEFUNC(Hex)
{
	if (n_src!=2) return false;

    TCHAR tmp[16];
	TCHAR tmp1[16];
	sprintf(tmp1,_TX("%%0%ux"),t_atoi(src[1]));
	sprintf(tmp,tmp1,t_atoi(src[0]));
	out.AddString(tmp);

    return true;
}

MAKEFUNC(StrChr)
{
	if (n_src!=2) return false;

	TCHAR * p=src[0];
	TCHAR s=src[1][0];

	while (*p && *p!=s) p++;
	if (*p==s)
        out.AddInt(1+p-src[0]);
	else out.AddChar('0');

    return true;
}

MAKEFUNC(StrRChr)
{
	if (n_src!=2) return false;

	TCHAR * p=src[0],*p1=0;
	TCHAR s=src[1][0];

	while(*p)
	{
		if (*p==s) p1=p;
		p++;
	}

	if (p1)
        out.AddInt(1+p1-src[0]);
	else out.AddChar('0');

    return true;
}

MAKEFUNC(StrStr)
{
	if (n_src!=2) return false;

	TCHAR * p = t_strstr(src[0],src[1]);

	if (p)
        out.AddInt(1+p-src[0]);
	else out.AddChar('0');

    return true;
}

// substr(string, index)
// substr(string, index, length)
MAKEFUNC(SubStr)
{
	if (n_src<2 || n_src>3) return false;

	int n1 = t_atoi(src[1]), n2;

	if (n_src == 3)
		n2 = t_atoi(src[2]);
	else n2 = n1;

	if (n1 < 1) n1=1;
	if (n2 >= n1)
	{
		n1--;
		n2--;
		while(n1<=n2 && src[0][n1])
			out.AddChar(src[0][n1++]);
	}

    return true;
}

MAKEFUNC(Len)
{
	if (n_src!=1) return false;

    out.AddInt(t_strlen(src[0]));
    return true;
}

MAKEFUNC(Add)
{
	UINT n;
	int s=0;

	for (n=0;n<n_src;n++)
		s+=t_atoi(src[n]);

	out.AddInt(s);

    return true;
}

MAKEFUNC(Sub)
{
	if (n_src==0) return false;

	UINT n;
	int s=t_atoi(src[0]);

	for (n=1;n<n_src;n++)
		s-=t_atoi(src[n]);

	out.AddInt(s);

    return true;
}

MAKEFUNC(Mul)
{
	UINT n;
	int s=1;

	for(n=0;n<n_src;n++)
		s*=t_atoi(src[n]);

	out.AddInt(s);

    return true;
}
				
MAKEFUNC(Div)
{
	if (n_src==0) return false;

	UINT n;
	int s=t_atoi(src[0]);

	for(n=1;n<n_src;n++)
	{
		int t=t_atoi(src[n]);
		if (t) s/=t;
		else t=0;
	}

	out.AddInt(s);

    return true;
}

MAKEFUNC(Mod)
{
	if (n_src==0) return false;

	UINT n;
	int s=t_atoi(src[0]);

	for(n=1;n<n_src;n++)
	{
		int t=t_atoi(src[n]);
		if (t) s%=t;
		else t=0;
	}

	out.AddInt(s);

    return true;
}

MAKEFUNC(Max)
{
	if (!n_src) return false;

	int m = t_atoi(src[0]);
	UINT n;

	for (n=1; n<n_src; n++)
	{
		int t = t_atoi(src[n]);
		if (t > m) m = t;
	}
	out.AddInt(m);

    return true;
}

MAKEFUNC(Min)
{
	if (!n_src) return false;

	int m=t_atoi(src[0]);
	UINT n;

	for(n=1;n<n_src;n++)
	{
		int t=t_atoi(src[n]);
		if (t<m) m=t;
	}
	out.AddInt(m);

    return true;
}

// replace(string, what_to_replace, replacement)
MAKEFUNC(Replace)
{
	if (n_src!=3) return false;
	TCHAR *p = src[0];

	while (*p)
	{
		UINT n=0;

		while (src[1][n] && p[n]==src[1][n]) n++;

		if (!src[1][n])
		{
			out.AddString(src[2]);
			p += n;
		}
		else out.AddChar(*p++);
	}

    return true;
}

struct
{
	TEXTFUNC func;
	const TCHAR * name;
}

FUNCS[] =
{
	If,_TX("if"),
	If2,_TX("if2"),
	Upper,_TX("upper"),
	Lower,_TX("lower"),
	Pad,_TX("pad"),
	Cut,_TX("cut"),
	PadCut,_TX("padcut"),
	Abbr,_TX("abbr"),
	Caps,_TX("caps"),
	Caps2,_TX("caps2"),
	Longest,_TX("longest"),
	Shortest,_TX("shortest"),
	Iflonger,_TX("iflonger"),
	Ifgreater,_TX("ifgreater"),
	Num,_TX("num"),Num,_TX("dec"),
	Hex,_TX("hex"),
	StrChr,_TX("strchr"),
	StrChr,_TX("strlchr"),
	StrRChr,_TX("strrchr"),
	StrStr,_TX("strstr"),
	SubStr,_TX("substr"),
	Len,_TX("len"),
	Add,_TX("add"),
	Sub,_TX("sub"),
	Mul,_TX("mul"),
	Div,_TX("div"),
	Mod,_TX("mod"),
	Min,_TX("min"),
	Max,_TX("max"),
	Replace,_TX("replace"),
};


class FMT
{
private:
	T_String str;
	TCHAR * spec;
	TAGFUNC f;
	TAGFREEFUNC ff;
	void * fp;
	TCHAR * org_spec;
	int found;

	void Error(TCHAR *e=0)
    {
        str.Reset();
        str.AddString(e ? e : _TX("[SYNTAX ERROR IN FORMATTING STRING]"));
        found++;  // force displaying
    }

	TCHAR * _FMT(TCHAR * s,UINT *f=0)
	{
		FMT fmt(this,s);
		TCHAR * c=(TCHAR*)fmt;
		if (f) *f=fmt.found;
		found+=fmt.found;
		return c;
	}

	static bool skipshit(TCHAR** _p,TCHAR *bl)
	{
		TCHAR * p=*_p;
		int bc1=0,bc2=0;
		while(*p)
		{
			if (!bc1 && !bc2 && bl)
			{
				TCHAR *z=bl;
				while(*z)
				{
					if (*z==*p) break;
					z++;
				}
				if (*z) break;				
			}
			if (*p=='\'')
			{
				p++;
				while(*p && *p!='\'') p++;
				if (!*p) return 0;
			}
			else if (*p=='(') bc1++;
			else if (*p==')')
			{
				if (--bc1<0) return 0;
			}
			else if (*p=='[') bc2++;
			else if (*p==']')
			{
				if (--bc2<0) return 0;
			}
			p++;
		}
		*_p=p;
		return *p && !bc1 && !bc2;
	}

	void run()
	{
		if (!spec) {Error();return;}
		while(*spec)
		{
			if (*spec=='%')
			{
				spec++;
				if (*spec=='%') {str.AddChar('%');spec++;continue;}
				TCHAR* s1=spec+1;
				while(*s1 && *s1!='%') s1++;
				if (!*s1) {Error();break;}
				*s1=0;
				const TCHAR * tag=f(spec,fp);
				*s1='%';
				//if (!tag) tag=tag_unknown;
				if (tag && tag[0])
				{
					found++;
					str.AddString(tag);
				}
				else
				{
					str.AddString(_TX("?"));
				}
				if (tag && ff) ff(tag,fp);
				spec=s1+1;
			}
			else if (*spec=='$')
			{
				spec++;
				if (*spec=='$') {str.AddChar('$');spec++;continue;}
				TCHAR * s1=spec+1;
				while(*s1 && *s1!='(') s1++;
				if (!*s1) {Error();break;}
				TCHAR * s2=s1+1;
				if (!skipshit(&s2,_TX(")"))) {Error();break;}
				if (!*s2) {Error();break;};
				TCHAR * p=s1+1;
				TCHAR* temp[64];
				UINT temp_f[64];
				UINT nt=0;
				TCHAR * p1=s1+1;
				while(p<=s2 && nt<64)
				{
					if (!skipshit(&p,_TX(",)"))) {Error();return;}
					if (p>s2 || (*p!=',' && *p!=')')) {Error(_TX("internal error"));return;}
					TCHAR bk=*p;
					*p=0;
					temp[nt]=_FMT(p1,&temp_f[nt]);
					nt++;
					*p=bk;;
					p1=p+1;
					p++;
				}
				*s1=0;
				UINT n;

				for (n=0; n<TABSIZE(FUNCS); n++)
					if (!t_stricmp(spec, FUNCS[n].name))
                        break;

				*s1='(';

				if (n != TABSIZE(FUNCS))
				{
					if (!FUNCS[n].func(nt, temp, temp_f, str))
                    {
                        Error(_TX("[INVALID $"));
                        str.AddString(FUNCS[n].name);
                        str.AddString(_TX(" SYNTAX]"));
                        return;
                    }
				}
				else
                {
                    Error(_TX("[UNKNOWN FUNCTION]"));
                    return;
                }

				for(n=0;n<nt;n++) free(temp[n]);
				spec=s2+1;
			}
			else if (*spec=='\'')
			{
				spec++;
				if (*spec=='\'') {str.AddChar('\'');spec++;continue;}
				TCHAR * s1=spec+1;
				while(*s1 && *s1!='\'') s1++;
				if (!*s1) {Error();break;}
				*s1=0;
				str.AddString(spec);
				*s1='\'';
				spec=s1+1;
			}
			else if (*spec=='[')
			{
				spec++;
				TCHAR * s1=spec;
				UINT bc=0;
				if (!skipshit(&s1,_TX("]"))) {Error();break;}
				TCHAR bk=*s1;
				*s1=0;
				FMT fmt(this,spec);
				fmt.run();
				if (fmt.found)
				{
					str.AddString(fmt.str);
					found+=fmt.found;
				}
				*s1=bk;
				spec=s1+1;
			}
			else if (*spec == ']') {Error();break;}
			else
			{
				str.AddChar(*spec);
				spec++;
			}
		}
	}

	FMT(FMT* base,TCHAR * _spec)
	{
		found=0;
		org_spec=0;
		f=base->f;
		ff=base->ff;
		fp=base->fp;
		spec=_spec;
	}
public:
	FMT(const TCHAR * p_spec,TAGFUNC _f,TAGFREEFUNC _ff,void * _fp)
	{
		found=0;
		org_spec=spec=t_strdup(p_spec);
		f=_f;
		ff=_ff;
		fp=_fp;
	}
	operator TCHAR*()
	{
		run();
		return str.GetBuf();
	}
	~FMT()
	{
		if (org_spec) free(org_spec);
	}
};

extern "C"
{

UINT tagz_format(const TCHAR * spec,TAGFUNC f,TAGFREEFUNC ff,void *fp,TCHAR* out,UINT max)
{
	TCHAR * zz=tagz_format_r(spec,f,ff,fp);
	UINT r=0;
	while(r<max-1 && zz[r])
	{
		out[r]=zz[r];
		r++;
	}
	out[r]=0;
	free(zz);
	return r;
}	

TCHAR * tagz_format_r(const TCHAR* spec,TAGFUNC f,TAGFREEFUNC ff,void * fp)
{
	return FMT(spec,f,ff,fp);
}


#endif


const char in_vorbis_tags_manual[]=
{
	"Syntax reference: \n"
	"\n"
	"* %tagname% - inserts field named <tagname>, eg. \"%artist%\"\n"
	"* $abbr(x) - inserts abbreviation of x, eg. \"$abbr(%album%)\" - will convert album name of \"Final Fantasy VI\" to \"FFVI\"\n"
	"* $abbr(x,y) - inserts abbreviation of x if x is longer than y characters; otherwise inserts full value of x, eg. \"$abbr(%album%,10)\"\n"
	"* $lower(x), $upper(x) - converts x to in lower/uppercase, eg. \"$upper(%title%)\"\n"
	"* $num(x,y) - displays x number and pads with zeros up to y characters (useful for track numbers), eg. $num(%tracknumber%,2)\n"
	"* $caps(x) - converts first letter in every word of x to uppercase, and all other letters to lowercase, eg. \"blah BLAH\" -> \"Blah Blah\"\n"
	"* $caps2(x) - similar to $caps, but leaves uppercase letters as they are, eg. \"blah BLAH\" -> \"Blah BLAH\"\n"
	"* $if(A,B,C) - if A contains at least one valid tag, displays B, otherwise displays C; eg. \"$if(%artist%,%artist%,unknown artist)\" will display artist name if present; otherwise will display \"unknown artist\"; note that \"$if(A,A,)\" is equivalent to \"[A]\" (see below)\n"
	"* $if2(A,B) - equals to $if(A,A,B)\n"
	"* $longest(A,B,C,....) - compares lengths of output strings produced by A,B,C... and displays the longest one, eg. \"$longest(%title%,%comment%)\" will display either title if it's longer than comment; otherwise it will display comment\n"
	"* $pad(x,y) - pads x with spaces up to y characters\n"
	"* $cut(x,y) - truncates x to y characters\n"
	"* $padcut(x,y) - pads x to y characters and truncates to y if longer\n"
	"* [ .... ] - displays contents of brackets only if at least one of fields referenced inside has been found, eg. \"%artist% - [%album% / ]%title%\" will hide [] block if album field is not present\n"
	"* \' (single quotation mark) - outputs raw text without parsing, eg, \'blah$blah%blah[][]\' will output the contained string and ignore all reserved characters (%,$,[,]) in it; you can use this feature to insert square brackets for an example.\n"
	"\n"
	"eg. \"[%artist% - ][$abbr(%album%,10)[ %tracknumber%] / ]%title%[ %streamtitle%]\"\n"
};


PCHAR CreateTitleString( PCHAR pszFileName )
{
	PCHAR	pszTitle	= NULL;
	PCHAR	pszArtist	= NULL;
	PCHAR	pszAlbum	= NULL;
	PCHAR	pszTempBuf	= NULL;

	int		nTitleSize	= 256;		// reserve some extra bytes


	pszTitle = get_tag_value_by_name( VORBIS_TAG_TITLE );
	pszArtist = get_tag_value_by_name( VORBIS_TAG_ARTIST );
	pszAlbum = get_tag_value_by_name( VORBIS_TAG_ALBUM );

	if ( vpi_info.pszTitle )
	{
		free( vpi_info.pszTitle );
		vpi_info.pszTitle = NULL;
	}

	if ( pszFileName ) 
	{
		nTitleSize += strlen( pszFileName );
	}
	if ( pszTitle ) 
	{
		nTitleSize += strlen( pszArtist );
	}
	if ( pszArtist ) 
	{
		nTitleSize += strlen( pszArtist );
	}
	if ( pszAlbum ) 
	{
		nTitleSize += strlen( pszAlbum );
	}

	pszTempBuf = calloc( nTitleSize, sizeof( CHAR ) );

	vpi_info.pszTitle = calloc( nTitleSize, sizeof( CHAR ) );

	if ( pszTempBuf && vpi_info.pszTitle )
	{

		if( pszArtist && pszTitle )
		{
			_snprintf( pszTempBuf, nTitleSize, "%s - %s", pszArtist, pszTitle );
		}
		else if( pszTitle )
		{
			_snprintf( pszTempBuf, nTitleSize, "%s", pszTitle );
		}
		else if( pszArtist )
		{
			_snprintf( pszTempBuf, nTitleSize, "%s - unknown", pszArtist );
		}
		else
		{
			_snprintf( pszTempBuf, nTitleSize, "%s (no title)", pszFileName );
		}

		// convert to ACSII
		UTT8_2_ASCII( pszTempBuf, vpi_info.pszTitle, nTitleSize );
	}

	if ( pszTempBuf )
	{
		free ( pszTempBuf );
	}

	return vpi_info.pszTitle;
}
