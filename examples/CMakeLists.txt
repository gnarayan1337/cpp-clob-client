# Example programs

# Public endpoints (no authentication required)
add_executable(unauthenticated example_public.cpp)
target_link_libraries(unauthenticated PRIVATE clob_client)

# Authenticated client (API key creation and usage)
add_executable(authenticated example_authenticated.cpp)
target_link_libraries(authenticated PRIVATE clob_client)

# Place an order
add_executable(place_order example_place_order.cpp)
target_link_libraries(place_order PRIVATE clob_client)

# On-chain approvals (ERC20/ERC1155)
add_executable(approvals example_approvals.cpp)
target_link_libraries(approvals PRIVATE clob_client)

# Parallel API calls (threads)
add_executable(parallel example_parallel.cpp)
target_link_libraries(parallel PRIVATE clob_client)

# Manual pagination (streaming equivalent)
add_executable(pagination example_streaming_pagination.cpp)
target_link_libraries(pagination PRIVATE clob_client)

# Full trading example
add_executable(trading test_real_trading_simple.cpp)
target_link_libraries(trading PRIVATE clob_client)
