#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "ds_sparse_array.h"
#include "ds_string.h"

#include <catch2/catch_test_macros.hpp>

ARENA_INIT(string_test, 1024);
ARENA_INIT(hashmap_test, 3000);
ARENA_INIT(sparse_test, 3000);

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

TEST_CASE("ds_sparse_array_static", "[DS_SPARSE_ARRAY_STATIC]") {
  auto a = arena::by_name("sparse_test");

  auto sa = sparse::init8<int, 4, 8>(a);

  SECTION("Initially empty") {
    for (U8 i = 0; i < 8; ++i) {
      REQUIRE_FALSE(sparse::has(sa, i));
      REQUIRE(sparse::value(sa, i) == nullptr);
    }
  }

  SECTION("Insert values and read back") {
    REQUIRE(sparse::insert(sa, U8(2), 42));
    REQUIRE(sparse::insert(sa, U8(5), 99));
    REQUIRE(sparse::has(sa, U8(2)));
    REQUIRE(sparse::has(sa, U8(5)));
    REQUIRE(*sparse::value(sa, U8(2)) == 42);
    REQUIRE(*sparse::value(sa, U8(5)) == 99);
    REQUIRE(sa._size == 2);
  }

  SECTION("Overwriting value at same id") {
    REQUIRE(sparse::insert(sa, U8(3), 10));
    REQUIRE(*sparse::value(sa, U8(3)) == 10);
    REQUIRE(sparse::insert(sa, U8(3), 77)); // overwrite
    REQUIRE(sa._size == 1);                 // still only 1 element
    REQUIRE(*sparse::value(sa, U8(3)) == 77);
  }

  SECTION("Remove element and compact") {
    REQUIRE(sparse::insert(sa, U8(1), 100));
    REQUIRE(sparse::insert(sa, U8(2), 200));
    sparse::remove(sa, U8(1));
    REQUIRE_FALSE(sparse::has(sa, U8(1)));
    REQUIRE(sa._size == 1);
    REQUIRE(sparse::has(sa, U8(2)));
    REQUIRE(*sparse::value(sa, U8(2)) == 200);
  }

  SECTION("next_id returns correct and wraps around") {
    std::vector<U8> ids;
    for (int i = 0; i < 4; ++i) {
      auto id = sparse::next_id(sa);
      REQUIRE(id < 8);
      ids.push_back(id);
      REQUIRE(sparse::insert(sa, id, i * 10));
    }

    REQUIRE(sparse::next_id(sa) == core::max_type<U8>()); // max capacity reached

    // remove one, then next_id should reuse that
    sparse::remove(sa, ids[1]);
    auto reused = sparse::next_id(sa);
    REQUIRE(sparse::has(sa, reused) == false);
  }

  SECTION("Insert fails when id is out of range") { REQUIRE_FALSE(sparse::insert(sa, U8(100), 1)); }

  SECTION("Remove does nothing on invalid id") {
    sparse::remove(sa, U8(6)); // nothing crashes
  }

  SECTION("value returns nullptr if not present") {
    REQUIRE(sparse::value(sa, U8(0)) == nullptr);
    REQUIRE(sparse::insert(sa, U8(0), 5));
    REQUIRE(sparse::value(sa, U8(0)) != nullptr);
  }

  SECTION("Insert -> Remove -> Insert -> Validate correctness") {
    REQUIRE(sparse::insert(sa, U8(0), 10));
    REQUIRE(sparse::insert(sa, U8(1), 20));
    REQUIRE(sparse::insert(sa, U8(2), 30));
    REQUIRE(sparse::insert(sa, U8(3), 40));
    REQUIRE(sa._size == 4);

    // Remove two items
    sparse::remove(sa, U8(1));
    sparse::remove(sa, U8(2));
    REQUIRE_FALSE(sparse::has(sa, U8(1)));
    REQUIRE_FALSE(sparse::has(sa, U8(2)));
    REQUIRE(sa._size == 2);

    // Insert two new values
    REQUIRE(sparse::insert(sa, U8(4), 50));
    REQUIRE(sparse::insert(sa, U8(5), 60));
    REQUIRE(sa._size == 4);

    // Check all current values
    REQUIRE(sparse::has(sa, U8(0)));
    REQUIRE(*sparse::value(sa, U8(0)) == 10);

    REQUIRE_FALSE(sparse::has(sa, U8(1)));
    REQUIRE_FALSE(sparse::value(sa, U8(1)));

    REQUIRE_FALSE(sparse::has(sa, U8(2)));
    REQUIRE_FALSE(sparse::value(sa, U8(2)));

    REQUIRE(sparse::has(sa, U8(3)));
    REQUIRE(*sparse::value(sa, U8(3)) == 40);

    REQUIRE(sparse::has(sa, U8(4)));
    REQUIRE(*sparse::value(sa, U8(4)) == 50);

    REQUIRE(sparse::has(sa, U8(5)));
    REQUIRE(*sparse::value(sa, U8(5)) == 60);
  }
}
