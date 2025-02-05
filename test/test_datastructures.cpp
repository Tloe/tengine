#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include <cstdio>
#include <ds_hashmap.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ds_string", "[DS_STRING]") {
  U8 mem[1024];
  auto a = mem::arena::init(mem, 1024);

  ds::String s = ds::string::init(&a, "tengine");

  REQUIRE(s == "tengine");
  REQUIRE(s != "nope");

  s += " is cool";
  s += ds::string::init(&a, "!!");

  REQUIRE(s == "tengine is cool!!");
}

TEST_CASE("ds_hashmap", "[DS_HASHMAP]") {
  U8 mem[1024];
  auto a = mem::arena::init(mem, 1024);

  auto hm = ds::hashmap::init<U64, U64>(&a);

  ds::hashmap::insert<U64, U64>(hm, 42, 142);
  ds::hashmap::insert<U64, U64>(hm, 52, 152);
  ds::hashmap::insert<U64, U64>(hm, 62, 162);
  ds::hashmap::insert<U64, U64>(hm, 72, 172);
  ds::hashmap::insert<U64, U64>(hm, 82, 182);

  REQUIRE(hm._size == 5);
  REQUIRE(hm._capacity == 8);
  REQUIRE(*ds::hashmap::value<U64, U64>(hm, 62) == 162);
  REQUIRE(ds::hashmap::contains<U64, U64>(hm, 42));

  ds::hashmap::erase<U64, U64>(hm, 42);

  REQUIRE(hm._size == 4);
  REQUIRE(hm._capacity == 8);
  REQUIRE(ds::hashmap::value<U64, U64>(hm, 42) == nullptr);
  REQUIRE(!ds::hashmap::contains<U64, U64>(hm, 42));

  /* auto hm1 = ds::hashmap::init<ds::String, U64>(&a); */
  /*  */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello0"), 40); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello1"), 41); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello2"), 42); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello3"), 43); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello4"), 44); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello5"), 45); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello6"), 46); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello7"), 47); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello8"), 48); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello9"), 49); */
  /* ds::hashmap::insert<ds::String, U64>(hm1, ds::string::init(&a,"hello10"), 50); */
  /*  */
  /*  */
  /* REQUIRE(hm._size == 10); */
  /* REQUIRE(hm._capacity == 16); */
}

TEST_CASE("ds_array_dynamic", "[DS_DYNAMICARRAY]") {
  U8 mem[1024];
  auto a = mem::arena::init(mem, 1024);

  auto da = ds::array::init<ds::String>(&a);
  
  auto s0 =  ds::string::init(&a, "hello0");

  ds::array::push_back(da, s0);
  ds::array::push_back(da, ds::string::init(&a, "hello1"));
  ds::array::push_back(da, ds::string::init(&a, "hello2"));

  REQUIRE( da._size == 3 );
}
