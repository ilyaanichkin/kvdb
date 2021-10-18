#include "kvdb_data_models.hpp"
#include <algorithm>

namespace network
{

constexpr long DecodedHeader::MAX_KEY_SIZE;
constexpr long DecodedHeader::MAX_VALUE_SIZE;

constexpr std::array< char, 8 > RequestHeader::MAGIC;
constexpr std::array< char, 8 > RequestHeader::INS;
constexpr std::array< char, 8 > RequestHeader::UPD;
constexpr std::array< char, 8 > RequestHeader::DEL;
constexpr std::array< char, 8 > RequestHeader::GET;

constexpr std::array< char, 8 > RequestFooter::MAGIC;

DecodedHeader::DecodedHeader( Opcode o, unsigned short kl, unsigned int vl )
    : mValueLength{ vl }
    , mKeyLength{ kl }
    , mOpcode{ Opcode::opInvalid }
{
}

DecodedHeader::DecodedHeader( std::string const& o, unsigned short kl, unsigned int vl )
    : mValueLength{ vl }
    , mKeyLength{ kl }
    , mOpcode{ Opcode::opInvalid }
{
    std::array< char, 8 > oo;
    for( size_t i = 0; i < 8 && i < o.length(); i++ )
        oo[ i ] = o [ i ];
    if( oo == RequestHeader::INS )
        mOpcode = Opcode::opInsert;
    else if( oo == RequestHeader::UPD )
        mOpcode = Opcode::opUpdate;
    else if( oo == RequestHeader::DEL )
        mOpcode = Opcode::opDelete;
    else if( oo == RequestHeader::GET )
        mOpcode = Opcode::opGet;
}

DecodedHeader::DecodedHeader( RequestHeader const& h )
    : mValueLength{ 0 }
    , mKeyLength{ 0 }
    , mOpcode{ Opcode::opInvalid }
{
    if( h.mHeader != RequestHeader::MAGIC )
        return;

    long kl = std::stol( std::string( h.mKeyLength.begin(), h.mKeyLength.end() ) );
    if( kl < 1 || kl > MAX_KEY_SIZE )
        return;

    long vl = std::stol( std::string( h.mValueLength.begin(), h.mValueLength.end() ) );
    if( vl < 0 || vl > MAX_VALUE_SIZE )
        return;

    if( h.mOpcode == RequestHeader::INS )
        mOpcode = Opcode::opInsert;
    else if( h.mOpcode == RequestHeader::UPD )
        mOpcode = Opcode::opUpdate;
    else if( h.mOpcode == RequestHeader::DEL )
        mOpcode = Opcode::opDelete;
    else if( h.mOpcode == RequestHeader::GET )
        mOpcode = Opcode::opGet;
    else
        return;

    mValueLength = vl;
    mKeyLength = kl;
}

RequestHeader::RequestHeader( DecodedHeader const& h )
    : mHeader( MAGIC )
    , mOpcode( MAGIC )
    , mKeyLength( { '\0','\0','\0','\0','\0','\0','\0','\0' } )
    , mValueLength( { '\0','\0','\0','\0','\0','\0','\0','\0' } )
{
    if( h.mKeyLength < 1 || h.mKeyLength > 1024 )
        return;
    if( h.mValueLength < 0 || h.mValueLength > 1048576 )
        return;
    if( h.mOpcode == Opcode::opInsert )
        mOpcode = INS;
    else if( h.mOpcode == Opcode::opUpdate )
        mOpcode = UPD;
    else if( h.mOpcode == Opcode::opDelete )
        mOpcode = DEL;
    else if( h.mOpcode == Opcode::opGet )
        mOpcode = GET;
    else
        return;
    std::string kl = std::to_string( h.mKeyLength );
    std::string vl = std::to_string( h.mValueLength );
    std::transform( kl.begin(), kl.end(), mKeyLength.begin(), []( unsigned char c ) -> unsigned char { return c; } );
    std::transform( vl.begin(), vl.end(), mValueLength.begin(), []( unsigned char c ) -> unsigned char { return c; } );
}

RequestFooter::RequestFooter()
    : mFooter( MAGIC )
{
}

} // namespace network
