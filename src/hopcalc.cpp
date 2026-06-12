# include "../hopcalc.hpp"

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
{
  o->insert( o->end(), (const char*)p, (const char*)p + l );
  return o;
}

namespace hopcalc
{

  enum: unsigned
  {
    hc_none = 0,

    hc_const = 1,
    hc_neg = 2,
    hc_var = 3,

    hc_mul = 4,
    hc_div = 5,
    hc_mod = 6,

    hc_add = 7,
    hc_sub = 8,

    hc_shl = 9,
    hc_shr = 10,

    hc_lt = 11,
    hc_le = 12,
    hc_gt = 13,
    hc_ge = 14,

    hc_eq = 15,
    hc_ne = 16,

    hc_b_and = 17,
    hc_b_xor = 18,
    hc_b_or = 19,

    hc_l_and = 30,
    hc_l_or = 21,

    hc_colon = 22,
    hc_quest = 23,

    hc_assign = 24,

    hc_skip = 99
  };

  static const struct
  {
    const char* name;
    unsigned    code;
  } operators[] =
  {
    { "*",  hc_mul   },
    { "/",  hc_div   },
    { "%",  hc_mod   },
    { "+",  hc_add   },
    { "-",  hc_sub   },
    { "<<", hc_shl   },
    { ">>", hc_shr   },
    { "<",  hc_lt    },
    { "<=", hc_le    },
    { ">",  hc_gt    },
    { ">=", hc_ge    },
    { "==", hc_eq    },
    { "!=", hc_ne    },
    { "&",  hc_b_and },
    { "^",  hc_b_xor },
    { "|",  hc_b_or  },
    { "&&", hc_l_and },
    { "||", hc_l_or  },
    { ":", hc_colon  },
    { "?", hc_quest  },
    { "=", hc_assign },
  };

  auto  ExprLen( const char* pbeg, const char* pend ) -> size_t;

  auto  NoSpace( const char* pbeg, const char* pend ) -> const char*
  {
    while ( pbeg != pend && (unsigned char)*pbeg <= 0x20 )
      ++pbeg;
    return pbeg;
  }

  template <char C>
  auto  ToBrace( const char* pbeg, const char* pend ) -> const char*
  {
    auto  base( pbeg );

    for ( pbeg = NoSpace( pbeg, pend ); pbeg != pend && *pbeg != C; )
      pbeg = NoSpace( pbeg + ExprLen( pbeg, pend ), pend );

    return pbeg != pend ? pbeg :
      throw SyntaxError( mtc::strprintf( "Unexpected end of string while looking for '%c'", C ), pbeg - base );
  }

  template <char C>
  auto  ToQuote( const char* pbeg, const char* pend ) -> const char*
  {
    auto  base( pbeg );

    while ( pbeg != pend )
    {
      if ( *pbeg == '\\' ) [[unlikely]]
      {
        if ( ++pbeg == pend )
          break;
      }
        else
      if ( *pbeg == C )
      {
        return pbeg;
      }
      ++pbeg;
    }

    throw SyntaxError( mtc::strprintf( "Unexpected end of string while looking for '%c'", C ), pbeg - base );
  }

  auto  ExprLen( const char* pbeg, const char* pend ) -> size_t
  {
    static
    char    delims[] = "()[]\"\'+-*/%,";
    auto    origin( pbeg );
    char    chnext;

    if ( pbeg >= pend )
      throw SyntaxError( "unexpected end of string", pbeg - origin );

    if ( (unsigned char)*pbeg <= 0x20 )
      throw SyntaxError( "unexpected space-started string", pbeg - origin );

    try
    {
      switch ( chnext = *pbeg++ )
      {
        case '(':
          return ToBrace<')'>( pbeg, pend ) + 1 - origin;
        case '[':
          return ToBrace<']'>( pbeg, pend ) + 1 - origin;
        case '\"':
          return ToQuote<'\"'>( pbeg, pend ) + 1 - origin;;
        case '\'':
          return ToQuote<'\''>( pbeg, pend ) + 1 - origin;;
        case '+':
        case '-':
        case '/':
        case '%':
        case ',':
          return pbeg - origin;
        default:
          while ( pbeg != pend && (unsigned char)*pbeg > 0x20 && memchr( delims, *pbeg, sizeof(delims) - 1 ) == nullptr )
            ++pbeg;
          return pbeg - origin;
      }
    }
    catch ( const SyntaxError& e )
    {
      throw SyntaxError( e.what(), e.GetOffset() + pbeg - origin );
    }
  }

