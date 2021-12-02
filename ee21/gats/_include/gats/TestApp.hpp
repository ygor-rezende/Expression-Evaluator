#pragma once
/*!	\file	TestApp.hpp
	\brief	TestApp framework declarations.
	\author	Garth Santor
	\date	2021-10-29
	\copyright	Garth Santor, Trinh Han

=============================================================
TestApp frame declarations.
	TestApp class declaration.
	TestApp::TestCase class declaration.
	TestApp::TestCase::check_equal template implemenation.
	TestApp::TestCase::check_close_epsilon template implemenation.
	GATS_TEST_CASE()
	GATS_TEST_CASE_WEIGHTED()
	GATS_CHECK()
	GATS_CHECK_MESSAGE()
	GATS_CHECK_EQUAL()
	GATS_CHECK_WITHIN()
	GATS_CHECK_THROW()
	GATS_FAIL()

=============================================================
Revision History
-------------------------------------------------------------

Version 2021.10.29
	Added:
		TestApp::current_case()
		GATS_FAIL()
	Changed:
		All macros now make their calls via TestApp::current_case()
		allow for out-of-line use of the CHECK macros.

Version 2021.09.23
	Fixed pass-count in GATS_CHECK_THROW
	Changed stream type from wide to standard.
	Added:
		char_type, ostream_type, ostringstream_type
		GATS_CHECK_MESSAGE
	Refactored internal macro names to 'DETAILED_ ...'
	Replaced some CHECK macros internals with methods
	Formatting: placed quotes around case name in output.
	Added filename to check failure report.

Version 2021.08.29
	Alpha release.

=============================================================

Copyright Garth Santor/Trinh Han

The copyright to the computer program(s) herein
is the property of Garth Santor/Trinh Han, Canada.
The program(s) may be used and/or copied only with
the written permission of Garth Santor/Trinh Han
or in accordance with the terms and conditions
stipulated in the agreement/contract under which
the program(s) have been supplied.
=============================================================*/


#include <gats/ConsoleApp.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include <filesystem>
#include <type_traits>


namespace gats {

	/*!	\brief class TestApp
	
		TestApp singleton class provides a framework for a Windows-based unit-test application. 
	*/
	class TestApp : public ConsoleApp {
	// TYPES
	public:
		using char_type = char;
		using ostream_type = std::basic_ostream<char_type>;
		using ofstream_type = std::basic_ofstream<char_type>;
		using ostringstream_type = std::basic_ostringstream<char_type>;
		using string_type = std::basic_string<char_type>;

		/*!	\brief class TestApp::TestCase
			
			TestCase is the base type of classes generated by GATS_TEST_CASE().
			It provides services to the test case that can be accessed via the
			GATS_CHECK macros. */
		class TestCase {
		// ATTRIBUTES
			string_type		name_m;				
			std::uintmax_t	nChecked_m = 0;
			std::uintmax_t	nPassed_m = 0;
			double			weight_m = 1.0;			// weighted score of this case.

		// OPERATIONS
		public:
			// Blocked
			TestCase(TestCase const&) = delete;
			void operator = (TestCase const&) = delete;

			// Constructors
			TestCase(string_type const& name, double weight);
			TestCase(string_type const& name) : TestCase(name, 1) {}

			// Application Interface
			virtual void execute() = 0;

			// Check Services
			void add_check() { ++nChecked_m; }
			void add_passed() { ++nPassed_m; }
			void output_check_location(ostream_type& os, std::filesystem::path file, int line);
			void check(bool condition, const char_type* const condStr, const char * const file, int line);
			void check_message(bool condition, const string_type& message, const char* const file, int line);
			template <typename LHS, typename RHS> 
			void check_equal(const LHS& lhs, const RHS& rhs, const char_type* lhsStr, const char_type* rhsStr, const char* const file, int line);
			template <typename LHS, typename RHS, typename VALUE>
			void check_close_within(const LHS& lhs, const RHS& rhs, const VALUE& minimum, const char_type* lhsStr, const char_type* rhsStr, const char_type* minimumStr, const char* const file, int line);

