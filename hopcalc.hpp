# if !defined( __hopcalc_hpp__ )
# define __hopcalc_hpp__
# include <mtc/zmap.h>
# include <functional>

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

}

# endif // !__hopcalc_hpp__
