/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise {
using namespace juce;


const MarkdownLayout& MarkdownRenderer::LayoutCache::getLayout(const AttributedString& s, float w)
{
	int64 hash = s.getText().hashCode64();

	for (auto l : cachedLayouts)
	{
		if (l->hashCode == hash && l->width == w)
			return l->l;
	}

	auto newLayout = new Layout(s, w);

	cachedLayouts.add(newLayout);
	return newLayout->l;
}


MarkdownRenderer::LayoutCache::Layout::Layout(const AttributedString& s, float w):
	l(s, w)
{
	hashCode = s.getText().hashCode64();
	width = w;
}



MarkdownParser::MarkdownParser(const String& markdownCode_) :
	markdownCode(markdownCode_.replace("\r\n", "\n")),
	it(markdownCode),
	currentParseResult(Result::fail("Nothing parsed yet"))
{
	setImageProvider(new ImageProvider(this));
	
	setLinkResolver(new DefaultLinkResolver(this));
}


MarkdownParser::~MarkdownParser()
{

	elements.clear();
	linkResolvers.clear();
	imageProviders.clear();
}

void MarkdownParser::setFonts(Font normalFont_, Font codeFont_, Font headlineFont_, float defaultFontSize_)
{
	styleData.f = normalFont_;
	
	styleData.fontSize = defaultFontSize_;
}


juce::String MarkdownParser::resolveLink(const MarkdownLink& url)
{
	for (auto lr : linkResolvers)
	{
		auto link = lr->getContent(url);

		if (link.isNotEmpty())
			return link;
	}

	return "Can't resolve link " + url.toString(MarkdownLink::UrlFull);
}

juce::Image MarkdownParser::resolveImage(const MarkdownLink& imageUrl, float width)
{
	for (auto ip: imageProviders)
	{
		auto img = ip->getImage(imageUrl, width);

		if (img.isValid())
			return img;
	}

	return {};
}

void MarkdownParser::setDefaultTextSize(float fontSize)
{
	styleData.fontSize = fontSize;
}

String MarkdownParser::getCurrentText(bool includeMarkdownHeader) const
{
	if (includeMarkdownHeader)
		return markdownCode;
	else
		return markdownCode.fromLastOccurrenceOf("---\n", false, false);
}





void MarkdownParser::setNewText(const String& newText)
{
	resetCurrentBlock();
	elements.clear();

	markdownCode = newText;
	it = Iterator(markdownCode);
	parse();
}


bool MarkdownParser::gotoLink(const MarkdownLink& url)
{
	if (url.isSamePage(lastLink))
	{
		lastLink = url;
		jumpToCurrentAnchor();
		return true;
	}

	auto lastAnchor = lastLink.toString(MarkdownLink::AnchorWithHashtag);
	lastLink = url;

	for (auto r : linkResolvers)
	{
		if (r->linkWasClicked(url))
			return true;
	}

	String newText = resolveLink(url).replace("\r\n", "\n");

	setNewText(newText);
	
	auto thisAnchor = url.toString(MarkdownLink::AnchorWithHashtag);

	if (thisAnchor.isEmpty() || thisAnchor != lastAnchor)
		jumpToCurrentAnchor();
	

	return true;
}


MarkdownLink MarkdownParser::getLinkForMouseEvent(const MouseEvent& event, Rectangle<float> whatArea)
{
	auto link = getHyperLinkForEvent(event, whatArea);
	return link.url;
}

Array<MarkdownLink> MarkdownParser::getImageLinks() const
{
	Array<MarkdownLink> sa;

	for (auto e : elements)
	{
		e->addImageLinks(sa);
	}

	return sa;
}

hise::MarkdownParser::HyperLink MarkdownParser::getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area)
{
	float y = 0.0f;

	for (auto* e : elements)
	{
		auto heightToUse = e->getHeightForWidthCached(area.getWidth());
		heightToUse += e->getTopMargin();
		Rectangle<float> eBounds(area.getX(), y, area.getWidth(), heightToUse);

		if (eBounds.contains(event.getPosition().toFloat()))
		{
			auto translatedPoint = event.getPosition().toFloat();
			translatedPoint.addXY(eBounds.getX(), -eBounds.getY());

			for (auto& h : e->hyperLinks)
			{
				if (h.area.contains(translatedPoint))
					return h;
			}
		}

		y += eBounds.getHeight();
	}

	return {};
}