			// Parent Services
			inline ostream_type& display() { return TestApp::display(); }
			inline ostream_type& log() { return TestApp::logFile_m; }

			constexpr auto operator <=> (TestCase const& rhs) const { return name_m <=> rhs.name_m; }
			constexpr bool operator == (TestCase const& rhs) const { return name_m == rhs.name_m; }

			// Access
			friend class TestApp;
		};

	// ATTRIBUTES
	private:
		using Container = std::vector<TestCase*>;
		static std::unique_ptr<Container> casesPtr_sm;
		static ofstream_type logFile_m;
		static TestCase* currentCasePtr_sm;

	// OPERATIONS
		static ostream_type& display() { return std::cout; }
		static Container& cases();

		// Interface
		void setup() override;
		int execute() override;

	public:
		static TestCase* current_case(const char* file, int line);
	};


	/*!	\brief Check for value equality.
	
		Check for value equality, reporting if different.
		This function is wrapped by GATS_CHECK_EQUAL() and not called directly.
	*/
	template <typename LHS, typename RHS>
	void TestApp::TestCase::check_equal(const LHS& lhs, const RHS& rhs, const TestApp::char_type* lhsStr, const TestApp::char_type* rhsStr, const char* const file, int line) {
		bool condition = lhs == rhs;
		++nChecked_m;
		if (condition==false) {
			ostringstream_type oss;
			output_check_location(oss, file, line);
			oss << "\"" << lhsStr <<  "\" [" << lhs << "] != \"" << rhsStr << "\" [" << rhs << "]\n";
			display() << oss.str();
			//log() << oss.str();
		} else
			++nPassed_m;
	}


	/*!	\brief Check for value closeness.

		Check for real number value similarity as defined by a minimum value, reporting if the difference is
		greater than the minimum.  This function is wrapped by GATS_CHECK_WITHIN() and not called directly.
	*/
	template <typename LHS, typename RHS, typename VALUE>
	void TestApp::TestCase::check_close_within(const LHS& lhs, const RHS& rhs, const VALUE& minimum, const char_type* lhsStr, const char_type* rhsStr, const char_type* minimumStr, const char* const file, int line) {
		bool condition = abs((lhs) - (rhs)) <= abs(minimum);
		add_check();
		if (condition==false) {
			ostringstream_type oss;
			output_check_location(oss, file, line);
			oss << "difference(" << lhsStr << ", " << rhsStr << ") > " << minimumStr << " ==> \t|" << lhs << " - " << rhs << "| > " << abs(minimum) << "\n";
			display() << oss.str();
			//log() << oss.str();
		} else
			add_passed();
	}

} // end-of-namespace gats



/*!	Creates a test case with the identifier 'name'

	\param 'name' is the test cases identifier.

	The identifier is used to indicate execution order (lexicographical ordering) and both the class and global object name.
*/
#define GATS_TEST_CASE(name) \
	static class TestCase_ ## name : public gats::TestApp::TestCase {\
	public: TestCase_ ## name() : TestCase(#name) { }\
	public: virtual void execute() override;\
	} TestCase_ ## name ## _g;\
	void TestCase_ ## name :: execute()


/*!	Creates a test case with the identifier 'name'

	\param 'name' is the test cases identifier.

	The identifier is used to indicate execution order (lexicographical ordering) and both the class and global object name.
*/
#define GATS_TEST_CASE_WEIGHTED(name,weight) \
	static class TestCase_ ## name : public gats::TestApp::TestCase {\
	public: TestCase_ ## name() : TestCase(#name,weight) { }\
	public: virtual void execute() override;\
	} TestCase_ ## name ## _g;\
	void TestCase_ ## name :: execute()


