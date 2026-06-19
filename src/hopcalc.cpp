# include "../hopcalc.hpp"
# include <cmath>

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

    hc_in = 17,

    hc_b_and = 18,
    hc_b_xor = 19,
    hc_b_or =  20,

    hc_l_and = 30,
    hc_l_or = 31,

    hc_colon = 32,
    hc_quest = 33,

    hc_assign = 34,

    hc_skip = 99,

    hc_abs   = 200,
    hc_floor = 201,
    hc_ceil  = 202,
    hc_round = 203,
    hc_acos  = 204,
    hc_asin  = 205,
    hc_atan  = 206,
    hc_cos   = 207,
    hc_cosh  = 208,
    hc_sin   = 209,
    hc_sinh  = 210,
    hc_tan   = 211,
    hc_tanh  = 212,
    hc_exp   = 213,
    hc_log   = 214,
    hc_log2  = 215,
    hc_log10 = 216,
    hc_pow   = 217,
    hc_sqrt  = 218,
    hc_pi    = 219
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
    { "in", hc_in    },
    { "&",  hc_b_and },
    { "^",  hc_b_xor },
    { "|",  hc_b_or  },
    { "&&", hc_l_and },
    { "||", hc_l_or  },
    { ":", hc_colon  },
    { "?", hc_quest  },
    { "=", hc_assign },
  };

  static const struct FuncDef
  {
    const char* name;
    unsigned    args;
    unsigned    code;
  } functions[] =
  {
    { "abs",   1, hc_abs   },
    { "floor", 1, hc_floor },
    { "ceil",  1, hc_ceil  },
    { "round", 1, hc_round },
    { "acos",  1, hc_acos  },
    { "asin",  1, hc_asin  },
    { "atan",  1, hc_atan  },
    { "cos",   1, hc_cos   },
    { "cosh",  1, hc_cosh  },
    { "sin",   1, hc_sin   },
    { "sinh",  1, hc_sinh  },
    { "tan",   1, hc_tan   },
    { "tanh",  1, hc_tanh  },
    { "exp",   1, hc_exp   },
    { "log",   1, hc_log   },
    { "ln",    1, hc_log   },
    { "log2",  1, hc_log2  },
    { "log10", 1, hc_log10 },
    { "pi",    0, hc_pi    },
    { "pow",   2, hc_pow   },
    { "sqrt",  1, hc_sqrt  }
  };

  static mtc::zval zero( 0 );

  auto  ExprLen( const char* pbeg, const char* pend ) -> size_t;

  auto  NoSpace( const char* pbeg, const char* pend ) -> const char*
  {
    while ( pbeg != pend && std::isspace( static_cast<unsigned char>( *pbeg ) ) )
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
      return 0;

    if ( std::isspace( static_cast<unsigned char>( *pbeg ) ) )
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

  auto  GetFunc( const char* pbeg, size_t ncch ) -> const FuncDef*
  {
    for ( auto& next: functions )
      if ( next.name[ncch] == 0 && strncmp( pbeg, next.name, ncch ) == 0 )
        return &next;
    return nullptr;
  }

  auto  AsArray( const char* pbeg, const char* pend, const char* orig ) -> mtc::zval;

  auto  AsValue( const char* pbeg, const char* pend, const char* orig ) -> mtc::zval
  {
    double  dvalue;
    char*   endptr;

    if ( *pbeg == '[' && pend[-1] == ']' )
      return AsArray( NoSpace( pbeg + 1, pend - 1 ), pend - 1, orig );

    if ( (*pbeg == '\"' || *pbeg == '\'') && pend[-1] == *pbeg )
      return mtc::zval( pbeg + 1, pend - pbeg - 2 );

    if ( pend - pbeg == 4 && strncasecmp( pbeg, "true", 4 ) == 0 )
      return mtc::zval( true );

    if ( pend - pbeg == 5 && strncasecmp( pbeg, "false", 5 ) == 0 )
      return mtc::zval( false );

    dvalue = mtc::w_strtod( pbeg, pend, &endptr );

    if ( auto tok = NoSpace( endptr, pend ); tok != pend )
      throw SyntaxError( "unexpected token", tok - orig );

    if ( std::abs(dvalue - std::trunc(dvalue)) < std::numeric_limits<double>::epsilon() * std::abs(dvalue) )
    {
      if ( dvalue >= 0 )
      {
        if ( dvalue <= std::numeric_limits<uint8_t>::max() )
          return mtc::zval( uint8_t(dvalue) );

        if ( dvalue <= std::numeric_limits<uint16_t>::max() )
          return mtc::zval( uint16_t(dvalue) );

        if ( dvalue <= std::numeric_limits<uint32_t>::max() )
          return mtc::zval( uint32_t(dvalue) );

        if ( dvalue <= std::numeric_limits<uint64_t>::max() )
          return mtc::zval( uint64_t(dvalue) );
      }
        else
      {
        if ( dvalue >= std::numeric_limits<int8_t>::min() )
          return mtc::zval( int8_t(dvalue) );

        if ( dvalue >= std::numeric_limits<int16_t>::min() )
          return mtc::zval( int16_t(dvalue) );

        if ( dvalue >= std::numeric_limits<int32_t>::min() )
          return mtc::zval( int32_t(dvalue) );

        if ( dvalue >= std::numeric_limits<int64_t>::min() )
          return mtc::zval( int64_t(dvalue) );
      }
    }
    return mtc::zval( dvalue );
  }

  auto  AsArray( const char* pbeg, const char* pend, const char* orig ) -> mtc::zval
  {
    mtc::array_zval zarray;

    for ( pbeg = NoSpace( pbeg, pend ); pend > pbeg && std::isspace( static_cast<unsigned char>( pend[-1] ) ); --pend )
      (void)NULL;

    for ( bool getVal = true; pbeg != pend; )
    {
      auto  explen = ExprLen( pbeg, pend );

      if ( explen == 1 && *pbeg == ',' )    // if comma...
      {
        if ( getVal )                       // ... and value expected, throw exception,
          throw SyntaxError( "unexpected ','", pbeg - orig );
        getVal = true;                      // else expect value
      }
        else
      if ( getVal )                         // else, if not a comma, and expect value, insert it
      {
        zarray.push_back( AsValue( pbeg, pbeg + explen, orig ) );
          getVal = false;
      }
        else                                // else, not comma, and not waiting value - expection
      throw SyntaxError( "',' expected", pbeg - orig );

      pbeg = NoSpace( pbeg + explen, pend );
    }
    return zarray;
  }

  inline  bool  IsConstant( const std::vector<char>& v )  {  return v.size() > 0 && v[0] == hc_const;  }
  inline  bool  IsVariable( const std::vector<char>& v )  {  return v.size() > 0 && v[0] == hc_var;  }

  template <class U>
  bool  in( const mtc::zval&, const U& );

  template <class U>
  bool  in( const mtc::zval& t, const std::vector<U>& u )
  {
    return std::find_if( u.begin(), u.end(), [&]( const U& u ) {  return t.eq( u );  } ) != u.end();
  }

  template <class C>
  bool  in( const mtc::zval& t, const std::basic_string<C>& u )
  {
    switch ( t.get_type() )
    {
      case mtc::zval::z_char:
        return u.find( *t.get_char() ) != mtc::charstr::npos;
      case mtc::zval::z_byte:
        return u.find( *t.get_byte() ) != mtc::charstr::npos;
      case mtc::zval::z_charstr:
        return mtc::w_strstr( u.c_str(), t.get_charstr()->c_str() ) != nullptr;
      case mtc::zval::z_widestr:
        return mtc::w_strstr( u.c_str(), t.get_widestr()->c_str() ) != nullptr;
      default:
        return false;
    }
  }

  bool  in( const mtc::zval& what, const mtc::zval& upset )
  {
    return
      upset.get_type() == mtc::zval::z_charstr      ? in( what, *upset.get_charstr() ) :
      upset.get_type() == mtc::zval::z_widestr      ? in( what, *upset.get_widestr() ) :
      upset.get_type() == mtc::zval::z_array_char   ? in( what, *upset.get_array_char() ) :
      upset.get_type() == mtc::zval::z_array_byte   ? in( what, *upset.get_array_byte() ) :
      upset.get_type() == mtc::zval::z_array_int16  ? in( what, *upset.get_array_int16() ) :
      upset.get_type() == mtc::zval::z_array_word16 ? in( what, *upset.get_array_word16() ) :
      upset.get_type() == mtc::zval::z_array_int32  ? in( what, *upset.get_array_int32() ) :
      upset.get_type() == mtc::zval::z_array_word32 ? in( what, *upset.get_array_word32() ) :
      upset.get_type() == mtc::zval::z_array_int64  ? in( what, *upset.get_array_int64() ) :
      upset.get_type() == mtc::zval::z_array_word64 ? in( what, *upset.get_array_word64() ) :
      upset.get_type() == mtc::zval::z_array_float  ? in( what, *upset.get_array_float() ) :
      upset.get_type() == mtc::zval::z_array_double ? in( what, *upset.get_array_double() ) :
      upset.get_type() == mtc::zval::z_array_charstr ? in( what, *upset.get_array_charstr() ) :
      upset.get_type() == mtc::zval::z_array_widestr ? in( what, *upset.get_array_widestr() ) :
      upset.get_type() == mtc::zval::z_array_zval   ? in( what, *upset.get_array_zval() ) : what.eq( upset );
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
    while ( pend != origin && std::isspace( static_cast<unsigned char>( pend[-1] ) ) )
      --pend;

    if ( pbeg >= pend )
      throw SyntaxError( "empty expression", pbeg - orig );

  // check if expression covers the whole string
    if ( (pbeg = origin + (explen = ExprLen( pbeg, pend ))) == pend )
    {
      if ( *origin == '(' && pend[-1] == ')' )
        return Compile( NoSpace( origin + 1, pend - 1 ), pend - 1, orig );

      if ( *origin == '$' )
      {
        return std::move( *::Serialize( Serialize( &output, uint8_t(hc_var) ),
          std::string_view( origin + 1, pend - origin - 1 ) ) );
      }

      return std::move( *AsValue( origin, pbeg, orig ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
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
        waitOp = GetFunc( pbeg, explen ) == 0;
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

    // выполнить предвычисление значений, если возможно
      if ( IsConstant( lexp ) && IsConstant( rexp ) )
      {
        mtc::zval lval;
        mtc::zval rval;

        lval.FetchFrom( (const char*)lexp.data() + 1 );
        rval.FetchFrom( (const char*)rexp.data() + 1 );

        switch ( opcode )
        {
          case hc_mul:  return std::move( *(lval *= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_div:  return std::move( *(lval /= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_mod:  return std::move( *(lval %= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_add:  return std::move( *(lval += rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_sub:  return std::move( *(lval -= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_shl:  return std::move( *(lval <<= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_shr:  return std::move( *(lval >>= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          case hc_lt:   return std::move( *mtc::zval( lval.lt( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_le:   return std::move( *mtc::zval( lval.le( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_gt:   return std::move( *mtc::zval( lval.gt( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_ge:   return std::move( *mtc::zval( lval.ge( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          case hc_eq:   return std::move( *mtc::zval( lval.eq( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_ne:   return std::move( *mtc::zval( lval.ne( rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          case hc_in:   return std::move( *mtc::zval( in( lval, rval ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          case hc_b_and:return std::move( *(lval &= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_b_xor:return std::move( *(lval ^= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_b_or: return std::move( *(lval |= rval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );

          case hc_l_and:return std::move( *mtc::zval( lval.ne( 0 ) && rval.ne( 0 ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          case hc_l_or: return std::move( *mtc::zval( lval.ne( 0 ) || rval.ne( 0 ) ).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
          default:      break;
        }
      }

      if ( opcode != hc_colon )
        output.push_back( opcode );

      output.insert( output.end(), lexp.begin(), lexp.end() );
      output.insert( output.end(), rexp.begin(), rexp.end() );

      return output;
    }

    if ( *origin == '-' || *origin == '+' )
    {
      auto  avalue = Compile( NoSpace( origin + 1, pend ), pend, orig );

      if ( *origin == '-' && IsConstant( avalue ) )
      {
        mtc::zval zval;

        zval.FetchFrom( (const char*)avalue.data() + 1 );

        if ( zval.is_numeric() )
          return std::move( *(zero - zval).Serialize( ::Serialize( &output, uint8_t(hc_const) ) ) );
      }

      output.insert( output.begin(), hc_neg );
      output.insert( output.end(), avalue.begin(), avalue.end() );
      return output;
    }

    if ( auto pfndef = GetFunc( origin, explen = ExprLen( origin, pend ) ); pfndef != nullptr )
    {
      auto  lbrace = NoSpace( origin + explen, pend );
      auto  acount = unsigned(0);

      if ( lbrace == pend || *lbrace != '(' )
        throw SyntaxError( "'(' expected", lbrace - orig );

      if ( pend[-1] != ')' )
        throw SyntaxError( "no closing ')' found", pend - orig );

      output.push_back( pfndef->code );

    // get expression bounds in braces
      auto  argorg = NoSpace( lbrace + 1, pend );
      auto  argbeg = argorg;
      auto  argend = pend - 1;
      auto  astart = argbeg;
      auto  AddArg = [&]( const char* beg, const char* end )
        {
          auto  subexp = Compile( beg, end, orig );
            output.insert( output.end(), subexp.begin(), subexp.end() );
          if ( ++acount > pfndef->args )
            throw SyntaxError( "too many arguments", beg - orig );
        };

      while ( argbeg < argend && std::isspace( static_cast<unsigned char>( argend[-1] ) ) )
        --argend;

    // get the arguments
      for ( ; ; )
      {
      // check if end of expression
        if ( astart == argend )
        {
          if ( astart != argbeg )
            AddArg( argbeg, astart );
          break;
        }
          else
      // check if comma; it must not be the first at the list of arguments
        if ( (explen = ExprLen( astart, argend )) == 1 && *astart == ',' )
        {
          if ( astart != argbeg ) AddArg( argbeg, astart );
            else throw SyntaxError( "unexpected ','", astart - orig );
          astart = argbeg = NoSpace( astart + 1, pend );
        }
          else
        astart = NoSpace( astart + explen, argend );
      }

      if ( acount < pfndef->args )
        throw SyntaxError( "not enough arguments", astart - orig );

      return output;
    }
    throw SyntaxError( "unexpected end of string", pbeg - orig );
  }

  const char*  JumpOver( const char* pbeg, const char* pend )
  {
    if ( pbeg < pend )
    {
      switch ( (unsigned char)*pbeg++ )
      {
        case hc_const:
          return mtc::zval::SkipToEnd( pbeg );

        case hc_var:
        case hc_assign:
          return SkipToEnd( pbeg, (const std::string*)nullptr );
        case hc_neg:    case hc_abs:    case hc_floor:
        case hc_ceil:   case hc_round:  case hc_acos:
        case hc_asin:   case hc_atan:   case hc_cos:
        case hc_cosh:   case hc_sin:    case hc_sinh:
        case hc_tan:    case hc_tanh:   case hc_exp:
        case hc_log:    case hc_log2:   case hc_log10:
        case hc_sqrt:
          return JumpOver( pbeg, pend );
        case hc_mul:  case hc_div:    case hc_mod:
        case hc_add:  case hc_sub:    case hc_shl:
        case hc_shr:  case hc_b_and:  case hc_b_xor:
        case hc_b_or: case hc_l_and:  case hc_l_or:
        case hc_lt:   case hc_le:     case hc_gt:
        case hc_ge:   case hc_eq:     case hc_ne:
        case hc_in:   case hc_pow:
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

      while ( zdepth <= pstack )
        new( zstack[zdepth++].zspace ) mtc::zval();

      if ( zstack[pstack].opcode == hc_none )
      {
        auto& cstack = zstack[pstack];

        unsigned  opcode;
        size_t    length;

        if ( pbeg < pend )  opcode = (unsigned char)*pbeg++;
          else throw std::runtime_error( "unexpected end of bytecode" );

        switch ( opcode )
        {
          case hc_const:
            pbeg = ::FetchFrom( pbeg, *cstack.target );
              --pstack;
            break;

          case hc_pi:
            *cstack.target = M_PI;
              --pstack;
            break;

          case hc_var:
            pbeg = ::FetchFrom( pbeg, length );
            *cstack.target = std::move( func->Get( { pbeg, length } ) );
            pbeg += length;
            --pstack;
            break;

          // unary operations
          case hc_neg:    case hc_abs:    case hc_floor:
          case hc_ceil:   case hc_round:  case hc_acos:
          case hc_asin:   case hc_atan:   case hc_cos:
          case hc_cosh:   case hc_sin:    case hc_sinh:
          case hc_tan:    case hc_tanh:   case hc_exp:
          case hc_log:    case hc_log2:   case hc_log10:
          case hc_sqrt:
            cstack.opcode = opcode;
            zstack[++pstack].init(  hc_none, cstack.target );
            break;

          // binary operations
          case hc_mul:  case hc_div:    case hc_mod:
          case hc_add:  case hc_sub:    case hc_shl:
          case hc_shr:  case hc_b_and:  case hc_b_xor:
          case hc_b_or: case hc_lt:     case hc_le:
          case hc_gt:   case hc_ge:     case hc_eq:
          case hc_ne:   case hc_in:     case hc_pow:
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

# define derive_eval_1( func )                      \
          case hc_##func: *cstack.target = func( cstack.target->cast_to_double() ); break;

          derive_eval_1( abs )
          derive_eval_1( floor )
          derive_eval_1( ceil )
          derive_eval_1( round )
          derive_eval_1( acos )
          derive_eval_1( asin )
          derive_eval_1( atan )
          derive_eval_1( cos )
          derive_eval_1( cosh )
          derive_eval_1( sin )
          derive_eval_1( sinh )
          derive_eval_1( tan )
          derive_eval_1( tanh )
          derive_eval_1( exp )
          derive_eval_1( log )
          derive_eval_1( log2 )
          derive_eval_1( log10 )
          derive_eval_1( sqrt )
# undef derive_eval_1

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

          case hc_lt:     *cstack.target = cstack.target->lt( cstack.zval() );  break;
          case hc_le:     *cstack.target = cstack.target->le( cstack.zval() );  break;
          case hc_gt:     *cstack.target = cstack.target->gt( cstack.zval() );  break;
          case hc_ge:     *cstack.target = cstack.target->ge( cstack.zval() );  break;
          case hc_eq:     *cstack.target = cstack.target->eq( cstack.zval() );  break;
          case hc_ne:     *cstack.target = cstack.target->ne( cstack.zval() );  break;
          case hc_in:     *cstack.target = in( *cstack.target, cstack.zval() ); break;

          case hc_pow:    *cstack.target = pow(
            cstack.target->cast_to_double(), cstack.zval().cast_to_double() ); break;

          default:        throw std::runtime_error( "unknown operator" );
        }
      }
    }

    return result;
  }

  // Expression implementation

# define derive_constructor( _type_ ) \
  Expression::Expression( _type_ t )  \
  {  mtc::zval( t ).Serialize( ::Serialize( (std::vector<char>*)this, char(hc_const) ) );  }

  derive_constructor( char )
  derive_constructor( uint8_t )
  derive_constructor( int16_t )
  derive_constructor( uint16_t )
  derive_constructor( int32_t )
  derive_constructor( uint32_t )
  derive_constructor( int64_t )
  derive_constructor( uint64_t )
  derive_constructor( float )
  derive_constructor( double )
  derive_constructor( const char* )
  derive_constructor( const widechar* )
# undef derive_constructor

# define derive_constructor_copy( _type_ ) \
  Expression::Expression( const _type_& t )  \
    {  mtc::zval( t ).Serialize( ::Serialize( (std::vector<char>*)this, char(hc_const) ) );  }

  derive_constructor_copy( mtc::charstr )
  derive_constructor_copy( mtc::widestr )
  derive_constructor_copy( mtc::array_char )
  derive_constructor_copy( mtc::array_byte )
  derive_constructor_copy( mtc::array_int16_t )
  derive_constructor_copy( mtc::array_word16_t )
  derive_constructor_copy( mtc::array_int32_t )
  derive_constructor_copy( mtc::array_word32_t )
  derive_constructor_copy( mtc::array_int64_t )
  derive_constructor_copy( mtc::array_word64_t )
  derive_constructor_copy( mtc::array_float )
  derive_constructor_copy( mtc::array_double )
  derive_constructor_copy( mtc::array_charstr )
  derive_constructor_copy( mtc::array_widestr )
# undef derive_constructor

# define derive_binary_operator( op, code )                       \
  Expression& Expression::operator op ( const Expression& xp )    \
  {                                                               \
    insert( begin(), char(hc_##code) );                           \
    insert( end(), xp.begin(), xp.end() );                        \
    return *this;                                                 \
  }

  derive_binary_operator( *=, mul )
  derive_binary_operator( /=, div )
  derive_binary_operator( %=, mod )
  derive_binary_operator( +=, add )
  derive_binary_operator( -=, sub )
  derive_binary_operator( <<=, shl )
  derive_binary_operator( >>=, shr )
  derive_binary_operator( &=, b_and )
  derive_binary_operator( ^=, b_xor )
  derive_binary_operator( |=, b_or )
# undef derive_binary_operator

# define derive_binary_operator( op, code )                           \
  Expression  Expression::operator op ( const Expression& xp ) const  \
  {                                                                   \
    std::vector<char> out{ char(hc_##code) };                         \
    out.insert( out.end(), begin(), end() );                          \
    out.insert( out.end(), xp.begin(), xp.end() );                    \
    return out;                                                       \
  }

  derive_binary_operator( *, mul )
  derive_binary_operator( /, div )
  derive_binary_operator( %, mod )
  derive_binary_operator( +, add )
  derive_binary_operator( -, sub )
  derive_binary_operator( <<, shl )
  derive_binary_operator( >>, shr )
  derive_binary_operator( <, lt )
  derive_binary_operator( <=, le )
  derive_binary_operator( >, gt )
  derive_binary_operator( >=, ge )
  derive_binary_operator( ==, eq )
  derive_binary_operator( !=, ne )
  derive_binary_operator( &&, l_and )
  derive_binary_operator( ||, l_or )
# undef derive_binary_operator

  Expression  Expression::operator_in( const Expression& xp ) const
  {
    std::vector<char> out{ char(hc_in) };
    out.insert( out.end(), begin(), end() );
    out.insert( out.end(), xp.begin(), xp.end() );
    return out;
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

# define derive_math( fn )                                  \
  hopcalc::Expression fn( const hopcalc::Expression& xp )   \
  {                                                         \
    std::vector out{ char(hopcalc::hc_##fn) };              \
      out.insert( out.end(), xp.begin(), xp.end() );        \
    return out;                                             \
  }

  derive_math( abs )
  derive_math( floor )
  derive_math( ceil )
  derive_math( round )
  derive_math( acos )
  derive_math( asin )
  derive_math( atan )
  derive_math( cos )
  derive_math( cosh )
  derive_math( sin )
  derive_math( sinh )
  derive_math( tan )
  derive_math( tanh )
  derive_math( exp )
  derive_math( log )
  derive_math( log2 )
  derive_math( log10 )
  derive_math( sqrt )
# undef derive_math

hopcalc::Expression pow( const hopcalc::Expression& x1, const hopcalc::Expression& x2 )
{
  std::vector out{ char(hopcalc::hc_pow) };
  out.insert( out.end(), x1.begin(), x1.end() );
  out.insert( out.end(), x2.begin(), x2.end() );
  return out;
}
