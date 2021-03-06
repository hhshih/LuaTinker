﻿// lua_tinker.h
//
// LuaTinker - Simple and light C++ wrapper for Lua.
//
// Copyright (c) 2005-2007 Kwon-il Lee (zupet@hitel.net)
//
// please check Licence.txt file for licence and legal issues.

#if !defined(_LUA_TINKER_H_)
#define _LUA_TINKER_H_

#include <new>
#include <cstring>
#include <type_traits>
#include "lua/lua.hpp"

namespace Gameplay
{
    class GameSpace;
}
#define ADD_CPREFIX(s) "_C_" s

namespace lua_tinker
{
    // init LuaTinker
    void	init(lua_State *L);

    void	init_s64(lua_State *L);
    void	init_u64(lua_State *L);

    // string-buffer excution
    void	dofile(lua_State *L, const char *filename);
    void	dostring(lua_State *L, const char* buff);
    void	dobuffer(lua_State *L, const char* buff, size_t sz);

    // debug helpers
    void	enum_stack(lua_State *L);
    int		on_error(lua_State *L);
    void	print_error(lua_State *L, const char* fmt, ...);

    // class helper
    int meta_get_no_error(lua_State *L);
    int meta_get(lua_State *L);
    int meta_set(lua_State *L);
    void push_meta(lua_State *L, const char* name);

    // dynamic type extention
    struct lua_value
    {
        virtual void to_lua(lua_State *L) = 0;
    };

    // type trait
    template<typename T> struct class_name;
    struct table;

    template<bool C, typename A, typename B> struct if_ {};
    template<typename A, typename B>		struct if_<true, A, B> { typedef A type; };
    template<typename A, typename B>		struct if_<false, A, B> { typedef B type; };

    template<typename A>
    struct is_ptr { static const bool value = false; };
    template<typename A>
    struct is_ptr<A*> { static const bool value = true; };

    template<typename A>
    struct is_ref { static const bool value = false; };
    template<typename A>
    struct is_ref<A&> { static const bool value = true; };

    template<typename A>
    struct remove_const { typedef A type; };
    template<typename A>
    struct remove_const<const A> { typedef A type; };

    template<typename A>
    struct base_type { typedef A type; };
    template<typename A>
    struct base_type<A*> { typedef A type; };
    template<typename A>
    struct base_type<A&> { typedef A type; };

    template<typename T>
    struct clean_type
    {
      using type = std::remove_pointer_t<std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<std::remove_const_t<T>>>>>;
    };

    template<typename T>
    using remove_const_reference = std::remove_const_t<std::remove_reference_t<T>>;

    template<typename T>
    using class_type = clean_type<T>;

    template<typename A>
    struct is_obj { static const bool value = !is_enum<A>::value; };
    template<> struct is_obj<char>					{ static const bool value = false; };
    template<> struct is_obj<unsigned char>			{ static const bool value = false; };
    template<> struct is_obj<short>					{ static const bool value = false; };
    template<> struct is_obj<unsigned short>		{ static const bool value = false; };
    template<> struct is_obj<long>					{ static const bool value = false; };
    template<> struct is_obj<unsigned long>			{ static const bool value = false; };
    template<> struct is_obj<int>					{ static const bool value = false; };
    template<> struct is_obj<unsigned int>			{ static const bool value = false; };
    template<> struct is_obj<float>					{ static const bool value = false; };
    template<> struct is_obj<double>				{ static const bool value = false; };
    template<> struct is_obj<char*>					{ static const bool value = false; };
    template<> struct is_obj<const char*>			{ static const bool value = false; };
    template<> struct is_obj<bool>					{ static const bool value = false; };
    template<> struct is_obj<lua_value*>			{ static const bool value = false; };
    template<> struct is_obj<long long>				{ static const bool value = false; };
    template<> struct is_obj<unsigned long long>	{ static const bool value = false; };
    template<> struct is_obj<table>					{ static const bool value = false; };

    /////////////////////////////////
    enum { no = 1, yes = 2 };
    typedef char(&no_type)[no];
    typedef char(&yes_type)[yes];

    struct int_conv_type { int_conv_type(int); };

    no_type int_conv_tester(...);
    yes_type int_conv_tester(int_conv_type);

    no_type vfnd_ptr_tester(const volatile char *);
    no_type vfnd_ptr_tester(const volatile short *);
    no_type vfnd_ptr_tester(const volatile int *);
    no_type vfnd_ptr_tester(const volatile long *);
    no_type vfnd_ptr_tester(const volatile double *);
    no_type vfnd_ptr_tester(const volatile float *);
    no_type vfnd_ptr_tester(const volatile bool *);
    yes_type vfnd_ptr_tester(const volatile void *);