/*!	Performs a check point for the specified condition.

	\param 'cond' is condition that must pass.
*/
#define GATS_CHECK(condition) gats::TestApp::current_case(__FILE__,__LINE__)->check((condition), #condition, __FILE__, __LINE__)


/*!	Performs a check point for the specified condition.

	\param 'cond' is condition that must pass.
*/
#define DETAIL_GATS_CHECK_MESSAGE(condition,message,file,line) {\
	bool cond = (condition);\
	gats::TestApp::string_type strMessage;\
	if (cond==false) {\
		gats::TestApp::ostringstream_type oss;\
		oss << message;\
		strMessage = oss.str();\
	}\
	gats::TestApp::current_case(__FILE__,__LINE__)->check_message(cond, strMessage, file, line);\
}
#define GATS_CHECK_MESSAGE(condition, message) DETAIL_GATS_CHECK_MESSAGE(condition, message, __FILE__, __LINE__)



/*!	Performs a check that the values are exact matches.

	\param 'testValue' is the value being checked.
	\param 'expectedValue' is the value that the test value should have.
*/
#define GATS_CHECK_EQUAL(testValue, expectedValue) gats::TestApp::current_case(__FILE__,__LINE__)->check_equal((testValue), (expectedValue), #testValue, #expectedValue, __FILE__, __LINE__)



/*!	Performs a check that the values are close matches.

	\param 'testValue' is the value being checked.
	\param 'expectedValue' is the value that the test value should have.
	\param 'minimum' is the maximum difference allowed.
*/
#define GATS_CHECK_WITHIN(testValue, expectedValue, minimum) gats::TestApp::current_case(__FILE__,__LINE__)->check_close_within((testValue), (expectedValue), (minimum), #testValue, #expectedValue, #minimum, __FILE__, __LINE__)



/*!	Performs a check that the values are exact matches.

	\param 'testValue' is the value being checked.
	\param 'expectedValue' is the value that the test value should have.
*/
#define DETAIL_GATS_CHECK_THROW(operation, expectedException, file, line) {\
	gats::TestApp::current_case(__FILE__,__LINE__)->add_check();\
	bool isGood = false;\
	try {\
		operation;\
	} catch(expectedException) {\
		gats::TestApp::current_case(__FILE__,__LINE__)->add_passed();\
		isGood = true;\
	} catch(...){\
		gats::TestApp::ostringstream_type oss;\
		gats::TestApp::current_case(__FILE__,__LINE__)->output_check_location(oss, file, line);\
		oss << "unknown exception \"" #expectedException "\" not thrown\n";\
		gats::TestApp::current_case(__FILE__,__LINE__)->display() << oss.str();\
		isGood = true;\
	}\
	if (!isGood) {\
		gats::TestApp::ostringstream_type oss;\
		gats::TestApp::current_case(__FILE__,__LINE__)->output_check_location(oss, file, line);\
		oss << "no exception thrown, expecting \"" #expectedException "\"\n";\
		gats::TestApp::current_case(__FILE__,__LINE__)->display() << oss.str();\
	}\
}
#define GATS_CHECK_THROW(operation, expectedException) DETAIL_GATS_CHECK_THROW(operation, expectedException, __FILE__, __LINE__)



/*!	Logs a failure in the test case.  Terminating the test case.

	\param 'msg' is the message report.

*/
#define DETAIL_GATS_FAIL(msg, file, line) {\
	gats::TestApp::current_case(__FILE__,__LINE__)->add_check();\
	gats::TestApp::ostringstream_type oss;\
	gats::TestApp::current_case(__FILE__,__LINE__)->output_check_location(oss, file, line);\
	oss << msg << '\n';\
	gats::TestApp::current_case(__FILE__,__LINE__)->display() << oss.str();\
	return;\
}
#define GATS_FAIL(msg) DETAIL_GATS_FAIL(msg, __FILE__, __LINE__)
