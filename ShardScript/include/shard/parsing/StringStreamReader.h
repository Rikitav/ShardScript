#pragma once
#include <shard/parsing/SourceReader.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <sstream>
#include <string>

namespace shard::parsing
{
	class StringStreamReader : public SourceReader
	{
		std::istringstream stringStream;

	public:
		StringStreamReader(const std::string& source);

	protected:
		shard::syntax::analysis::TextLocation GetLocation(std::string& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}