    template <typename T> T* add_ptr(T&);

    template <bool C> struct bool_to_yesno { typedef no_type type; };
    template <> struct bool_to_yesno<true> { typedef yes_type type; };

    using std::is_enum;

    /////////////////////////////////

    // from lua
    template<typename T>
    struct void2val { static T invoke(void* input){ return *(T*)input; } };
    template<typename T>
    struct void2ptr { static T* invoke(void* input){ return *(T**)(void**)(&input); } };
    template<typename T>
    struct void2ref { static T& invoke(void* input){ return *(T*)input; } };

    template<typename T>
    struct void2type
    {
        static T invoke(void* ptr)
        {
            return	if_<is_ptr<T>::value
                , void2ptr<typename base_type<T>::type>
                , typename if_<is_ref<T>::value
                , void2ref<typename base_type<T>::type>
                , void2val<typename base_type<T>::type>
                >::type
            >::type::invoke(ptr);
        }
    };

    struct user
    {
        user(void* p) : m_p(p) {}
        virtual ~user() {}
        void* m_p;
    };

    template<typename T>
    struct user2type { static T invoke(lua_State *L, int index) { return void2type<T>::invoke(lua_touserdata(L, index)); } };

    template<typename T>
    struct lua2enum 
    {
        static T invoke(lua_State *L, int index)
        {
          if (!lua_isnumber(L, index))
            throw std::exception("Cannot get correct enum");

          auto number = lua_tointeger(L, index);
          return *reinterpret_cast<T*>(&number);
        };
    };

    template<typename T>
    struct lua2object
    {
        static T invoke(lua_State *L, int index)
        {
            if (!lua_isuserdata(L, index))
            {
                lua_pushstring(L, "No class at first argument. (forgot ':' expression ?)");
                on_error(L);
                throw std::exception("no class at first argument");
            }
#ifdef DO_LUA_TYPECHECKING
            else
            {
                int top = lua_gettop(L);
                lua_getmetatable(L, index);
                bool has_parent;
                do
                {
                    has_parent = false;
                    lua_pushliteral(L, "__name");
                    lua_gettable(L, -2);
                    const char* lua_class_name = lua_tostring(L, -1);
                    const char* true_class_name = class_name<class_type<T>::type>::name();
                    lua_pop(L, 1);



                    if(true_class_name[0] == '\0')
                    {
                        lua_pushliteral(L, "Invalid input argument type");
                        on_error(L);

                        DA_ASSERT(0, 
                            "Input argument *%d* incorrect. When checking <<%s>> class, cannot find base type name for [[%s]] class! "
                            "(Use CLASS/COMP_EXPORT on base type, then use CLASS/COMP_INH to specify inheritance)", 
                            index - 1,
                            lua_class_name, 
                            typeid(class_type<T>::type).name());
                    }

                    else if (std::strcmp(lua_class_name, true_class_name) != 0)
                    {
                        lua_pushliteral(L, "__parent");
                        lua_gettable(L, -2);
			
                        if (lua_isnil(L, -1))
                        {
                            lua_pushliteral(L, "Invalid input argument type");
                            on_error(L);

                            DA_ASSERT(0, 
                            "Input argument *%d* incorrect. When checking <<%s>> class, cannot find base type name for [[%s]] class! "
                            "(Use CLASS/COMP_EXPORT on base type, then use CLASS/COMP_INH to specify inheritance)", 
                            index - 1,
                            true_class_name, 
                            lua_class_name);
                        }

                        has_parent = true;
                    }

                } while (has_parent);
                lua_settop(L, top);
            }
#endif
            return void2type<T>::invoke(user2type<user*>::invoke(L, index)->m_p);
        }
    };

    template<typename T>
    T lua2type(lua_State *L, int index)
    {
        return	if_<is_enum<T>::value
            , lua2enum<T>
            , lua2object<T>
        >::type::invoke(L, index);
    }

    template<typename T>
    struct val2user : user
    {
        val2user() : user(new T) {}

        template<typename T1>
        val2user(T1 t1) : user(new T(t1)) {}

        template<typename T1, typename T2>
        val2user(T1 t1, T2 t2) : user(new T(t1, t2)) {}

        template<typename T1, typename T2, typename T3>
        val2user(T1 t1, T2 t2, T3 t3) : user(new T(t1, t2, t3)) {}

