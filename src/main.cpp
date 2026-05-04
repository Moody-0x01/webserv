#include <Server.hpp>
#include <cstddef>
#include <cstring>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

void test_chunk_context(void);

// char chunked_data[] = "a\r\n1234567890\r\n4\r\nWiki\r\n3\r\nmom\r\n0\r\n\r\n";

int main(int ac, char **av)
// int main()
{
	std::string config_file = DEFAULT_CONF;
	if (ac > 1) config_file = av[1];
	try {
		Config conf = parse_config_file(config_file);
		std::vector<ServerConfig> servers = conf.getservers();
		Multiplexer *multi = Multiplexer::create_multiplexer(servers);
		conf.debug();
		multi->run();
	} catch (std::runtime_error &e) {
		std::cerr << "[ Multiplexer::init ] " << e.what() << "\n";
		return (1);
	} catch (const char *e) {
		std::cerr << "[ Multiplexer::init ] " << e << "\n";
		return (1);
	}
	// test_chunk_context();
}

struct ChunkTest {
    std::string name;
    std::string data;
    uint32_t expected_final_mask;
    bool should_be_error;
};

void run_test(const char* name, const char* raw_str, size_t len, int expected_mask) {
    ChunkContext chunk;
    // Simulate incoming buffer
    std::vector<char> buffer(raw_str, raw_str + len);

    std::cout << "[TEST] " << name << "\n";
    chunk.unpack(buffer);

    bool error_detected = (chunk.status_mask & CHUNK_ERROR);
    
    if (error_detected && (expected_mask == CHUNK_ERROR)) {
        std::cout << "  PASS: Correcty identified error.\n";
    } else if (chunk.status_mask == expected_mask) {
        std::cout << "  PASS: Status matches expected value.\n";
    } else {
        std::cout << "  FAIL: Expected " << expected_mask 
                  << " but got " << chunk.status_mask << "\n";
    }
    std::cout << "---------------------------------------\n";
}

void test_suite() {
    // Test 1: Standard valid chunk
    const char* t1 = "5\r\nHello\r\n0\r\n\r\n";
    run_test("Standard Valid", t1, strlen(t1), CHUNK_COMPLETE);

    // Test 2: Incomplete data (Ends inside the data segment)
    const char* t2 = "5\r\nHel";
    run_test("Incomplete Data", t2, strlen(t2), CHUNK_DATA | CHUNK_TRAILER);

    // Test 3: Malformed size (Not Hex)
    const char* t3 = "Z\r\n";
    run_test("Invalid Hex Size", t3, strlen(t3), CHUNK_ERROR);

    // Test 4: Empty body (Immediate terminator)
    const char* t4 = "0\r\n\r\n";
    run_test("Empty Body", t4, strlen(t4), CHUNK_COMPLETE);
    
    // Test 5: Missing LF after CR
    const char* t5 = "5\r Hello"; 
    run_test("Missing LF", t5, 8, CHUNK_ERROR);

	// Extensions: Valid chunk with Extensions
	// Note: The current implementation does not handle extensions, so this will be treated as an error.
	// In a full implementation, you would want to parse the extensions and not treat them as an error.
	// For this test, we expect an error due to the presence of extensions which are not currently supported.
	// If the implementation is updated to handle extensions, this test should be updated to reflect the expected behavior.
	// const char* t6 = "5;ex1=12781728\r\nHello\r\n0\r\n\r\n";
	// example of extensions: 
	//     1. chunk-size (hex) followed by optional chunk-extension, then CRLF
	//     2. chunk-extension is a semicolon followed by a token and optional value
	//     e.g. "5;ex1=12781728\r\nHello\r\n0\r\n\r\n" where "5" is the chunk size, "ex1=12781728" is the extension, and "Hello" is the chunk data.
    const char* t6 = "6;ex1=12781728\r\nHelloi\r\r\n0\r\n\r\n"; 
    run_test("Missing LF", t6, strlen(t6), CHUNK_COMPLETE);
}

void test_chunk_context(void)
{
	test_suite();
}