  auto  GetCode( const char* pbeg, size_t ncch ) -> unsigned
  {
    for ( auto& next: operators )
      if ( next.name[ncch] == 0 && strncmp( pbeg, next.name, ncch ) == 0 )
        return next.code;
    return 0;
  }

  auto  Compile( const char* pbeg, const char* pend, const char* orig ) -> std::vector<char>
  {
    auto        output = std::vector<char>();
    auto        origin = NoSpace( pbeg, pend );
    const char* divbeg = nullptr;
    unsigned    divlen;
    unsigned    opcode = 0;
    unsigned    opnext;
    size_t      explen;

    if ( orig == nullptr )
      orig = pbeg;

  // skip ending spaces
    while ( pend != origin && (unsigned char)pend[-1] <= 0x20 )
      --pend;

    if ( pbeg >= pend )
      throw SyntaxError( "empty expression", pbeg - orig );

  // check if expression covers the whole string
    if ( (pbeg = origin + (explen = ExprLen( pbeg, pend ))) == pend )
    {
      double  dvalue;
      char*   endptr;

      if ( *origin == '(' && pend[-1] == ')' )
        return Compile( NoSpace( origin + 1, pend - 1 ), pend - 1, orig );

      if ( *origin == '[' && pend[-1] == ']' )
        return Compile( NoSpace( origin + 1, pend - 1 ), pend - 1, orig );

      if ( (*origin == '\"' || *origin == '\'') && pend[-1] == *origin )
        return std::move( *mtc::zval( origin + 1, pend - origin - 2 ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

      if ( pend - origin == 4 && memcmp( origin, "true", 4 ) == 0 )
        return std::move( *mtc::zval( true ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

      if ( pend - origin == 5 && memcmp( origin, "false", 5 ) == 0 )
        return std::move( *mtc::zval( false ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

      if ( *origin == '$' )
      {
        return std::move( *::Serialize( Serialize( &output, uint8_t(hc_var) ),
          std::string_view( origin + 1, pend - origin - 1 ) ) );
      }

      dvalue = mtc::w_strtod( origin, pend, &endptr );

      if ( auto tok = NoSpace( endptr, pend ); tok != pend )
        throw SyntaxError( "unexpected token", tok - endptr );

      if ( std::abs(dvalue - std::trunc(dvalue)) < std::numeric_limits<double>::epsilon() * std::abs(dvalue) )
      {
        if ( dvalue >= 0 )
        {
          if ( dvalue <= std::numeric_limits<uint8_t>::max() )
            return std::move( *mtc::zval( uint8_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue <= std::numeric_limits<uint16_t>::max() )
            return std::move( *mtc::zval( uint16_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue <= std::numeric_limits<uint32_t>::max() )
            return std::move( *mtc::zval( uint32_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue <= std::numeric_limits<uint64_t>::max() )
            return std::move( *mtc::zval( uint64_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
        }
          else
        {
          if ( dvalue >= std::numeric_limits<int8_t>::min() )
            return std::move( *mtc::zval( int8_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue >= std::numeric_limits<int16_t>::min() )
            return std::move( *mtc::zval( int16_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue >= std::numeric_limits<int32_t>::min() )
            return std::move( *mtc::zval( int32_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          if ( dvalue >= std::numeric_limits<int64_t>::min() )
            return std::move( *mtc::zval( int64_t(dvalue) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
        }
      }
      return std::move( *mtc::zval( dvalue ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
    }

  // search division point with binary operator
    bool  waitOp = true;

    for ( pbeg = NoSpace( pbeg, pend ); pbeg < pend; pbeg = NoSpace( pbeg + explen, pend ) )
    {
      explen = ExprLen( pbeg, pend );

      if ( !waitOp && explen == 1 && (*pbeg == '-' || *pbeg == '+') )
      {
        waitOp = false;
        continue;
      }

      if ( (opnext = GetCode( pbeg, explen )) == 0 )
      {
        waitOp = true;
        continue;
      }

      if ( opnext >= opcode )
      {
        divbeg = pbeg;
        divlen = explen;
        opcode = opnext;
      }
      waitOp = false;
    }

  // check if divided
    if ( divbeg != nullptr )
    {
      auto  lexp = Compile( origin, divbeg, orig );
      auto  rexp = Compile( NoSpace( divbeg + divlen, pend ), pend, orig );

      if ( opcode != hc_colon )
        output.push_back( opcode );

      output.insert( output.end(), lexp.begin(), lexp.end() );
      output.insert( output.end(), rexp.begin(), rexp.end() );

      return output;
    }

    if ( *origin == '-' || *origin == '+' )
    {
      auto  code = Compile( NoSpace( origin + 1, pend ), pend, orig );

      if ( *origin == '-' )
        output.push_back( hc_neg );

      output.insert( output.end(), code.begin(), code.end() );
      return output;
    }
    throw SyntaxError( "unexpected end of string", pbeg - orig );
  }

  const char*  JumpOver( const char* pbeg, const char* pend )
  {
    if ( pbeg < pend )
    {
      switch ( *pbeg++ )
      {
        case hc_const:
          return mtc::zval::SkipToEnd( pbeg );
        case hc_var:
          return SkipToEnd( pbeg, (const std::string*)nullptr );
        case hc_neg:
          return JumpOver( pbeg, pend );
        case hc_mul:  case hc_div:    case hc_mod:
        case hc_add:  case hc_sub:    case hc_shl:
        case hc_shr:  case hc_b_and:  case hc_b_xor:
        case hc_b_or: case hc_l_and:  case hc_l_or:
        case hc_lt:   case hc_le:     case hc_gt:
        case hc_ge:   case hc_eq:     case hc_ne:
          return JumpOver( JumpOver( pbeg, pend ), pend );
        case hc_quest:
          return JumpOver( JumpOver( JumpOver( pbeg, pend ), pend ), pend );
        default:
          throw std::runtime_error( "operator not supported in JumpOver" );
      }
    }
    return pbeg;
  }

  struct StackEntry
  {
    unsigned    opcode;   // 0 - парсить байт-код; иначе - выполнить оператор
    mtc::zval*  target;   // Куда положить результат

  alignas(mtc::zval)
    char        zspace[sizeof(mtc::zval)];   // Временный правый аргумент для бинарных операций

    auto  init( unsigned op, mtc::zval* to ) -> StackEntry&
      {  return opcode = op, target = to, *this;  }
    auto  zval() const -> const mtc::zval&
      {  return *(const mtc::zval*)zspace;  }
    auto  zval() -> mtc::zval&
      {  return *(mtc::zval*)zspace;  }
  };

  class ClearStack
  {
    StackEntry* stack;
    int&        depth;

  public:
    ClearStack( StackEntry* s, int& d ): stack( s ), depth( d )
      {}
   ~ClearStack()
      {
        while ( depth > 0 )
          ((mtc::zval*)stack[--depth].zspace)->~zval();
      }
  };

  auto  Evaluate( const char* pbeg, const char* pend, IUserData* func ) -> mtc::zval
  {
    using string_view = std::string_view;

    static mtc::zval zero( 0 );
    constexpr int MAX_STACK = 32;

    mtc::zval   result;
    StackEntry  zstack[MAX_STACK];
    int         pstack = 0;
    int         zdepth = 0;
    ClearStack  zclear( zstack, zdepth );

    string_view assign[MAX_STACK];
    int         assptr = -1;

    zstack[0].init( hc_none, &result );

    while ( pstack >= 0 )
    {
      if ( pstack + 1 >= MAX_STACK )
        throw std::runtime_error("stack overflow");

      while ( zdepth < pstack )
        new( zstack[zdepth++].zspace ) mtc::zval();

      if ( zstack[pstack].opcode == hc_none )
      {
        auto& cstack = zstack[pstack];

        unsigned  opcode;
        size_t    length;

        if ( pbeg < pend )  opcode = *pbeg++;
          else throw std::runtime_error( "unexpected end of bytecode" );

        switch ( opcode )
        {
          case hc_const:
            pbeg = ::FetchFrom( pbeg, *cstack.target );
              --pstack;
            break;

          case hc_var:
            pbeg = ::FetchFrom( pbeg, length );
            *cstack.target = std::move( func->Get( { pbeg, length } ) );
            pbeg += length;
            --pstack;
            break;

          case hc_neg:
            cstack.opcode = opcode;
            zstack[++pstack].init(  hc_none, cstack.target );
            break;

          case hc_mul:  case hc_div:    case hc_mod:
          case hc_add:  case hc_sub:    case hc_shl:
          case hc_shr:  case hc_b_and:  case hc_b_xor:
          case hc_b_or: case hc_lt:     case hc_le:
          case hc_gt:   case hc_ge:     case hc_eq:
          case hc_ne:
            cstack.opcode = opcode;
            zstack[++pstack].init( hc_none, &cstack.zval() );
            zstack[++pstack].init( hc_none,  cstack.target );
            break;

          case hc_l_and:
          case hc_l_or:
          case hc_quest:
            cstack.opcode = opcode;
            zstack[++pstack].init( hc_none, cstack.target );
            break;

          case hc_assign:
            if ( *pbeg++ != hc_var )
              throw std::runtime_error( "l-value required as left operand" );

            zstack[++pstack].init( hc_none, &cstack.zval() );

            pbeg = ::FetchFrom( pbeg, length );
              assign[++assptr] = { pbeg, length };
            cstack.opcode = hc_assign;
              pbeg += length;
            break;

          default:
            throw std::runtime_error( "operator not supported" );
        }
      }
        else
      {
        auto& cstack = zstack[pstack--];

        switch ( cstack.opcode )
        {
          case hc_assign:
            func->Set( assign[assptr], *cstack.target = std::move( cstack.zval() ) );
            --assptr;
            break;

          case hc_neg:
            *cstack.target = zero - *cstack.target;
            break;

          case hc_l_and:
            if ( cstack.target->eq( 0 ) ) pbeg = JumpOver( pbeg, pend );
              else zstack[++pstack].init( hc_none, cstack.target );
            break;

          case hc_l_or:
            if ( cstack.target->ne( 0 ) )  pbeg = JumpOver( pbeg, pend );
              else zstack[++pstack].init( hc_none, cstack.target );
            break;

          case hc_quest:
            if ( cstack.target->ne( 0 ) )
            {
              zstack[++pstack].init( hc_skip, cstack.target );
              zstack[++pstack].init( hc_none, cstack.target );
            }
              else
            {
              pbeg = JumpOver( pbeg, pend );
              zstack[++pstack].init( hc_none, cstack.target );
            }
            break;

          case hc_skip:
            pbeg = JumpOver( pbeg, pend );
            break;

          // Математика и сравнения
          case hc_mul:    *cstack.target *=  cstack.zval(); break;
          case hc_div:    *cstack.target /=  cstack.zval(); break;
          case hc_mod:    *cstack.target %=  cstack.zval(); break;
          case hc_add:    *cstack.target +=  cstack.zval(); break;
          case hc_sub:    *cstack.target -=  cstack.zval(); break;
          case hc_shl:    *cstack.target <<= cstack.zval(); break;
          case hc_shr:    *cstack.target >>= cstack.zval(); break;
          case hc_b_and:  *cstack.target &=  cstack.zval(); break;
          case hc_b_xor:  *cstack.target ^=  cstack.zval(); break;
          case hc_b_or:   *cstack.target |=  cstack.zval(); break;

          case hc_lt:     *cstack.target = cstack.target->lt( cstack.zval() ); break;
          case hc_le:     *cstack.target = cstack.target->le( cstack.zval() ); break;
          case hc_gt:     *cstack.target = cstack.target->gt( cstack.zval() ); break;
          case hc_ge:     *cstack.target = cstack.target->ge( cstack.zval() ); break;
          case hc_eq:     *cstack.target = cstack.target->eq( cstack.zval() ); break;
          case hc_ne:     *cstack.target = cstack.target->ne( cstack.zval() ); break;
          default:        throw std::runtime_error( "unknown operator" );
        }
      }
    }

    return result;
  }

  # if 0
  const char*  Evaluate( mtc::zval& zout, const char* pbeg, const char* pend, IUserData* func )
  {
    static mtc::zval zero( 0 );

    while ( pbeg < pend )
    {
      auto      opcode = *pbeg++;
      size_t    length;

      switch ( opcode )
      {
        case hc_const:
          return ::FetchFrom( pbeg, zout );
        case hc_neg:
          pbeg = Evaluate( zout, pbeg, pend, func );
            zout = zero - zout;
          return pbeg;
        case hc_var:
          {
            pbeg = ::FetchFrom( pbeg, length );
              zout = std::move( func->Get( pbeg, length ) );
            return pbeg + length;
          }
        case hc_mul:  case hc_div:    case hc_mod:
        case hc_add:  case hc_sub:    case hc_shl:
        case hc_shr:  case hc_b_and:  case hc_b_xor:
        case hc_b_or: case hc_lt:     case hc_le:
        case hc_gt:   case hc_ge:     case hc_eq:
        case hc_ne:
          {
            mtc::zval zarg;

            pbeg = Evaluate( zout, pbeg, pend, func );
            pbeg = Evaluate( zarg, pbeg, pend, func );

            switch ( opcode )
            {
              case hc_mul:    return zout *= zarg, pbeg;
              case hc_div:    return zout /= zarg, pbeg;
              case hc_mod:    return zout %= zarg, pbeg;
              case hc_add:    return zout += zarg, pbeg;
              case hc_sub:    return zout -= zarg, pbeg;
              case hc_shl:    return zout <<= zarg, pbeg;
              case hc_shr:    return zout >>= zarg, pbeg;
              case hc_b_and:  return zout &= zarg, pbeg;
              case hc_b_xor:  return zout ^= zarg, pbeg;
              case hc_b_or:   return zout |= zarg, pbeg;
              case hc_lt:     return zout = zout.lt( zarg ), pbeg;
              case hc_le:     return zout = zout.le( zarg ), pbeg;
              case hc_gt:     return zout = zout.gt( zarg ), pbeg;
              case hc_ge:     return zout = zout.ge( zarg ), pbeg;
              case hc_eq:     return zout = zout.eq( zarg ), pbeg;
              default:/*ne*/  return zout = zout.ne( zarg ), pbeg;
            }
          }
        case hc_l_and:
          {
            pbeg = Evaluate( zout, pbeg, pend, func );

            return zout.eq( 0 ) ? JumpOver( pbeg, pend ) : Evaluate( zout, pbeg, pend, func );
          }
        case hc_l_or:
          {
            pbeg = Evaluate( zout, pbeg, pend, func );

            return zout.ne( 0 ) ? JumpOver( pbeg, pend ) : Evaluate( zout, pbeg, pend, func );
          }
        case hc_quest:
          {
            pbeg = Evaluate( zout, pbeg, pend, func );

            return zout.ne( 0 ) ?
              JumpOver( Evaluate( zout, pbeg, pend, func ), pend ) :
              Evaluate( zout, JumpOver( pbeg, pend ), pend, func );
          }
        default:
          throw std::runtime_error( "operator not supported" );
      }
    }
    return pbeg;
  }

  mtc::zval   Evaluate( const char* pbeg, const char* pend, IUserData* func )
  {
    mtc::zval result;

    return Evaluate( result, pbeg, pend, func ), result;
  }
  # endif

}