        template<typename T1, typename T2, typename T3, typename T4>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4) : user(new T(t1, t2, t3, t4)) {}

        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) : user(new T(t1, t2, t3, t4, t5)) {}

        ~val2user() { delete ((T*)m_p); }
    };

    template<typename T>
    struct ptr2user : user
    {
        ptr2user(T* t) : user((void*)t) {}
    };

    template<typename T>
    struct ref2user : user
    {
        ref2user(T& t) : user(&t) {}
    };

    // to lua
    template<typename T>
    struct val2lua { static void invoke(lua_State *L, T& input){ new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(input); } };
    template<typename T>
    struct ptr2lua { static void invoke(lua_State *L, T* input){ if (input) new(lua_newuserdata(L, sizeof(ptr2user<T>))) ptr2user<T>(input); else lua_pushnil(L); } };
    template<typename T>
    struct ref2lua { static void invoke(lua_State *L, T& input){ new(lua_newuserdata(L, sizeof(ref2user<T>))) ref2user<T>(input); } };

    template<typename T>
    struct enum2lua { static void invoke(lua_State *L, T val) { lua_pushnumber(L, static_cast<std::underlying_type<T>::type>(val)); } };

    template<typename T>
    struct object2lua
    {
        static void invoke(lua_State *L, T val)
        {
            if_<is_ptr<T>::value
                , ptr2lua<typename base_type<T>::type>
                , typename if_<is_ref<T>::value
                , ref2lua<typename base_type<T>::type>
                , val2lua<typename base_type<T>::type>
                >::type
            >::type::invoke(L, val);

            push_meta(L, class_name<typename class_type<T>::type>::name());
            lua_setmetatable(L, -2);
        }
    };


    template<typename T>
    void type2lua(lua_State *L, T val)
    {
        if_<is_enum<T>::value
            , enum2lua<T>
            , object2lua<T>
        >::type::invoke(L, val);
    }

    // get value from cclosure
    template<typename T>
    T upvalue_(lua_State *L)
    {
        return user2type<T>::invoke(L, lua_upvalueindex(1));
    }

    // read a value from lua stack
    template<typename T>
    T read(lua_State *L, int index)				{ return lua2type<T>(L, index); }

    template<>	char*				read(lua_State *L, int index);
    template<>	const char*			read(lua_State *L, int index);
    template<>	char				read(lua_State *L, int index);
    template<>	unsigned char		read(lua_State *L, int index);
    template<>	short				read(lua_State *L, int index);
    template<>	unsigned short		read(lua_State *L, int index);
    template<>	long				read(lua_State *L, int index);
    template<>	unsigned long		read(lua_State *L, int index);
    template<>	int					read(lua_State *L, int index);
    template<>	unsigned int		read(lua_State *L, int index);
    template<>	float				read(lua_State *L, int index);
    template<>	double				read(lua_State *L, int index);
    template<>	bool				read(lua_State *L, int index);
    template<>	void				read(lua_State *L, int index);
    template<>	long long			read(lua_State *L, int index);
    template<>	unsigned long long	read(lua_State *L, int index);
    template<>	table				read(lua_State *L, int index);

    // push a value to lua stack
    template<typename T>
    void push(lua_State *L, T ret)					{ type2lua<T>(L, ret); }

    template<>	void push(lua_State *L, char ret);
    template<>	void push(lua_State *L, unsigned char ret);
    template<>	void push(lua_State *L, short ret);
    template<>	void push(lua_State *L, unsigned short ret);
    template<>	void push(lua_State *L, long ret);
    template<>	void push(lua_State *L, unsigned long ret);
    template<>	void push(lua_State *L, int ret);
    template<>	void push(lua_State *L, unsigned int ret);
    template<>	void push(lua_State *L, float ret);
    template<>	void push(lua_State *L, double ret);
    template<>	void push(lua_State *L, char* ret);
    template<>	void push(lua_State *L, const char* ret);
    template<>	void push(lua_State *L, bool ret);
    template<>	void push(lua_State *L, lua_value* ret);
    template<>	void push(lua_State *L, long long ret);
    template<>	void push(lua_State *L, unsigned long long ret);
    template<>	void push(lua_State *L, table ret);
    
    template<typename T>
    using RefCorrector = typename if_<is_obj<remove_const_reference<T>>::value, T ,remove_const_reference<T>>::type;

    // pop a value from lua stack
    template<typename T>
    T pop(lua_State *L) { T t = read<T>(L, -1); lua_pop(L, 1); return t; }

    template<>	void	pop(lua_State *L);
    template<>	table	pop(lua_State *L);

    template <std::size_t... Is>
    struct indices {};

    template <std::size_t N, std::size_t... Is>
    struct build_indices
      : build_indices<N - 1, N - 1, Is...> {};

    template <std::size_t... Is>
    struct build_indices<0, Is...> : indices<Is...> {};

    template<typename RVal, typename ...Args>
    struct functor
    {
        static int invoke(lua_State *L) 
        { 
          try { return invoker(L, build_indices<sizeof...(Args)>{}); }
          catch (std::exception e) { return 0; }
        }
        template<std::size_t... Is> static int invoker(lua_State* L, indices<Is...>) { push(L, upvalue_<RVal(*)(Args...)>(L)(read<RefCorrector<Args>>(L, Is + 1)...)); return 1; }
    };

    template<typename ...Args>
    struct functor<void, Args...>
    {
        static int invoke(lua_State *L) 
        { 
          try { return invoker(L, build_indices<sizeof...(Args)>{}); }
          catch (std::exception e) { return 0; }
        }
        template<std::size_t... Is> static int invoker(lua_State* L, indices<Is...>) { upvalue_<void(*)(Args...)>(L)(read<RefCorrector<Args>>(L, Is + 1)...); return 0; }
    };


    template<typename RVal>
    struct functor<RVal>
    {
        static int invoke(lua_State *L) { push(L, upvalue_<RVal(*)()>(L)()); return 1; }
    };

    template<>
    struct functor<void>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)()>(L)(); return 0; }
    };

    // functor (non-managed)
    template<typename T1>
    struct functor<int, lua_State*, T1>
    {
        static int invoke(lua_State *L) 
        { 
          try
          {
            return upvalue_<int(*)(lua_State*, T1)>(L)(L, read<T1>(L, 1));
          }
          catch (std::exception e)
          {
            return 0;
          }
        }
    };

    template<>
    struct functor<int, lua_State*>
    {
        static int invoke(lua_State *L) { return upvalue_<int(*)(lua_State*)>(L)(L); }
    };

    // push_functor
    template<typename RVal>
    void push_functor(lua_State *L, RVal(*func)())
    {
        lua_pushcclosure(L, functor<RVal>::invoke, 1);
    }

    template<typename RVal, typename T1>
    void push_functor(lua_State *L, RVal(*func)(T1))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal, T1>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2>
    void push_functor(lua_State *L, RVal(*func)(T1, T2))
    {
        lua_pushcclosure(L, functor<RVal, T1, T2>::invoke, 1);
        (void)func;
    }

    template<typename RVal, typename T1, typename T2, typename T3>
    void push_functor(lua_State *L, RVal(*func)(T1, T2, T3))
    {
        lua_pushcclosure(L, functor<RVal, T1, T2, T3>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4>
    void push_functor(lua_State *L, RVal(*func)(T1, T2, T3, T4))
    {
        lua_pushcclosure(L, functor<RVal, T1, T2, T3, T4>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5>
    void push_functor(lua_State *L, RVal(*func)(T1, T2, T3, T4, T5))
    {
        lua_pushcclosure(L, functor<RVal, T1, T2, T3, T4, T5>::invoke, 1);
    }

    // member variable
    struct var_base
    {
        virtual void get(lua_State *L) = 0;
        virtual void set(lua_State *L) = 0;
    };

    template<typename T, typename V>
    struct mem_var : var_base
    {
        V T::*_var;
        mem_var(V T::*val) : _var(val) {}
        void get(lua_State *L)	
        { 
          try
          {
            push<if_<is_obj<V>::value && !std::is_pointer<V>::value, V&, V>::type>(L, read<T*>(L, 1)->*(_var));
          }
          catch (std::exception e) { }
        }
        void set(lua_State *L)	
        { 
          try
          {
            read<T*>(L, 1)->*(_var) = read<V>(L, 3);
          }
          catch (std::exception e) {}
        }
    };


    // class member functor (with return value)
    template<typename RVal, typename T, typename ...Args>
    struct mem_functor
    {
        static int invoke(lua_State *L) 
        { 
          try { return invoker(L, build_indices<sizeof...(Args)>{}); }
          catch (std::exception e) { return 0; }
        }
        template<std::size_t... Is> static int invoker(lua_State* L, indices<Is...>) { push(L, (read<T*>(L, 1)->*upvalue_<RVal(T::*)(Args...)>(L))(read<RefCorrector<Args>>(L, Is + 2)...)); return 1; }
    };

    template<typename T, typename ...Args>
    struct mem_functor<void, T, Args...>
    {
        static int invoke(lua_State *L) 
        { 
          try { return invoker(L, build_indices<sizeof...(Args)>{}); }
          catch (std::exception e) { return 0; }
        }
        template<std::size_t... Is> static int invoker(lua_State* L, indices<Is...>) { (read<T*>(L, 1)->*upvalue_<void(T::*)(Args...)>(L))(read<RefCorrector<Args>>(L, Is + 2)...); return 0; }
    };


    template<typename RVal, typename T>
    struct mem_functor<RVal, T>
    {
      static int invoke(lua_State *L) 
      { 
        try
        {
          push(L, (read<T*>(L, 1)->*upvalue_<RVal(T::*)(void)>(L))()); 
          return 1;
        }
        catch (std::exception e)
        {
          return 0;
        }
      }
    };

    template<typename T>
    struct mem_functor<void, T>
    {
        static int invoke(lua_State *L) 
        { 
          try
          {
            (read<T*>(L, 1)->*upvalue_<void(T::*)(void)>(L))();
            return 0;
          }
          catch(std::exception e)
          {
            return 0;
          }
        }
    };

    // class member functor (non-managed)
    template<typename T, typename T1>
    struct mem_functor<int, T, lua_State*, T1>
    {
        static int invoke(lua_State *L) 
        { 
          try
          {
            return (read<T*>(L, 1)->*upvalue_<int(T::*)(lua_State*, T1)>(L))(L, read<T1>(L, 2));
          }
          catch (std::exception e)
          {
            return 0;
          }
        }
    };

    template<typename T>
    struct mem_functor<int, T, lua_State*>
    {
        static int invoke(lua_State *L) 
        {
          try
          {
            return (read<T*>(L, 1)->*upvalue_<int(T::*)(lua_State*)>(L))(L);
          }
          catch (std::exception e)
          {
            return 0;
          }
        }
    };

    // push_functor
    template<typename RVal, typename T>
    void push_functor(lua_State *L, RVal(T::*func)())
    {
        lua_pushcclosure(L, mem_functor<RVal, T>::invoke, 1);
        (void)func;
    }

    template<typename RVal, typename T>
    void push_functor(lua_State *L, RVal(T::*func)() const)
    {
        lua_pushcclosure(L, mem_functor<RVal, T>::invoke, 1);
        (void)func;
    }

    template<typename RVal, typename T, typename ...Args>
    void push_functor(lua_State *L, RVal(T::*func)(Args...))
    {
        lua_pushcclosure(L, mem_functor<RVal, T, Args...>::invoke, 1);
        (void)func;
    }

    template<typename RVal, typename T, typename ...Args>
    void push_functor(lua_State *L, RVal(T::*func)(Args...) const)
    {
        lua_pushcclosure(L, mem_functor<RVal, T, Args...>::invoke, 1);
    }

    template<typename T, typename ...Args>
    struct ConstructorInvoker
    {
      template<std::size_t... Is> static int invoker(lua_State* L, indices<Is...>) 
      { 
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<RefCorrector<Args>>(L, Is + 2)...);
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);
        return 1;
      }
      static int invoke(lua_State *L) 
      { 
        try
        {
          return invoker(L, build_indices<sizeof...(Args)>{});
        }
        catch (std::exception e)
        {
          return 0;
        }
      }
    };

    // constructor
    template<typename T, typename ...Args>
    int constructor(lua_State *L)
    {
      return ConstructorInvoker<T, Args...>::invoke(L);
    }

    // destroyer
    template<typename T>
    int destroyer(lua_State *L)
    {
        ((user*)lua_touserdata(L, 1))->~user();
        return 0;
    }

    // global function
    template<typename F>
    void def(lua_State* L, const char* name, F func)
    {
        lua_pushstring(L, name);
        lua_pushlightuserdata(L, *(void**)&func);
        push_functor(L, func);
        lua_settable(L, LUA_GLOBALSINDEX);
        (void)func;
    }

    // global variable
    template<typename T>
    void set(lua_State* L, const char* name, T object)
    {
        lua_pushstring(L, name);
        push(L, object);
        lua_settable(L, LUA_GLOBALSINDEX);
    }

    template<typename T>
    T get(lua_State* L, const char* name)
    {
        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        return pop<T>(L);
    }

    template<typename T>
    void decl(lua_State* L, const char* name, T object)
    {
        set(L, name, object);
    }

    // call
    template<typename RVal>
    RVal call(lua_State* L, const char* name)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);

        if (lua_isfunction(L, -1))
        {
            lua_pcall(L, 0, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1>
    RVal call(lua_State* L, const char* name, T1 arg)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if (lua_isfunction(L, -1))
        {
            push(L, arg);
            lua_pcall(L, 1, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if (lua_isfunction(L, -1))
        {
            push(L, arg1);
            push(L, arg2);
            lua_pcall(L, 2, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if (lua_isfunction(L, -1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            lua_pcall(L, 3, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if (lua_isfunction(L, -1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            push(L, arg4);
            lua_pcall(L, 4, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }


    // class init
    template<typename T>
    void class_add(lua_State* L, const char* name)
    {
        class_name<T>::name(name);

        lua_pushstring(L, name);
        lua_newtable(L);

        lua_pushstring(L, "__name");
        lua_pushstring(L, name);
        lua_rawset(L, -3);

        lua_pushstring(L, "TryGet");
        lua_pushcclosure(L, meta_get_no_error, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__index");
        lua_pushcclosure(L, meta_get, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__newindex");
        lua_pushcclosure(L, meta_set, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__gc");
        lua_pushcclosure(L, destroyer<T>, 0);
        lua_rawset(L, -3);

        lua_settable(L, LUA_GLOBALSINDEX);
    }

    // Tinker Class Inheritence
    template<typename T, typename P>
    void class_inh(lua_State* L)
    {
        push_meta(L, class_name<T>::name());
        if (lua_istable(L, -1))
        {
            lua_pushstring(L, "__parent");
            push_meta(L, class_name<P>::name());
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // Tinker Class Constructor
    template<typename T, typename F>
    void class_con(lua_State* L, F func)
    {
        push_meta(L, class_name<T>::name());
        if (lua_istable(L, -1))
        {
            lua_newtable(L);
            lua_pushstring(L, "__call");
            lua_pushcclosure(L, func, 0);
            lua_rawset(L, -3);
            lua_setmetatable(L, -2);
        }
        lua_pop(L, 1);
        (void)func;
    }

    // Tinker Class Functions
    template<typename T, typename F>
    void class_def(lua_State* L, const char* name, F func)
    {
        push_meta(L, class_name<T>::name());
        if (lua_istable(L, -1))
        {
            lua_pushstring(L, name);
            new(lua_newuserdata(L, sizeof(F))) F(func);
            push_functor(L, func);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // Tinker Class Variables
    template<typename T, typename BASE, typename VAR>
    void class_mem(lua_State* L, const char* name, VAR BASE::*val)
    {
        push_meta(L, class_name<T>::name());
        if (lua_istable(L, -1))
        {
            lua_pushstring(L, name);
            new(lua_newuserdata(L, sizeof(mem_var<BASE, VAR>))) mem_var<BASE, VAR>(val);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }


    template<typename T>
    struct class_name
    {
        // global name
        static const char* name(const char* name = NULL)
        {
            static char temp[256] = "";
            if (name) std::strcpy(temp, name);
            return temp;
        }
    };

    // Table Object on Stack
    struct table_obj
    {
        table_obj(lua_State* L, int index);
        ~table_obj();

        void inc_ref();
        void dec_ref();

        bool validate();

        template<typename T>
        void set(const char* name, T object)
        {
            if (validate())
            {
                lua_pushstring(m_L, name);
                push(m_L, object);
                lua_settable(m_L, m_index);
            }
        }

        template<typename T>
        T get(const char* name)
        {
            if (validate())
            {
                lua_pushstring(m_L, name);
                lua_gettable(m_L, m_index);
            }
            else
            {
                lua_pushnil(m_L);
            }

            return pop<T>(m_L);
        }

        lua_State*		m_L;
        int				m_index;
        const void*		m_pointer;
        int				m_ref;
    };

    // Table Object Holder
    struct table
    {
        table(lua_State* L);
        table(lua_State* L, int index);
        table(lua_State* L, const char* name);
        table(const table& input);
        ~table();

        template<typename T>
        void set(const char* name, T object)
        {
            m_obj->set(name, object);
        }

        template<typename T>
        T get(const char* name)
        {
            return m_obj->get<T>(name);
        }

        table_obj*		m_obj;
    };


} // namespace lua_tinker


#endif //_LUA_TINKER_H_