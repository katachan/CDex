/***************************************************************************
    copyright            : (C) 2002, 2003 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include <tbytevectorlist.h>
#include <tdebug.h>

#include "commentsframe.h"

using namespace TagLib;
using namespace ID3v2;

class CommentsFrame::CommentsFramePrivate
{
public:
  CommentsFramePrivate() : textEncoding(String::Latin1) {}
  String::Type textEncoding;
  ByteVector language;
  String description;
  String text;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

CommentsFrame::CommentsFrame(String::Type encoding) : Frame("COMM")
{
  d = new CommentsFramePrivate;
  d->textEncoding = encoding;
}

CommentsFrame::CommentsFrame(const ByteVector &data) : Frame(data)
{
  d = new CommentsFramePrivate;
  setData(data);
}

CommentsFrame::~CommentsFrame()
{
  delete d;
}

String CommentsFrame::toString() const
{
  return d->text;
}

ByteVector CommentsFrame::language() const
{
  return d->language;
}

String CommentsFrame::description() const
{
  return d->description;
}

String CommentsFrame::text() const
{
  return d->text;
}

void CommentsFrame::setLanguage(const ByteVector &languageEncoding)
{
  d->language = languageEncoding.mid(0, 3);
}

void CommentsFrame::setDescription(const String &s)
{
  d->description = s;
}

void CommentsFrame::setText(const String &s)
{
  d->text = s;
}


String::Type CommentsFrame::textEncoding() const
{
  return d->textEncoding;
}

void CommentsFrame::setTextEncoding(String::Type encoding)
{
  d->textEncoding = encoding;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void CommentsFrame::parseFields(const ByteVector &data)
{
  if(data.size() < 5) {
    debug("A comment frame must contain at least 5 bytes.");
    return;
  }

  d->textEncoding = String::Type(data[0]);
  d->language = data.mid(1, 3);

  int byteAlign = d->textEncoding == String::Latin1 || d->textEncoding == String::UTF8 ? 1 : 2;

  ByteVectorList l = ByteVectorList::split(data.mid(4), textDelimiter(d->textEncoding), byteAlign);

  if(l.size() == 2) {
    d->description = String(l.front(), d->textEncoding);
    d->text = String(l.back(), d->textEncoding);
  }
}

ByteVector CommentsFrame::renderFields() const
{
  ByteVector v;

  v.append(char(d->textEncoding));
  v.append(d->language.size() == 3 ? d->language : "   ");
  v.append(d->description.data(d->textEncoding));
  v.append(textDelimiter(d->textEncoding));
  v.append(d->text.data(d->textEncoding));

  return v;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

CommentsFrame::CommentsFrame(const ByteVector &data, Header *h) : Frame(h)
{
  d = new CommentsFramePrivate();
  parseFields(fieldData(data));
}
