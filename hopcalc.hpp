# if !defined( __hopcalc_hpp__ )
# define __hopcalc_hpp__
# include <mtc/zmap.h>
# include <functional>

namespace hopcalc
{
  class Expression;
}

hopcalc::Expression abs  ( const hopcalc::Expression& );
hopcalc::Expression floor( const hopcalc::Expression& );
hopcalc::Expression ceil ( const hopcalc::Expression& );
hopcalc::Expression round( const hopcalc::Expression& );
hopcalc::Expression acos ( const hopcalc::Expression& );
hopcalc::Expression asin ( const hopcalc::Expression& );
hopcalc::Expression atan ( const hopcalc::Expression& );
hopcalc::Expression cos  ( const hopcalc::Expression& );
hopcalc::Expression cosh ( const hopcalc::Expression& );
hopcalc::Expression sin  ( const hopcalc::Expression& );
hopcalc::Expression sinh ( const hopcalc::Expression& );
hopcalc::Expression tan  ( const hopcalc::Expression& );
hopcalc::Expression tanh ( const hopcalc::Expression& );
hopcalc::Expression exp  ( const hopcalc::Expression& );
hopcalc::Expression log  ( const hopcalc::Expression& );
hopcalc::Expression log2 ( const hopcalc::Expression& );
hopcalc::Expression log10( const hopcalc::Expression& );
hopcalc::Expression sqrt ( const hopcalc::Expression& );
hopcalc::Expression pow  ( const hopcalc::Expression&, const hopcalc::Expression& );

namespace hopcalc
{

  class SyntaxError: public std::invalid_argument
  {
    using std::invalid_argument::invalid_argument;

  public:
    SyntaxError( const std::string& msg, int pos ): invalid_argument( msg ), offset( pos ) {}
    SyntaxError( const char* msg, int pos ): invalid_argument( msg ), offset( pos ) {}
    SyntaxError( const SyntaxError& ) = default;

    int   GetOffset() const {  return offset;  }

  protected:
    int   offset;

  };

 /*
  * User space variables ($[a-z\.:]) access interface
  */
  struct IUserData
  {
    virtual auto  Get( std::string_view ) -> mtc::zval = 0;
    virtual auto  Set( std::string_view, const mtc::zval& ) -> mtc::zval& = 0;
  };

  class WUserData: public IUserData
  {
    IUserData*                                                      ptr = nullptr;
    std::function<mtc::zval ( std::string_view )>                   get;
    std::function<mtc::zval&( std::string_view, const mtc::zval& )> set;

  public:
    WUserData():
      get( []( std::string_view ) -> mtc::zval {  throw std::invalid_argument( "'Get( key )' is undefined" );  } ),
      set( []( std::string_view, const mtc::zval& ) -> mtc::zval& {  throw std::invalid_argument( "'Set( key, val )' is undefined" );  } ) {}
    WUserData( IUserData* func ):
      ptr( func ) {}

    template <typename F, typename std::enable_if<
      std::is_convertible<typename std::result_of<F(std::string_view)>::type, mtc::zval>::value, int>::type = 0>
    WUserData( F&& fn_get ): get( fn_get ),
      set( []( std::string_view, const mtc::zval& ) -> mtc::zval& {  throw std::invalid_argument( "'Set( key, val )' is undefined" );  } ) {}

    template <typename F, typename std::enable_if<
      std::is_convertible<typename std::result_of<F(std::string_view, const mtc::zval&)>::type, mtc::zval&>::value, int>::type = 1>
    WUserData( F&& fn_set ):
      get( []( std::string_view ) -> mtc::zval {  throw std::invalid_argument( "'Get( key )' is undefined" );  } ),
      set( fn_set ) {}

    template <typename F1, typename F2, std::enable_if_t<std::is_invocable_r_v<mtc::zval, F1, std::string_view>
      && std::is_invocable_r_v<mtc::zval&, F2, std::string_view, const mtc::zval&>, int> = 0>
    WUserData( F1&& fn_get, F2&& fn_set ):
      get( fn_get ),
      set( fn_set ) {}

    auto  Get( std::string_view key ) -> mtc::zval override
      {  return get( key );  }
    auto  Set( std::string_view key, const mtc::zval& val ) -> mtc::zval& override
      {  return set( key, val );  }

    operator IUserData* () const {  return ptr != nullptr ? ptr : (IUserData*)this;  }
  };

  class Expression: public std::vector<char>
  {
    Expression( std::vector<char>&& src ): vector( std::move( src ) ) {}

