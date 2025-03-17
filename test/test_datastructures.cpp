#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "ds_string.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ds_string", "[DS_STRING]") {
  auto a = INIT_ARENA("string_test", 1024);

  String s = string::init(a, "tengine");

  REQUIRE(s == "tengine");
  REQUIRE(s != "nope");

  s += " is cool";
  s += string::init(a, "!!");

  REQUIRE(s == "tengine is cool!!");
}

TEST_CASE("ds_hashmap", "[DS_HASHMAP]") {
  auto a = INIT_ARENA("hasmap_test", 1024);

  auto hm = hashmap::init64<U64>(a);

  hashmap::insert(hm, 42, 142);
  hashmap::insert(hm, 52, 152);
  hashmap::insert(hm, 62, 162);
  hashmap::insert(hm, 72, 172);
  hashmap::insert(hm, 82, 182);

  REQUIRE(hm._size == 5);
  REQUIRE(hm._capacity == 8);
  REQUIRE(*hashmap::value(hm, 62) == 162);
  REQUIRE(hashmap::contains(hm, 42));

  hashmap::erase(hm, 42);

  REQUIRE(hm._size == 4);
  REQUIRE(hm._capacity == 8);
  REQUIRE(hashmap::value(hm, 42) == nullptr);
  REQUIRE(!hashmap::contains(hm, 42));

  auto hm1 = hashmap::initString<U64>(a);
  hashmap::insert(hm1, string::init(a, "hello0"), 40);
  hashmap::insert(hm1, string::init(a, "hello1"), 41);
  hashmap::insert(hm1, string::init(a, "hello2"), 42);
  hashmap::insert(hm1, string::init(a, "hello3"), 43);
  hashmap::insert(hm1, string::init(a, "hello4"), 44);
  hashmap::insert(hm1, string::init(a, "hello5"), 45);
  hashmap::insert(hm1, string::init(a, "hello6"), 46);
  hashmap::insert(hm1, string::init(a, "hello7"), 47);
  hashmap::insert(hm1, string::init(a, "hello8"), 48);
  hashmap::insert(hm1, string::init(a, "hello9"), 49);
  hashmap::insert(hm1, string::init(a, "hello10"), 50);

  REQUIRE(hashmap::contains(hm1, string::init(a, "hello0")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello1")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello2")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello3")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello4")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello5")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello6")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello7")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello8")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello9")));
  REQUIRE(hashmap::contains(hm1, string::init(a, "hello10")));

  REQUIRE(hm1._size == 11);
  REQUIRE(hm1._capacity == 16);

  REQUIRE(*hashmap::value(hm1, string::init(a, "hello0")) == 40);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello1")) == 41);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello2")) == 42);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello3")) == 43);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello4")) == 44);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello5")) == 45);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello6")) == 46);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello7")) == 47);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello8")) == 48);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello9")) == 49);
  REQUIRE(*hashmap::value(hm1, string::init(a, "hello10")) == 50);
}

TEST_CASE("ds_array_dynamic", "[DS_DYNAMICARRAY]") {
  auto a = INIT_ARENA("hasmap_test", 1024);

  auto da = array::init<String>(a);

  auto s0 = string::init(a, "hello0");

  array::push_back(da, s0);
  array::push_back(da, string::init(a, "hello1"));
  array::push_back(da, string::init(a, "hello2"));

  REQUIRE(da._size == 3);
}

TEST_CASE("ds_sparse_array_satic", "[DS_SPARSE_ARRAY_STATIC]") {}
