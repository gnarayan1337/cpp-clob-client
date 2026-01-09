# Test suite

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executables
add_executable(test_eip712 test_eip712.cpp)
target_link_libraries(test_eip712 PRIVATE clob_client gtest_main)

add_executable(test_signer test_signer.cpp)
target_link_libraries(test_signer PRIVATE clob_client gtest_main)

add_executable(test_utilities test_utilities.cpp)
target_link_libraries(test_utilities PRIVATE clob_client gtest_main)

add_executable(test_order_builder test_order_builder.cpp)
target_link_libraries(test_order_builder PRIVATE clob_client gtest_main)

# Comprehensive test suites matching Rust client
add_executable(test_endpoints_comprehensive test_endpoints_comprehensive.cpp)
target_link_libraries(test_endpoints_comprehensive PRIVATE clob_client gtest_main)

add_executable(test_authentication_flows test_authentication_flows.cpp)
target_link_libraries(test_authentication_flows PRIVATE clob_client gtest_main)

add_executable(test_order_lifecycle test_order_lifecycle.cpp)
target_link_libraries(test_order_lifecycle PRIVATE clob_client gtest_main)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_eip712)
gtest_discover_tests(test_signer)
gtest_discover_tests(test_utilities)
gtest_discover_tests(test_order_builder)

# Order serialization tests
add_executable(test_order_serialization test_order_serialization.cpp)
target_link_libraries(test_order_serialization PRIVATE clob_client GTest::gtest_main)
gtest_discover_tests(test_order_serialization)

# Mock authentication tests
add_executable(test_mock_authentication test_mock_authentication.cpp)
target_link_libraries(test_mock_authentication PRIVATE clob_client GTest::gtest_main)
gtest_discover_tests(test_mock_authentication)

# Mock order posting tests
add_executable(test_mock_order_posting test_mock_order_posting.cpp)
target_link_libraries(test_mock_order_posting PRIVATE clob_client GTest::gtest_main)
gtest_discover_tests(test_mock_order_posting)
gtest_discover_tests(test_endpoints_comprehensive)
gtest_discover_tests(test_authentication_flows)
gtest_discover_tests(test_order_lifecycle)

# L2 HMAC signature tests (verifies against Rust/Python implementations)
add_executable(test_hmac_l2 test_hmac_l2.cpp)
target_link_libraries(test_hmac_l2 PRIVATE clob_client GTest::gtest_main)
gtest_discover_tests(test_hmac_l2)


