
#include "catch.hpp"

#include <raspberry/raspberry.hpp>

#include <string>

RASPBERRY_DECL_METHOD(FuncConcept, func);
RASPBERRY_DECL_METHOD(SquareConcept, square);

using AnyFunc = raspberry::Any<FuncConcept<int()const>,SquareConcept<float(float)>>;

struct SomeFunc {
    int func() const {
        return 42;
    }

    float square(float x) {
        return x*x;
    }
};

TEST_CASE("Objects can be stored in Any", "[raspberry]") {
    AnyFunc f = SomeFunc{};

    REQUIRE(f.func() == 42);
    REQUIRE(f.square(12) == 144);
}

struct negative_test_assign {
    template <typename AF>
    static decltype( AF(std::declval<const AF&>()), bool{} ) test(int) { return false; }

    template <typename AF>
    static bool test(long) { return true; }
};

TEST_CASE("Any cannot be stored in Any or copied", "[raspberry]") {
    REQUIRE(negative_test_assign::test<AnyFunc>(int{}));
}

RASPBERRY_DECL_METHOD(RefDetectConcept, ref_detect);

using AnyRefDetector = raspberry::Any<RefDetectConcept<void(int)>>;

struct RefDetector {
    int value = 0;

    void ref_detect(int x) {
        value = x;
    }
};

TEST_CASE("Objects are copied by default", "[raspberry]") {
    RefDetector rd;
    REQUIRE(rd.value == 0);

    AnyRefDetector ard = rd;
    REQUIRE(rd.value == 0);

    ard.ref_detect(42);
    REQUIRE(rd.value == 0);
}

TEST_CASE("std::reference_wrapper is used to capture by reference", "[raspberry]") {
    RefDetector rd;
    REQUIRE(rd.value == 0);

    AnyRefDetector ard = std::ref(rd);
    REQUIRE(rd.value == 0);

    ard.ref_detect(42);
    REQUIRE(rd.value == 42);
}

RASPBERRY_DECL_METHOD(SetStringConcept, set_string);

using AnySetString = raspberry::Any<
        SetStringConcept< void(const std::string&) >,
        SetStringConcept< void(const char*) >
>;

struct StringSetter {
    std::string value;

    void set_string(const std::string& s) {
        value = s;
    }

    void set_string(const char* s) {
        value = s;
    }
};

TEST_CASE("Methods can be overloaded", "[raspberry]") {
    StringSetter s;
    AnySetString a = std::ref(s);

    a.set_string("char[]");
    REQUIRE(s.value == "char[]");

    a.set_string(std::string("std::string"));
    REQUIRE(s.value == "std::string");

}

RASPBERRY_DECL_METHOD(MaybeConstGetter, get);

using AnyMaybeConstGetter = raspberry::Any<
        MaybeConstGetter< int&() >,
        MaybeConstGetter< const int&() const >
>;

struct SomeMaybeConstGetter {
    int value;
    int& get() { return value; }
    const int& get() const { return value; }
};

TEST_CASE("Const and non-const overloads can coexist", "[raspberry]") {
    SomeMaybeConstGetter s;
    AnyMaybeConstGetter a = std::ref(s);

    s.value = 7;
    REQUIRE(s.value == 7);

    a.get() = 42;
    REQUIRE(s.value == 42);

    const auto& ac = a;
    REQUIRE(ac.get() == 42);
    REQUIRE(std::is_const<std::remove_reference_t<decltype(ac.get())>>::value);
}

using AnyMaybeConstGetterReversed = raspberry::Any<
        MaybeConstGetter< const int&() const >,
        MaybeConstGetter< int&() >
>;

TEST_CASE("Const and non-const overloads can come in any order", "[raspberry]") {
    SomeMaybeConstGetter s;
    AnyMaybeConstGetterReversed a = std::ref(s);

    s.value = 7;
    REQUIRE(s.value == 7);

    a.get() = 42;
    REQUIRE(s.value == 42);

    const auto& ac = a;
    REQUIRE(ac.get() == 42);
    REQUIRE(std::is_const<std::remove_reference_t<decltype(ac.get())>>::value);
}

RASPBERRY_DECL_METHOD(ConstTester, c_func);

using AnyConstTester = raspberry::Any< ConstTester< void() > >;

struct SomeConstTester {
    void c_func() const {}
};

TEST_CASE("Const methods can be called from non-const concepts", "[raspberry]") {
    AnyConstTester ac = SomeConstTester{};
    ac.c_func();
    REQUIRE(true);
}

RASPBERRY_DECL_METHOD(ConversionTester, test);

using AnyConversionTester = raspberry::Any< ConversionTester< int(double) > >;

struct SomeConversionTester {
    double test(double d) const { return d; }
};

TEST_CASE("Method return values follow implicit conversion through concepts", "[raspberry]") {
    SomeConversionTester s;
    double d = 7.42;
    REQUIRE(s.test(d) == 7.42);

    AnyConversionTester a = s;
    REQUIRE(a.test(d) == 7);
}

