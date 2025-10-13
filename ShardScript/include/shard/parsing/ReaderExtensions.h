#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/SourceReader.h>
#include <vector>

using namespace std;
using namespace shard::parsing;

static vector<SyntaxToken> ReadToEnd(SourceReader& reader)
{
	vector<SyntaxToken> sequence;
	while (reader.CanConsume())
	{
		sequence.push_back(reader.Current());
		reader.Consume();
	}

	return sequence;
}