void MarkdownParser::createDatabaseEntriesForFile(File root, MarkdownDataBase::Item& item, File f, Colour c)
{
	jassert(root.isDirectory());

	MarkdownParser p(f.loadFileAsString());
	
	p.parse();

	if (p.getParseResult().failed())
	{
		DBG(p.getParseResult().getErrorMessage());
	}

	try
	{
		auto saveURL = item.url;

		item = MarkdownDataBase::Item(root, f, p.header.getKeywords(), p.header.getDescription());

		if (saveURL.isValid())
			item.url = saveURL;

		item.c = c;
		item.tocString = item.keywords[0];
		item.icon = p.getHeader().getKeyValue("icon");
		
		item.setIndexFromHeader(p.getHeader());
		item.applyWeightFromHeader(p.getHeader());

		MarkdownDataBase::Item lastHeadLine;

		for (auto e : p.elements)
		{
			if (auto h = dynamic_cast<Headline*>(e))
			{
				MarkdownDataBase::Item headLineItem(root, f, p.header.getKeywords(), p.header.getDescription());

				headLineItem.description = h->content.getText();

				if (headLineItem.description.trim() == item.tocString)
					continue;

				headLineItem.url = item.url.getChildUrl(h->anchorURL);
				headLineItem.c = c;

				headLineItem.tocString << headLineItem.description;

				if (h->headlineLevel >= 3)
					headLineItem.tocString = {};


				item.addChild(std::move(headLineItem));

			}
		}
	}
	catch (String& s)
	{
		ignoreUnused(s);
		DBG("---");
		DBG("Error parsing metadata for file " + f.getFullPathName());
		DBG(s);
	}
}





bool MarkdownParser::Helpers::isNewElement(juce_wchar c)
{
	return c == '#' || c == '|' || c == '!' || c == '>' || c == '-' || c == 0 || c == '\n' || CharacterFunctions::isDigit(c);
}

bool MarkdownParser::Helpers::isEndOfLine(juce_wchar c)
{
	return c == '\r' || c == '\n' || c == 0;
}


bool MarkdownParser::Helpers::isNewToken(juce_wchar c, bool isCode)
{
	if (c == '0')
		return true;

	const static String codeDelimiters("`\n\r");
	const static String delimiters("|>#");

	if (isCode)
		return codeDelimiters.indexOfChar(c) != -1;
	else
		return delimiters.indexOfChar(c) != -1;
}


bool MarkdownParser::Helpers::belongsToTextBlock(juce_wchar c, bool isCode, bool stopAtLineEnd)
{
	if (stopAtLineEnd)
		return !isEndOfLine(c);

	return !isNewToken(c, isCode);
}


void MarkdownParser::Element::drawHighlight(Graphics& g, Rectangle<float> area)
{
	if (selected)
	{
		g.setColour(parent->styleData.backgroundColour.contrasting().withAlpha(0.05f));
		g.fillRoundedRectangle(area.expanded(0.0f, 6.0f), 3.0f);
	}

	for (auto r : searchResults)
	{
		g.setColour(Colours::red.withAlpha(0.5f));

		auto r_t = r.translated(area.getX(), area.getY());
		g.fillRoundedRectangle(r_t, 3.0f);
		g.drawRoundedRectangle(r_t, 3.0f, 1.0f);
	}
}


float MarkdownParser::Element::getHeightForWidthCached(float width, bool forceUpdate)
{
	if (width != lastWidth || forceUpdate)
	{
		cachedHeight = getHeightForWidth(width);
		lastWidth = width;
		return cachedHeight;
	}

	return cachedHeight;
}

