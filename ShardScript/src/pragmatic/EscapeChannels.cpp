#include <shard/pragmatic/EscapeChannels.hpp>

using namespace shard;

EscapeLevel shard::join_escape_level(EscapeLevel left, EscapeLevel right)
{
	return left > right ? left : right;
}

EscapeChannel shard::operator|(EscapeChannel left, EscapeChannel right)
{
	return static_cast<EscapeChannel>(static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
}

EscapeChannel shard::operator&(EscapeChannel left, EscapeChannel right)
{
	return static_cast<EscapeChannel>(static_cast<unsigned int>(left) & static_cast<unsigned int>(right));
}