    friend auto ::abs  ( const Expression& ) -> Expression;
    friend auto ::floor( const Expression& ) -> Expression;
    friend auto ::ceil ( const Expression& ) -> Expression;
    friend auto ::round( const Expression& ) -> Expression;
    friend auto ::acos ( const Expression& ) -> Expression;
    friend auto ::asin ( const Expression& ) -> Expression;
    friend auto ::atan ( const Expression& ) -> Expression;
    friend auto ::cos  ( const Expression& ) -> Expression;
    friend auto ::cosh ( const Expression& ) -> Expression;
    friend auto ::sin  ( const Expression& ) -> Expression;
    friend auto ::sinh ( const Expression& ) -> Expression;
    friend auto ::tan  ( const Expression& ) -> Expression;
    friend auto ::tanh ( const Expression& ) -> Expression;
    friend auto ::exp  ( const Expression& ) -> Expression;
    friend auto ::log  ( const Expression& ) -> Expression;
    friend auto ::log2 ( const Expression& ) -> Expression;
    friend auto ::log10( const Expression& ) -> Expression;
    friend auto ::sqrt ( const Expression& ) -> Expression;
    friend auto ::pow  ( const Expression&, const Expression& ) -> Expression;

  public:
    Expression( char );
    Expression( uint8_t );
    Expression( int16_t );
    Expression( uint16_t );
    Expression( int32_t );
    Expression( uint32_t );
    Expression( int64_t );
    Expression( uint64_t );
    Expression( float );
    Expression( double );
    Expression( const mtc::charstr& );
    Expression( const mtc::widestr& );
    Expression( const char* );
    Expression( const widechar* );
    Expression( const std::vector<char>& );
    Expression( const std::vector<uint8_t>& );
    Expression( const std::vector<int16_t>& );
    Expression( const std::vector<uint16_t>& );
    Expression( const std::vector<int32_t>& );
    Expression( const std::vector<uint32_t>& );
    Expression( const std::vector<int64_t>& );
    Expression( const std::vector<uint64_t>& );
    Expression( const std::vector<float>& );
    Expression( const std::vector<double>& );
    Expression( const std::vector<mtc::charstr>& );
    Expression( const std::vector<mtc::widestr>& );

    Expression& operator *= ( const Expression& );
    Expression& operator /= ( const Expression& );
    Expression& operator %= ( const Expression& );
    Expression& operator += ( const Expression& );
    Expression& operator -= ( const Expression& );
    Expression& operator <<= ( const Expression& );
    Expression& operator >>= ( const Expression& );
    Expression& operator &= ( const Expression& );
    Expression& operator ^= ( const Expression& );
    Expression& operator |= ( const Expression& );
    Expression& operator = ( const Expression& );

    Expression  operator * ( const Expression& ) const;
    Expression  operator / ( const Expression& ) const;
    Expression  operator % ( const Expression& ) const;
    Expression  operator + ( const Expression& ) const;
    Expression  operator - ( const Expression& ) const;
    Expression  operator << ( const Expression& ) const;
    Expression  operator >> ( const Expression& ) const;
    Expression  operator < ( const Expression& ) const;
    Expression  operator <= ( const Expression& ) const;
    Expression  operator > ( const Expression& ) const;
    Expression  operator >= ( const Expression& ) const;
    Expression  operator == ( const Expression& ) const;
    Expression  operator != ( const Expression& ) const;
    Expression  operator & ( const Expression& ) const;
    Expression  operator ^ ( const Expression& ) const;
    Expression  operator | ( const Expression& ) const;
    Expression  operator && ( const Expression& ) const;
    Expression  operator || ( const Expression& ) const;

    Expression  operator_in ( const Expression& ) const;
  };

  auto  Compile( const char*, const char*, const char* = nullptr ) -> std::vector<char>;
  auto  Evaluate( const char*, const char*, IUserData* = nullptr ) -> mtc::zval;

  inline
  auto  Compile( std::string_view view ) -> std::vector<char>
    {  return Compile( view.data(), view.data() + view.size() );  }

  inline
  auto  Evaluate( std::string_view prog, const WUserData& func = {} ) -> mtc::zval
    {  return Evaluate( prog.data(), prog.data() + prog.size(), func );  }

  inline
  auto  Evaluate( const std::vector<char>& prog, const WUserData& func = {} ) -> mtc::zval
    {  return Evaluate( prog.data(), prog.data() + prog.size(), func );  }

  // Expression implementation

}

# endif // !__hopcalc_hpp__
