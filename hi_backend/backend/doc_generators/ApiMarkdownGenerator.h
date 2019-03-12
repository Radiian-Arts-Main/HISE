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

#pragma once

namespace hise {
using namespace juce;


struct AutogeneratedDatatabase
{
	virtual void registerAtDatabase(MarkdownDataBase& db) = 0;
};


struct ScriptingApiDatabase
{
	static constexpr char apiWildcard[] = "/scripting/scripting-api";

	struct Data
	{
		Data();
		~Data();

		
		ValueTree v;
	};

	struct ItemGenerator : public hise::MarkdownDataBase::DirectoryItemGenerator
	{
		ItemGenerator(File rootFile_, MarkdownDatabaseHolder& holder_):
		  DirectoryItemGenerator(rootFile_.getChildFile("scripting"), Colours::pink),
		  rootFile(rootFile_),
		  rootUrl(rootFile_, apiWildcard),
		  holder(holder_)
		{}

		MarkdownDataBase::Item createRootItem(MarkdownDataBase& parent) override;

	private:

		

		MarkdownDatabaseHolder& holder;

		File rootFile;
		MarkdownLink rootUrl;

		MarkdownDataBase::Item updateWithValueTree(MarkdownDataBase::Item& item, ValueTree& v);

		SharedResourcePointer<Data> data;
	};

	struct Resolver : public hise::MarkdownParser::LinkResolver
	{
		Resolver(File scriptingDocRoot) :
			LinkResolver(),
			docRoot(scriptingDocRoot),
			rootURL(docRoot, apiWildcard)
		{}

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("ScriptingApiLinkResolver"); }

		LinkResolver* clone(MarkdownParser* parent) const override
		{
			return new Resolver(docRoot);
		}

		String getContent(const MarkdownLink& url) override;
		String createMethodText(ValueTree& mv);
		File getFileToEdit(const MarkdownLink& url) override;

		SharedResourcePointer<Data> data;

		File docRoot;
		MarkdownLink rootURL;
	};
};


}