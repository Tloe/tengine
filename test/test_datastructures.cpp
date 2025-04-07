#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "ds_string.h"

#include <catch2/catch_test_macros.hpp>

INIT_ARENA(string_test, 1024);
INIT_ARENA(hashmap_test, 3000);

TEST_CASE("ds_string", "[DS_STRING]") {
  auto a = arena::by_name("string_test");

  String s = string::init(a, "tengine");

  REQUIRE(s == "tengine");
  REQUIRE(s != "nope");

  s += " is cool";
  s += string::init(a, "!!");

  REQUIRE(s == "tengine is cool!!");
}

TEST_CASE("ds_hashmap", "[DS_HASHMAP]") {
  auto a = arena::by_name("hashmap_test");

  auto hm = hashmap::init64<U64>(a);

  hashmap::insert(hm, 0, 142);
  hashmap::insert(hm, 1, 152);
  hashmap::insert(hm, 2, 162);
  hashmap::insert(hm, 3, 172);
  hashmap::insert(hm, 4, 182);
  hashmap::insert(hm, 5, 142);
  hashmap::insert(hm, 6, 152);
  hashmap::insert(hm, 7, 162);
  hashmap::insert(hm, 8, 172);
  hashmap::insert(hm, 9, 182);
  hashmap::erase(hm, 5);
  hashmap::erase(hm, 6);
  hashmap::erase(hm, 7);
  hashmap::erase(hm, 8);
  hashmap::erase(hm, 9);
  hashmap::insert(hm, 10, 182);
  hashmap::insert(hm, 11, 182);
  hashmap::insert(hm, 12, 182);
  hashmap::insert(hm, 13, 182);
  hashmap::insert(hm, 14, 182);
  hashmap::insert(hm, 15, 182);
  hashmap::insert(hm, 16, 182);
  hashmap::insert(hm, 17, 182);
  hashmap::insert(hm, 18, 182);
  hashmap::insert(hm, 19, 182);
  hashmap::insert(hm, 20, 182);
  hashmap::insert(hm, 288, 182);

  REQUIRE(hm._size == 17);
  REQUIRE(hm._capacity == 32);
  REQUIRE(*hashmap::value(hm, 0) == 142);
  REQUIRE(*hashmap::value(hm, 1) == 152);
  REQUIRE(*hashmap::value(hm, 2) == 162);
  REQUIRE(*hashmap::value(hm, 3) == 172);
  REQUIRE(*hashmap::value(hm, 4) == 182);
  REQUIRE(*hashmap::value(hm, 10) == 182);
  REQUIRE(*hashmap::value(hm, 11) == 182);
  REQUIRE(*hashmap::value(hm, 12) == 182);
  REQUIRE(*hashmap::value(hm, 13) == 182);
  REQUIRE(*hashmap::value(hm, 14) == 182);
  REQUIRE(*hashmap::value(hm, 15) == 182);
  REQUIRE(*hashmap::value(hm, 16) == 182);
  REQUIRE(*hashmap::value(hm, 17) == 182);
  REQUIRE(*hashmap::value(hm, 18) == 182);
  REQUIRE(*hashmap::value(hm, 19) == 182);
  REQUIRE(*hashmap::value(hm, 20) == 182);
  REQUIRE(*hashmap::value(hm, 288) == 182);

  hashmap::erase(hm, 424242);

  REQUIRE(hm._size == 17);
  REQUIRE(hm._capacity == 32);
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
  auto a = arena::by_name("hashmap_test");

  auto da = array::init<String>(a);

  auto s0 = string::init(a, "hello0");

  array::push_back(da, s0);
  array::push_back(da, string::init(a, "hello1"));
  array::push_back(da, string::init(a, "hello2"));

  REQUIRE(da._size == 3);
}

TEST_CASE("ds_sparse_array_satic", "[DS_SPARSE_ARRAY_STATIC]") {}
