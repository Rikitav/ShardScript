#pragma once
#include <shard/parsing/SourceReader.h>
#include <shard/syntax/analysis/TextLocation.h>

#include <fstream>
#include <string>

namespace shard::parsing
{
	class FileReader : public SourceReader
	{
		std::string Filename;
		std::fstream InputStream;

	public:
		FileReader(const std::string& fileName);
		~FileReader() override;

		shard::syntax::analysis::TextLocation GetLocation(std::string& word) override;

	protected:
		bool ReadNext() override;
		bool PeekNext() override;
	};
}