struct RecAnyFunc final : raspberry::Any<
    FuncConcept<int(RecAnyFunc&)>
> { using Any::Any; };

struct RecAnyTester {
    int x;
    int func(RecAnyFunc&) { return x; }
};

TEST_CASE("Any can be used for recursive CRTP", "[raspberry]") {
    RecAnyFunc rat = RecAnyTester{7};
    REQUIRE(rat.func(rat) == 7);

    rat = RecAnyTester{42};
    REQUIRE(rat.func(rat) == 42);

    RecAnyFunc rat2 = std::move(rat);
    rat = RecAnyTester{13};
    REQUIRE(rat.func(rat) == 13);
    REQUIRE(rat2.func(rat2) == 42);
};

struct RecAnyFuncValue final : raspberry::Any<
    FuncConcept<int(RecAnyFuncValue)>
> { using Any::Any; };

struct RecAnyValueTester {
    int x;
    int func(RecAnyFuncValue) { return x; }
};

TEST_CASE("Recursive Any concepts can accept Any value types", "[raspberry]") {
    RecAnyFuncValue rat1 = RecAnyValueTester{7};
    RecAnyFuncValue rat2 = RecAnyValueTester{42};
    REQUIRE(rat1.func(std::move(rat2)) == 7);
};

using AnyFuncBase = raspberry::Any<FuncConcept<int()const>>;

using AnySquareBase = raspberry::Any<SquareConcept<float(float)>>;

using AnyFuncSquare = raspberry::Any<AnyFuncBase,AnySquareBase>;

TEST_CASE("Anys can be used as a base concepts", "[raspberry]") {
    AnyFuncSquare f = SomeFunc{};

    REQUIRE(f.func() == 42);
    REQUIRE(f.square(12) == 144);
}

TEST_CASE("Derived Anys can be upcast to base Anys", "[raspberry]") {
    AnyFuncSquare f = SomeFunc{};
    AnyFuncBase f2 = std::move(f);

    REQUIRE(f2.func() == 42);
}

using AnyFunc1 = raspberry::Any<FuncConcept<int()const>>;
using AnyFunc2 = raspberry::Any<FuncConcept<int()const>, SquareConcept<float(float)>>;

TEST_CASE("Conversion between unrelated Anys is possible", "[raspberry]") {
    AnyFunc2 f1 = SomeFunc{};
    AnyFunc1 f2 = std::move(f1);

    REQUIRE(f2.func() == 42);
}

using AnyFuncAllOverloads = raspberry::Any<
    FuncConcept<int()>,
    FuncConcept<int()const>
>;

using AnyFuncAllRefOverloads = raspberry::Any<
    FuncConcept<int()&>,
    FuncConcept<int()const&>,
    FuncConcept<int()&&>,
    FuncConcept<int()const&&>
>;

struct FuncAllOverloadsTest {
    int func() { return 1; }
    int func() const { return 2; }
};

struct FuncAllRefOverloadsTest {
    int func() & { return 3; }
    int func() const & { return 4; }
    int func() && { return 5; }
    int func() const && { return 6; }
};

TEST_CASE("Concepts support all forms of overloading", "[raspberry]") {
    AnyFuncAllOverloads afao = FuncAllOverloadsTest{};
    AnyFuncAllRefOverloads afaro = FuncAllRefOverloadsTest{};

    REQUIRE(static_cast<AnyFuncAllOverloads&>(afao).func() == 1);
    REQUIRE(static_cast<const AnyFuncAllOverloads&>(afao).func() == 2);
    REQUIRE(static_cast<AnyFuncAllRefOverloads&>(afaro).func() == 3);
    REQUIRE(static_cast<const AnyFuncAllRefOverloads&>(afaro).func() == 4);
    REQUIRE(static_cast<AnyFuncAllRefOverloads&&>(afaro).func() == 5);
    REQUIRE(static_cast<const AnyFuncAllRefOverloads&&>(afaro).func() == 6);
};

TEST_CASE("Pointers are captured by value and dereferenced", "[raspberry]") {
    RefDetector rd;
    REQUIRE(rd.value == 0);

    AnyRefDetector ard = &rd;
    REQUIRE(rd.value == 0);

    ard.ref_detect(42);
    REQUIRE(rd.value == 42);
}

TEST_CASE("std::unique_ptr is captured by value and dereferenced", "[raspberry]") {
    auto rdp = std::make_unique<RefDetector>();
    auto* rd = rdp.get();
    REQUIRE(rd->value == 0);

    AnyRefDetector ard = std::move(rdp);
    REQUIRE(rd->value == 0);

    ard.ref_detect(42);
    REQUIRE(rd->value == 42);
}

TEST_CASE("std::shared_ptr is captured by value and dereferenced", "[raspberry]") {
    auto rd = std::make_shared<RefDetector>();
    REQUIRE(rd->value == 0);

    AnyRefDetector ard = rd;
    REQUIRE(rd->value == 0);

    ard.ref_detect(42);
    REQUIRE(rd->value == 42);
}