void MarkdownParser::Element::recalculateHyperLinkAreas(MarkdownLayout& l, Array<HyperLink>& links, float topMargin)
{
	for (const auto& area : l.linkRanges)
	{
		auto r = std::get<0>(area);

		for (auto& link : links)
		{
			if (link.urlRange == r)
			{
				link.area = std::get<1>(area).translated(0.0f, topMargin);
				break;
			}
		}
	}

#if 0
	for (auto& link : links)
	{
		bool found = false;

		
		
		for (int i = 0; i < l.getNumLines(); i++)
		{
			if (found)
				break;

			auto& line = l.getLine(i);
			
			for (auto r : line.runs)
			{
				if (r->font.isUnderlined())
				{
					

					if (r->glyphs.size() > 0)
					{
						auto thisRange = r->stringRange;
						auto urlRange = link.urlRange;

						if (!thisRange.getIntersectionWith(urlRange).isEmpty())
						{
							int i1 = 0;
							int i2 = r->glyphs.size() - 1;



							auto x = r->glyphs.getReference(i1).anchor.getX();
							auto& last = r->glyphs.getReference(i2);
							auto right = last.anchor.getX() + last.width;

							auto offset = line.leading;

							auto yRange = line.getLineBoundsY();

							link.area = { x + offset, yRange.getStart() + topMargin,
								right - x, yRange.getLength() };

							found = true;
							break;
						}
					}
				}
			}

		}
	}
#endif
}


void MarkdownParser::Element::prepareLinksForHtmlExport(const String& )
{
	for (auto& l : hyperLinks)
	{
		auto link = parent->getHolder()->getDatabase().getLink(l.url.toString(MarkdownLink::UrlFull));

		if (link.getType() == MarkdownLink::MarkdownFileOrFolder)
		{
			throw String("Can't resolve link `" + l.url.toString(MarkdownLink::UrlFull) + "`");
		}
		else
		{
			l.url = link;
		}
	}
}

juce::Array<juce::Range<int>> MarkdownParser::Element::getMatchRanges(const String& fullText, const String& searchString, bool countNewLines)
{
	int length = searchString.length();

	if (length <= 1)
		return {};

	int index = 0;
	auto c = fullText.getCharPointer();

	Array<Range<int>> ranges;

	while (*c != 0)
	{
		if (String(c).startsWith(searchString))
		{
			ranges.add({ index, index + length });

			c += length;
			index += length;
			continue;
		}

		if (countNewLines || *c != '\n')
			index++;

		c++;
	}

	return ranges;
}

void MarkdownParser::Element::searchInStringInternal(const AttributedString& textToSearch, const String& searchString)
{
	searchResults.clear();

	if (searchString.isEmpty())
		return;

	auto ranges = getMatchRanges(textToSearch.getText(), searchString, false);
	auto text = textToSearch.getText();

	if (ranges.size() > 0)
	{
		MarkdownLayout searchLayout(textToSearch, lastWidth, true);
		searchLayout.addYOffset((float)getTopMargin());

		for (auto r : ranges)
		{
			searchResults.add(searchLayout.normalText.getBoundingBox(r.getStart(), r.getLength(), true));
		}
	}
}

MarkdownParser::Iterator::Iterator(const String& text_) :
	text(text_),
	it(text.getCharPointer())
{

}

juce::juce_wchar MarkdownParser::Iterator::peek()
{
	if (it.isEmpty())
		return 0;

	return *(it);
}

bool MarkdownParser::Iterator::advanceIfNotEOF(int numCharsToSkip /*= 1*/)
{
	if (it.isEmpty())
		return false;

	it += numCharsToSkip;
	return !it.isEmpty();
}

bool MarkdownParser::Iterator::advance(int numCharsToSkip /*= 1*/)
{
	it += numCharsToSkip;
	return !it.isEmpty();
}

bool MarkdownParser::Iterator::advance(const String& stringToSkip)
{
	return advance(stringToSkip.length());
}

bool MarkdownParser::Iterator::next(juce::juce_wchar& c)
{
	if (it.isEmpty())
		return false;

	c = *it++;
	return c != 0;
}

bool MarkdownParser::Iterator::match(juce_wchar expected)
{
	juce_wchar c;

	if (!next(c))
		return false;

	if (c != expected)
	{
		String s;

		int lineNumber = 1;

		auto lineCounter = text.getCharPointer();

		while (lineCounter != it)
		{
			if (*lineCounter == '\n')
				lineNumber++;

			lineCounter++;
		}

		auto copy = it;

		int i = 20;

		

		while (*copy != 0 && i > 0)
		{
			copy++;
			i--;
		}

		String excerpt(it, copy);

		s << "Line " << String(lineNumber) << " - ";
		s << "Error at '" << excerpt << "': ";
		s << "Expected: " << expected;
		s << ", Actual: " << c;
		throw s;
	}

	return c == expected;
}

