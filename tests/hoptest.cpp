# include "../hopcalc.hpp"
# include <mtc/test-it-easy.hpp>

namespace hopcalc
{
  auto  ExprLen( const char* pbeg, const char* pend ) -> size_t;

  auto  ExprLen( const char* pbeg ) -> size_t
    {  return ExprLen( pbeg, pbeg + strlen( pbeg ) );  }
  auto  ExprLen( std::string_view view ) -> size_t
    {  return ExprLen( view.data(), view.data() + view.size() );  }
}

using namespace hopcalc;

struct UserData: IUserData, protected mtc::zmap
{
  auto  Get( std::string_view key ) -> mtc::zval override
  {
    auto  pvalue = get( key );

    return pvalue != nullptr ? *pvalue : mtc::zval();
  }
  auto  Set( std::string_view key, const mtc::zval& val ) -> mtc::zval& override
  {
    return *put( key, val );
  }
};

template <class Call>
auto  GetSyntaxError( Call call ) -> SyntaxError
{
  try
  {
    call();
    return { "no error occured", -1 };
  }
  catch ( const SyntaxError& x )
  {
    return x;
  }
  catch ( ... )
  {
    return { "unknown error", -1 };
  }
}

TestItEasy::RegisterFunc  storage_fs( []()
  {
    TEST_CASE( "hopcalc" )
    {
      SECTION( "ExprLen breaks strings to expressions" )
      {
        SECTION( "called with empty string, it causes exception" )
        {
          REQUIRE( ExprLen( "" ) == 0 );
        }
        SECTION( "called with spaced string, it causes exception" )
        {
          REQUIRE_EXCEPTION( ExprLen( " a" ), std::invalid_argument );
        }
        SECTION( "for simple strings, it returns length" )
        {
          REQUIRE( ExprLen( "a" ) == 1 );
          REQUIRE( ExprLen( "12" ) == 2 );
          REQUIRE( ExprLen( "196.4" ) == 5 );
          REQUIRE( ExprLen( "-1" ) == 1 );
        }
        SECTION( "meeting spaces, braces or quotes, it stops" )
        {
          REQUIRE( ExprLen( "abc(" ) == 3 );
          REQUIRE( ExprLen( "12[" ) == 2 );
          REQUIRE( ExprLen( "196.4\"" ) == 5 );
          REQUIRE( ExprLen( "196.4\''" ) == 5 );
          REQUIRE( ExprLen( "196.4 " ) == 5 );
          REQUIRE( ExprLen( "-196.4 " ) == 1 );
        }
        SECTION( "for braced expressions, it searches to the end" )
        {
          REQUIRE( ExprLen( "(abc def) " ) == 9 );
          REQUIRE( ExprLen( "-(abc def)" ) == 1 );
          REQUIRE( ExprLen( "[12]a" ) == 4 );
          REQUIRE( ExprLen( "\"196.4\"6" ) == 7 );
          REQUIRE( ExprLen( "\'196.4\'." ) == 7 );
        }
        SECTION( "for unpaired braces, it causes exception" )
        {
          REQUIRE_EXCEPTION( ExprLen( "(abc def] " ), std::invalid_argument );
          REQUIRE_EXCEPTION( ExprLen( "[12[a" ), std::invalid_argument );
          REQUIRE_EXCEPTION( ExprLen( "\"196.4" ), std::invalid_argument );
          REQUIRE_EXCEPTION( ExprLen( "\'196.4" ), std::invalid_argument );
        }
        SECTION( "escaping supported in string literals" )
        {
          REQUIRE( ExprLen( "\"abc\\\"de\"") == 9 );
          REQUIRE( ExprLen( "\'abc\\\'de\'") == 9 );
        }
      }
      SECTION( "Compile() creates the compiled expressions" )
      {
        REQUIRE( Compile( "3 + 5" ).size() != 0 );
        REQUIRE( Compile( "\"a\" + 1" ).size() != 0 );
        REQUIRE( Compile( "1 + \"a\"" ).size() != 0 );
        REQUIRE( Compile( "1 + 3 * 7" ).size() != 0 );
        REQUIRE( Compile( "-1 + 3 * 7" ).size() != 0 );
        REQUIRE( Compile( "$x" ).size() != 0 );
        REQUIRE( Compile( "$x && $y.data" ).size() != 0 );

        REQUIRE( Compile( "-pow(3.0, 4)" ).size() != 0 );

        REQUIRE( Compile( "[]" ).size() != 0 );
        REQUIRE( Compile( "[1]" ).size() != 0 );
        REQUIRE( Compile( "[1, 2]" ).size() != 0 );
        REQUIRE( Compile( "[1, 2, \"a\"]" ).size() != 0 );
        REQUIRE( Compile( "[1, 2, \"a\", [2.57, 3.14]]" ).size() != 0 );

        REQUIRE( Compile( "2 in [1, 2, \"a\", [2.57, 3.14]]" ).size() != 0 );

        SECTION( "it checks argument count for functions" )
        {
          REQUIRE_EXCEPTION( Compile( "pow()" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "pow()" );  } ).GetOffset() == 4 );
          REQUIRE_EXCEPTION( Compile( "pow(3.0)" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "pow(3.0)" );  } ).GetOffset() == 7 );
          REQUIRE_EXCEPTION( Compile( "pow(3.0, 4, 2)" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "pow(3.0, 4, 2)" );  } ).GetOffset() == 12 );
          REQUIRE_EXCEPTION( Compile( "[, 2]" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "[, 2]" );  } ).GetOffset() == 1 );
          REQUIRE_EXCEPTION( Compile( "[1, [2]" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "[1, [2]" );  } ).GetOffset() == 7 );
          REQUIRE_EXCEPTION( Compile( "[1, 2, $a]" ), hopcalc::SyntaxError );
            REQUIRE( GetSyntaxError( [](){  Compile( "[1, 2, $a]" );  } ).GetOffset() == 7 );
        }
        SECTION( "it pre-calculates constant expressions" )
        {
          REQUIRE( Expression( Compile( "3 + 4" ) ).to_string() == "7" );
          REQUIRE( Expression( Compile( "1 & 3" ) ).to_string() == "1" );
        }
      }
      SECTION( "Evaluate() evaluates compiled expressions" )
      {
        SECTION( "arythmetic and string" )
        {
          REQUIRE( Evaluate( Compile( "3 + 4" ) ).eq( 7 ) );
          REQUIRE( Evaluate( Compile( "\"3\" + \"4\"" ) ).eq( "34" ) );
          REQUIRE( Evaluate( Compile( "0 && $a" ) ).eq( 0 ) );
          REQUIRE( Evaluate( Compile( "3 || $a" ) ).eq( 3 ) );
          REQUIRE( Evaluate( Compile( "-3" ) ).eq( -3 ) );
          REQUIRE( Evaluate( Compile( "1 +-3" ) ).eq( -2 ) );
          REQUIRE( Evaluate( Compile( "[1, 2, '3']" ) ).eq( mtc::array_zval{ 1, 2, "3" } ) );
        }
        SECTION( "compare operations" )
        {
          REQUIRE( Evaluate( Compile( "1 == 2" ) ).eq( false ) );
          REQUIRE( Evaluate( Compile( "2 == 2" ) ).eq( true ) );
          REQUIRE( Evaluate( Compile( "\"aaa\" == \"bbb\"" ) ).eq( false ) );
          REQUIRE( Evaluate( Compile( "\"aaa\" == \"aaa\"" ) ).eq( true ) );
          REQUIRE( Evaluate( Compile( "\"aaa\" == \"aaa\"" ) ).eq( true ) );

          REQUIRE( Evaluate( Compile( "2 in [1, 2, \"a\", [2.57, 3.14]]" ) ).eq( true ) );
          REQUIRE( Evaluate( Compile( "5 in [1, 2, \"a\", [2.57, 3.14]]" ) ).eq( false ) );
        }
        SECTION( "long boolean expressions" )
        {
          REQUIRE( Evaluate( Compile( "1 == 2 || \"a\" == 4" ) ).eq( false ) );
          REQUIRE( Evaluate( Compile( "2 == 2 || \"a\" == 4" ) ).eq( true ) );
        }
        SECTION( "expressions with braces" )
        {
          REQUIRE( Evaluate( Compile( "(1 + 2) * 4" ) ).eq( 12 ) );
          REQUIRE( Evaluate( Compile( "-1.7 + (1 + 2) * 4" ) ).eq( 10.3 ) );
          REQUIRE( Evaluate( Compile( "-1.7 - -1.7" ) ).eq( 0 ) );
          REQUIRE( Evaluate( Compile( "7 - 2 - 2" ) ).eq( 3 ) );
          REQUIRE( Evaluate( Compile( "7 + -(2 - 1)" ) ).eq( 6 ) );
          REQUIRE( Evaluate( Compile( "7 - -(2 - 1)" ) ).eq( 8 ) );
          REQUIRE( Evaluate( Compile( "-$a" ), []( std::string_view ){  return mtc::zval( 7 );  } ).eq( -7 ) );
          REQUIRE( Evaluate( Compile( "(1+2)*(3+4)" ) ).eq( 21 ) );
        }
        SECTION( "functions" )
        {
          REQUIRE( Evaluate( Compile( "1 + pi()" ) ).gt( 4.1411 ) );
          REQUIRE( Evaluate( Compile( "1 + pi()" ) ).lt( 4.1416 ) );
          REQUIRE( Evaluate( Compile( "sin(pi()/2) > 0.99" ) ).ne( 0 ) );
          REQUIRE( Evaluate( Compile( "cos(pi()/2) < 0.01" ) ).ne( 0 ) );
          REQUIRE( Evaluate( Compile( "cos(pi()/2 + pi()/6) < 0" ) ).ne( 0 ) );
          REQUIRE( Evaluate( Compile( "pow(3,4)" ) ).eq( 81 ) );
        }
        SECTION( "ternary operator" )
        {
          REQUIRE( Evaluate( Compile( "7 ? 1 : \"one\"" ) ).eq( 1 ) );
          REQUIRE( Evaluate( Compile( "6 + (7 ? 1 : \"one\")" ) ).eq( 7 ) );
          REQUIRE( Evaluate( Compile( "(7 ? 1 : \"one\") + 11" ) ).eq( 12 ) );
          REQUIRE( Evaluate( Compile( "2 * ((7 ? 1 : \"one\") + 11)" ) ).eq( 24 ) );
          REQUIRE( Evaluate( Compile( "(1 == 1) ? \"zero\" : 0" ) ).eq( "zero" ) );
          REQUIRE( Evaluate( Compile( "(1 == 2) ? \"zero\" : 0" ) ).eq( 0 ) );
        }
        SECTION( "assignment" )
        {
          UserData  func;
          mtc::zval zval;

          REQUIRE( Evaluate( Compile( "7 ? 1 : \"one\"" ) ).eq( 1 ) );
          REQUIRE( Evaluate( Compile( "6 + (7 ? 1 : \"one\")" ) ).eq( 7 ) );
          REQUIRE( Evaluate( Compile( "(7 ? 1 : \"one\") + 11" ) ).eq( 12 ) );
          REQUIRE( Evaluate( Compile( "(1 == 1) ? \"zero\" : 0" ) ).eq( "zero" ) );
          REQUIRE( Evaluate( Compile( "(1 == 2) ? \"zero\" : 0" ) ).eq( 0 ) );
          REQUIRE( Evaluate( Compile( "$a = \"zzz\"" ), &func ).eq( "zzz" ) );
          REQUIRE( Evaluate( Compile( "$a = \"yyy\"" ), [&]( std::string_view, const mtc::zval& zv ) -> mtc::zval&
            {  return zval = zv;  }).eq( "yyy" ) );
          REQUIRE( zval.eq( "yyy" ) );
        }
        SECTION( "some expressions are transitive" )
        {
          REQUIRE( Evaluate( Compile( "1 + \"a\"" ) ).eq( "1a" ) );
          REQUIRE( Evaluate( Compile( "\"a\" + 1" ) ).eq( "a1" ) );
        }
      }
      SECTION( "Expression constructor" )
      {
        SECTION( "it constructs expressions" )
        {
          REQUIRE( Evaluate( Expression( "aaa" ) += "bbb" ).eq( "aaabbb" ) );
          REQUIRE( Evaluate( Expression( 721 ) + 2 ).eq( 723 ) );
          REQUIRE( Evaluate( Expression( 'b' ).operator_in( Expression( "abc" ) ) ).eq( true ) );
          REQUIRE( Evaluate( Expression( 'q' ).operator_in( Expression( "abc" ) ) ).eq( false ) );
          REQUIRE( Evaluate( Expression( "b" ).operator_in( Expression( { "a", "b", "c" } ) ) ).eq( true ) );
          REQUIRE( Evaluate( Expression( 7 ).operator_in( mtc::array_int32{ 2, 3, 11 } ) ).eq( false ) );
          REQUIRE( Evaluate( Expression( 11 ).operator_in( mtc::array_int32{ 2, 3, 11 } ) ).eq( true ) );
          REQUIRE( Evaluate( sqrt( Expression( 81 ) ) ).eq( 9 ) );
          REQUIRE( Evaluate( sqrt( Expression( "abc" ) ) ).get_type() == mtc::zval::z_double );
          REQUIRE( Evaluate( sqrt( Expression( "abc" ) ) ).eq( 0 ) );
          REQUIRE( Evaluate( pow( Expression( 2 ), 4 ) ).eq( 16 ) );
        }
        SECTION( "expressions are printable" )
        {
          REQUIRE( ((Expression( "a" ) + Expression::Variable( "b" )) / (3 + 2) ).to_string() == "(\"a\" + $b) / 5" );

          SECTION( "- functions" )
          {
            REQUIRE( ( sin( Expression( 1.57 ) ) / pi() ).to_string() == "sin(1.57) / pi()" );
          }

          SECTION( "- pre-calculated operators")
          {
            REQUIRE( (Expression( 3 ) * 1).to_string() == "3" );
            REQUIRE( (Expression( 3 ) / 3).to_string() == "1" );
            REQUIRE( (Expression( 3 ) % 2).to_string() == "1" );
            REQUIRE( (Expression( 3 ) + 1).to_string() == "4" );
            REQUIRE( (Expression( 3 ) - 1).to_string() == "2" );
            REQUIRE( (Expression( 3 ) << 1).to_string() == "6" );
            REQUIRE( (Expression( 3 ) >> 1).to_string() == "1" );
            REQUIRE( (Expression( 3 ) & 1).to_string() == "1" );
            REQUIRE( (Expression( 3 ) ^ 1).to_string() == "2" );
            REQUIRE( (Expression( 2 ) | 1).to_string() == "3" );

            REQUIRE( (Expression( true ) && true).to_string() == "true" );
            REQUIRE( (Expression( true ) && false).to_string() == "false" );
            REQUIRE( (Expression( false ) && true ).to_string() == "false" );
            REQUIRE( (Expression( false ) && false ).to_string() == "false" );

            REQUIRE( (Expression( true ) || true).to_string() == "true" );
            REQUIRE( (Expression( true ) || false).to_string() == "true" );
            REQUIRE( (Expression( false ) || true).to_string() == "true" );
            REQUIRE( (Expression( false ) || false).to_string() == "false" );

            REQUIRE( (Expression( 3 ) *= 1).to_string() == "3" );
            REQUIRE( (Expression( 3 ) /= 3).to_string() == "1" );
            REQUIRE( (Expression( 3 ) %= 2).to_string() == "1" );
            REQUIRE( (Expression( 3 ) += 1).to_string() == "4" );
            REQUIRE( (Expression( 3 ) -= 1).to_string() == "2" );
            REQUIRE( (Expression( 3 ) <<= 1).to_string() == "6" );
            REQUIRE( (Expression( 3 ) >>= 1).to_string() == "1" );
            REQUIRE( (Expression( 3 ) &= 1).to_string() == "1" );
            REQUIRE( (Expression( 3 ) ^= 1).to_string() == "2" );
            REQUIRE( (Expression( 2 ) |= 1).to_string() == "3" );

            REQUIRE( (Expression( 1 ).operator_in( std::vector<int>{ 1, 2, 3 } )).to_string() == "true" );
          }

          REQUIRE( conditional( Expression::Variable( "param" ) == 3, "a", "b" ).to_string() == "$param == 3 ? \"a\" : \"b\"" );
        }
      }
    }
  } );

# include <chrono>

template <unsigned Lops, class Func>
auto  Measure( Func func ) -> double
{
  auto  tstart = std::chrono::steady_clock::now();

  for ( unsigned i = 0; i < Lops; ++i )
    func();

  return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - tstart ).count();
}

int   main()
{
  UserData func;

//  auto  expr = Compile( "$a + $b + $c" );
  auto  expr = Compile( "(($1 + 1) + ($b - 1)) / $c" );
 // fprintf( stdout, "%s\n", Evaluate( expr, []( const char*, size_t ) -> mtc::zval{  return 14;  } ).to_string().c_str() );

//  return 0;
  fprintf( stdout, "%.2f\n", Measure<1000000>( [&]()
    {
      Evaluate( expr, { []( std::string_view ) -> mtc::zval{  return 14;  } } );
    } ) );

  return TestItEasy::Conclusion();
}
