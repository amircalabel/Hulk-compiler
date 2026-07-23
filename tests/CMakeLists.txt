# tests/CMakeLists.txt
add_executable(hulk_tests
    test_scanner.cpp
    test_parser.cpp
    test_ast.cpp
    test_vm.cpp
)

target_link_libraries(hulk_tests hulk)

add_test(NAME ScannerTests COMMAND hulk_tests --scanner)
add_test(NAME ParserTests COMMAND hulk_tests --parser)
add_test(NAME ASTTests COMMAND hulk_tests --ast)
add_test(NAME VMTests COMMAND hulk_tests --vm)