bool MarkdownParser::Iterator::matchIf(juce_wchar expected)
{
	juce_wchar c;

	if (peek() == expected)
		return next(c);

	return false;
}

void MarkdownParser::Iterator::skipWhitespace()
{
	juce_wchar c = peek();

	while (CharacterFunctions::isWhitespace(peek()))
	{
		if (!next(c))
			break;
	}
}

juce::String MarkdownParser::Iterator::getRestString() const
{
	if (it.isEmpty())
		return {};

	return String(it);
}


juce::String MarkdownParser::Iterator::advanceLine()
{
	String s;

	juce_wchar c;
	
	if (!next(c))
		return s;

	while (c != 0 && c != '\n')
	{
		s << c;
		if (!next(c))
			break;
	}

	if (c == '\n')
		s << c;

	return s;
}

int MarkdownParser::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	source.skipWhitespace();

	const juce_wchar firstChar = source.peekNextChar();

	switch (firstChar)
	{
	case '#':
	{
		source.skipToEndOfLine();
		return 1;
	}
	case '*':
	{
		while (source.peekNextChar() == '*')
			source.skip();

		while (!source.isEOF() && source.peekNextChar() != '*')
			source.skip();

		while (source.peekNextChar() == '*')
			source.skip();

		return 2;
	}
	case '`':
	{
		source.skip();

		while (!source.isEOF() && source.peekNextChar() != '`')
		{
			source.skip();
		}

		source.skip();

		return 3;
	}
	
	case '>':
	{
		source.skipToEndOfLine();
		return 4;
	}
	case '-':
	{
		source.skip();

		if (source.nextChar() == '-')
		{
			if (source.nextChar() == '-')
			{
				source.skipToEndOfLine();

				while (!source.isEOF())
				{
					if (source.nextChar() == '-')
					{
						if (source.nextChar() == '-')
						{
							if (source.nextChar() == '-')
							{
								break;
							}
						}
					}

					source.skipToEndOfLine();
				}

				return 5;
			}
			else
				return 0;
		}
		else
			return 0;
	}
	case '!':
	case '[':
	{
		source.skip();

		while (!source.isEOF() && source.peekNextChar() != ')')
			source.skip();

		source.skip();

		return 6;
	}
	case '|': source.skipToEndOfLine(); return 7;
	default: source.skip(); return 0;
	}
}

juce::CodeEditorComponent::ColourScheme MarkdownParser::Tokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme s;

	s.set("normal", Colour(0xFFAAAAAA));
	s.set("headline", Colour(SIGNAL_COLOUR).withAlpha(0.7f));
	s.set("highlighted", Colours::white);
	s.set("fixed", Colours::lightblue);
	s.set("comment", Colour(0xFF777777));
	s.set("metadata", Colour(0xFFaa7777));
	s.set("link", Colour(0xFF8888FF));
	s.set("table", Colour(0xFFCCCCCC));

	return s;
}


int MarkdownParser::SnippetTokeniser::readNextToken(CodeDocument::Iterator& source)
{

	auto c = source.nextChar();

	if (c == '{')
	{
		source.skipToEndOfLine();
		return 1;
	}
	else
		return 0;
}

juce::CodeEditorComponent::ColourScheme MarkdownParser::SnippetTokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme s;

	s.set("normal", Colour(0xFFAAAAAA));
	s.set("content", Colour(0xFF555555));

	return s;
}


juce::Image MarkdownParser::ImageProvider::getImage(const MarkdownLink& /*imageURL*/, float width)
{
	Image img = Image(Image::PixelFormat::ARGB, (int)width, (int)40.0f, true);
	Graphics g(img);
	g.fillAll(Colours::grey);
	g.setColour(Colours::black);
	g.drawRect(0.0f, 0.0f, width, 40.0f, 1.0f);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Empty", 0, 0, (int)width, (int)40.0f, Justification::centred);
	return img;
}


juce::Image MarkdownParser::ImageProvider::resizeImageToFit(const Image& otherImage, float width)
{
	if (width == 0.0)
		return {};

	if (otherImage.isNull() || otherImage.getWidth() < (int)width)
		return otherImage;

	float ratio = (float)otherImage.getWidth() / width;

	auto newWidth = jmax((int)width, 10);
	auto newHeight = jmax((int)((float)otherImage.getHeight() / ratio), 10);

	return otherImage.rescaled(newWidth, newHeight);
}


}

