#pragma once

#include <array>
#include <string>

template < std::size_t N, std::size_t ... Is >
constexpr std::array< char, N - 1 > ToArray( const char ( &a )[ N ], std::index_sequence< Is... > )
{
    return { { a[ Is ]... } };
}

template < std::size_t N >
constexpr std::array< char, N - 1 > ToArray( const char ( &a )[ N ] )
{
    return ToArray( a, std::make_index_sequence< N - 1 >() );
}

enum class Opcode
{
    opInsert,
    opUpdate,
    opDelete,
    opGet,
    op__MaxCount,
    opInvalid = op__MaxCount
};

namespace network
{

struct RequestHeader;

struct DecodedHeader
{
    static constexpr long MAX_KEY_SIZE = 1024;
    static constexpr long MAX_VALUE_SIZE = 1024 * 1024;

    DecodedHeader( Opcode o, unsigned short kl, unsigned int vl );
    DecodedHeader( std::string const& o, unsigned short kl, unsigned int vl );
    DecodedHeader( RequestHeader const& rh );

    unsigned int mValueLength;
    unsigned short mKeyLength;
    Opcode mOpcode;
};

struct RequestHeader
{
    static constexpr std::array< char, 8 > MAGIC{ '\x55', '\x35', '\xec', '\xaf', '\x9c', '\x9a', '\x7b', '\xe2' };
    static constexpr std::array< char, 8 > INS{ ToArray( "INSERT  " ) };
    static constexpr std::array< char, 8 > UPD{ ToArray( "UPDATE  " ) };
    static constexpr std::array< char, 8 > DEL{ ToArray( "DELETE  " ) };
    static constexpr std::array< char, 8 > GET{ ToArray( "GET     " ) };

    RequestHeader( DecodedHeader const& h );

    std::array< char, 8 > mHeader; // 0x5535ecaf9c9a7be2 - magic start request sequence
    std::array< char, 8 > mOpcode; // "INSERT  ", "UPDATE  ", "DELETE  ", "GET     "
    std::array< char, 8 > mKeyLength; // 1..1024
    std::array< char, 8 > mValueLength; // 1..1048576
};

struct RequestFooter
{
    static constexpr std::array< char, 8 > MAGIC{ '\x1e', '\x1e', '\xf7', '\x91', '\xe9', '\x5e', '\xff', '\x53' };

    RequestFooter();

    std::array< char, 8 > mFooter; // 1e1ef791e95eff53 - magic end request sequence
};

} // namespace network
