#include <gtest/gtest.h>
#include <dependency_resolver.hpp>

using jaszyk::dependency_resolver;

class DependencyResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        
    }

    void TearDown() override {

    }

    dependency_resolver resolver;
};


class BaseClass {
public:
    BaseClass() = default;
    virtual ~BaseClass() = default;
    virtual int get_value() = 0;
    virtual void increment() = 0;
};

class DerivedClass : public BaseClass {
    std::shared_ptr<int> counter_;
public:
    DerivedClass(std::shared_ptr<int> counter) 
        : counter_(counter) 
    {
    }

    virtual ~DerivedClass() = default;

    int get_value() override {
        return *counter_;
    }

    void increment() override {
        (*counter_)++;
    }
};

class Controller {
    std::shared_ptr<BaseClass> base_;
public:
    Controller(std::shared_ptr<BaseClass> base) 
        : base_(base)
    { }

    void increment() {
        base_->increment();
    }

    int get_value() {
        return base_->get_value();
    }
};

TEST_F(DependencyResolverTest, TestSimpleSingleton) {
    resolver.add_singleton(10);

    ASSERT_EQ(resolver.resolve<DerivedClass>()->get_value(), 10);
}

TEST_F(DependencyResolverTest, TestSimpleSingletonWithDependency) {
    resolver.add_singleton(1);
    resolver.add_singleton<BaseClass, DerivedClass>();

    auto c1 = resolver.resolve<Controller>();

    ASSERT_EQ(c1->get_value(), 1);
    c1->increment();

    auto c2 = resolver.resolve<Controller>();
    ASSERT_EQ(c2->get_value(), 2);
}

TEST_F(DependencyResolverTest, TestTransientWithDependency) {
    resolver.add_transient<int>();
    resolver.add_transient<BaseClass, DerivedClass>();

    auto c1 = resolver.resolve<Controller>();

    ASSERT_EQ(c1->get_value(), 0);
    c1->increment();

    auto c2 = resolver.resolve<Controller>();
    ASSERT_EQ(c2->get_value(), 0);
}

TEST_F(DependencyResolverTest, TestScopedWithDependency) {
    resolver.add_scoped<int>();
    resolver.add_scoped<BaseClass, DerivedClass>();

    auto scope = resolver.make_scope();

    auto c1 = resolver.resolve<Controller>(scope);

    ASSERT_EQ(c1->get_value(), 0);
    c1->increment();

    auto c2 = resolver.resolve<Controller>(scope);
    ASSERT_EQ(c2->get_value(), 1);

    auto other_scope = resolver.make_scope();

    auto c3 = resolver.resolve<Controller>(other_scope);
    ASSERT_EQ(c3->get_value(), 0);
}

class BaseService2 {
public:
    BaseService2() = default;
    virtual ~BaseService2() = default;
    virtual int get_value() = 0;
    virtual void increment() = 0;
    virtual const std::string& get_text() const = 0;
};

class DerivedService2 : public BaseService2 {
    std::shared_ptr<int> counter_;
    std::shared_ptr<std::string> text_;
    std::shared_ptr<BaseClass> base_;
public:
    DerivedService2(std::shared_ptr<std::string> text, std::shared_ptr<int> counter, std::shared_ptr<BaseClass> base) 
        : counter_(counter) 
    {
    }

    virtual ~DerivedService2() = default;

    int get_value() override {
        return *counter_;
    }

    void increment() override {
        (*counter_)++;
    }

    const std::string& get_text() const override {
        return *text_;
    }
};

class Controller2 {
    std::shared_ptr<BaseService2> base_;
    std::shared_ptr<std::string> text_;
public:
    Controller2(std::shared_ptr<BaseService2> base, std::shared_ptr<std::string> text) 
        : base_(base)
        , text_(text)
    { }

    void increment() {
        base_->increment();
    }

    int get_value() {
        return base_->get_value();
    }

    const std::string& get_text() const {
        return *text_;
    }
};

TEST_F(DependencyResolverTest, TestComplexDependencies) {
    resolver.add_singleton(std::string("Hello World"));
    resolver.add_singleton(150);

    resolver.add_scoped<BaseClass, DerivedClass>();
    resolver.add_transient<BaseService2, DerivedService2>();

    auto scope = resolver.make_scope();

    auto c1 = resolver.resolve<Controller2>(scope);

    ASSERT_EQ(c1->get_value(), 150);
    c1->increment();

    auto c2 = resolver.resolve<Controller2>(scope);
    ASSERT_EQ(c2->get_value(), 151);

    auto other_scope = resolver.make_scope();

    auto c3 = resolver.resolve<Controller2>(other_scope);
    ASSERT_EQ(c3->get_value(), 151);
    c3->increment();
    ASSERT_EQ(c3->get_text(), std::string("Hello World"));

    auto c4 = resolver.resolve<Controller2>(other_scope);
    ASSERT_EQ(c4->get_value(), 152);
}


// Run the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
