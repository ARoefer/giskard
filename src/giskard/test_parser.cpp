#include "giskard/giskard_parser.hpp"

using namespace std;
using namespace giskard;


class Test {
protected:
	Test(string _name)
	: name(_name) {}
public:
	virtual bool run() = 0;

	const string name;
};

class SimpleFile : public Test {
public:
	SimpleFile()
	: Test("Simple File Test") {}
	
	bool run() {
		GiskardParser gp;
		try {
			Scope2Ptr scope = gp.parse_giskard_from_file("test_data/simple_func.giskard"); 
		} catch (std::exception& e) {
			cerr << e.what() << endl;
			return false;
		}

		return true;
	}
};

typedef giskard_skipper<std::string::const_iterator> TSkipper;

class SyntaxTest : public Test {
public:
	SyntaxTest(string name)
	: Test(name) {}

protected:
	giskard_grammar<std::string::const_iterator> g;
	TSkipper skip;
};

class SkipperTest : public SyntaxTest {
public:
	SkipperTest(string name, string _input, bool _fail)
	: SyntaxTest(name)
	, input(_input)
	, fail(_fail) {}

	bool run() {
		using boost::spirit::qi::phrase_parse;
		auto begin = input.begin();
		auto end = input.end();

		GiskardSpecPtr temp;
		bool r = phrase_parse(begin, end, g, skip, temp);

		if (begin != end) {
			if (!fail) {
				std::cerr << "Parsing failed! Remaining: '" << std::string(begin, end) << "'" << std::endl;
			}
			return fail;
		}

		return (!fail && r) || (fail && !r);
	}

protected:
	const string input;
	bool fail;
};

template <typename T>
class GrammaRulePtrTest : public SyntaxTest {
public:
	GrammaRulePtrTest(string name, string _input, bool _fail)
	: SyntaxTest(name)
	, input(_input)
	, fail(_fail) {}

	virtual qi::rule<std::string::const_iterator, T(), TSkipper> getRule() = 0; 

	bool run() {
		using boost::spirit::qi::phrase_parse;
		auto begin = input.begin();
		auto end = input.end();

		T temp;
		bool r = phrase_parse(begin, end, getRule(), skip, temp);

		if (begin != end) {
			if (!fail) {
				std::cerr << "    Parsing failed! Remaining: '" << std::string(begin, end) << "'" << std::endl;
			}
			return fail;
		}

		if (fail) {
			if (temp)
				std::cerr << "    Pointer is not NULL: " << endl << temp->toString() << endl;
		} else {
			if (!temp)
				std::cerr << "    Pointer is NULL" << endl;
			else if (!temp->isValid()) {
				r = false;
				std::cerr << "Faulty structure: " << endl << temp->toString() << endl;
			}
		}

		r = r && temp;

		return (!fail && r) || (fail && !r);
	}

protected:
	const string input;
	bool fail;
};

template <typename T>
class GrammaRuleObjectTest : public SyntaxTest {
public:
	GrammaRuleObjectTest(string name, string _input, bool _fail)
	: SyntaxTest(name)
	, input(_input)
	, fail(_fail) {}

	virtual qi::rule<std::string::const_iterator, T(), TSkipper> getRule() = 0; 

	bool run() {
		using boost::spirit::qi::phrase_parse;
		auto begin = input.begin();
		auto end = input.end();

		T temp;
		bool r = phrase_parse(begin, end, getRule(), skip, temp);

		if (begin != end) {
			if (!fail) {
				std::cerr << "Parsing failed! Remaining: '" << std::string(begin, end) << "'" << std::endl;
			}
			return fail;
		}

		if (r && fail) {
			std::cerr << "Faulty parse result: " << temp << std::endl;
		}

		return (!fail && r) || (fail && !r);
	}

protected:
	const string input;
	bool fail;
};

class ExpressionTest : public GrammaRulePtrTest<ITokenPtr> {
public:
	ExpressionTest(string name, string input, bool fail)
	: GrammaRulePtrTest<ITokenPtr>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, ITokenPtr(), TSkipper> getRule() { return g.expression; }; 	
};

class FunctionDefinitionTest : public GrammaRulePtrTest<ITokenPtr> {
public:
	FunctionDefinitionTest(string name, string input, bool fail)
	: GrammaRulePtrTest<ITokenPtr>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, ITokenPtr(), TSkipper> getRule() { return g.function; }; 	
};

class NamedExpressionTest : public GrammaRulePtrTest<ITokenPtr> {
public:
	NamedExpressionTest(string name, string input, bool fail)
	: GrammaRulePtrTest<ITokenPtr>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, ITokenPtr(), TSkipper> getRule() { return g.namedExpression; }; 	
};

class ImportStatementTest : public GrammaRulePtrTest<ImportOperationPtr> {
public:
	ImportStatementTest(string name, string input, bool fail)
	: GrammaRulePtrTest<ImportOperationPtr>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, ImportOperationPtr(), TSkipper> getRule() { return g.importStatement; }
};

class DeclarationTest : public GrammaRuleObjectTest<TDeclaration> {
public:
	DeclarationTest(string name, string input, bool fail)
	: GrammaRuleObjectTest<TDeclaration>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, TDeclaration(), TSkipper> getRule() { return g.declaration; }; 	
};

class DeclarationsListTest : public GrammaRuleObjectTest<TArgumentList> {
public:
	DeclarationsListTest(string name, string input, bool fail)
	: GrammaRuleObjectTest<TArgumentList>(name, input, fail) {}

	virtual qi::rule<std::string::const_iterator, TArgumentList(), TSkipper> getRule() { return g.argDefinitionsList; }
};


void runTest(boost::shared_ptr<Test> test) {
	if (test->run()) {
		cout << "SUCCESS: " << test->name << endl;
	} else {
		cerr << "FAILED: " << test->name << endl;
	}
}


int main(int argc, char const *argv[])
{
	giskard::Scope scope;
	std::string in = "";
	
	string baseDir = "some_dir/";
	std::string basic = "modules/common.giskard";
	std::string package = "$(giskard)/modules/common.giskard";

	ImportOperation ia(basic, "common");
	ImportOperation ib(package, "common");

	//runTest(instance<SimpleFile>());
	runTest(instance<SkipperTest>("Skip Spaces", "       	\n \t", false));
	runTest(instance<SkipperTest>("Skip Comment", "# This is a comment\n", false));
	runTest(instance<SkipperTest>("Skip Comment and White Spaces", "\t # This is a comment\n \t \t  ", false));

	runTest(instance<ExpressionTest>("Const Scalar", "1.0442", false));
	runTest(instance<ExpressionTest>("Const Reference", "foo", false));
	runTest(instance<ExpressionTest>("Addition", "2 + 5", false));
	runTest(instance<ExpressionTest>("Addition Containing Reference", "2 + foo", false));
	runTest(instance<ExpressionTest>("Subtraction", "2 - 5", false));
	runTest(instance<ExpressionTest>("Subtraction Negated", "2 - -5", false));
	runTest(instance<ExpressionTest>("Multiplication", "5.55 * 9", false));
	runTest(instance<ExpressionTest>("Division", "5.55 / 11", false));
	runTest(instance<ExpressionTest>("Namespace Reference", "a.b", false));
	runTest(instance<ExpressionTest>("Function Call", "foo(1,2,3)", false));
	runTest(instance<ExpressionTest>("Malformed Function Call 1", "foo(1,2,3", true));
	runTest(instance<ExpressionTest>("Malformed Function Call 2", "foo(1,,3)", true));
	runTest(instance<ExpressionTest>("Malformed Expression 1", "3 -* 4", true));

	runTest(instance<NamedExpressionTest>("Named Expression Test", "foo = 3*4", false));
	
	runTest(instance<ImportStatementTest>("Import Test 1", "import \"some_file.giskard\"", false));
	runTest(instance<ImportStatementTest>("Import Test 2", "import \"some_file.giskard\" as sf", false));
	runTest(instance<ImportStatementTest>("Malformed Import Test 1", "import some_file.giskard", true));
	runTest(instance<ImportStatementTest>("Malformed Import Test 2", "import some_file.giskard as sf", true));

	runTest(instance<DeclarationTest>("Declaration Test", "scalar foo", false));
	runTest(instance<DeclarationTest>("Malformed Declaration Test 1", "scalar foo bar", true));
	runTest(instance<DeclarationTest>("Malformed Declaration Test 2", "scalar foo = 10", true));
	runTest(instance<DeclarationTest>("Malformed Declaration Test 3", "foo bar", true));
	runTest(instance<DeclarationTest>("Malformed Declaration Test 4", "foo scalar", true));
	
	runTest(instance<DeclarationsListTest>("Argument Declaration List", "(scalar a, vec3 b, rotation c, frame f)", false));

	runTest(instance<FunctionDefinitionTest>("Function Definition 1", "scalar foo(scalar a, vec3 b, rotation c, frame f) { return a * b * c * f; }", false));
	runTest(instance<FunctionDefinitionTest>("Function Definition 2", "def scalar foo(scalar a, vec3 b, rotation c, frame f) { bar = 3 * 4; return a * b * c * f; }", false));
	runTest(instance<FunctionDefinitionTest>("Malformed Function Definition - Missing Type", "def foo(scalar a, vec3 b, rotation c, frame f) { return a * b * c * f; }", true));
	runTest(instance<FunctionDefinitionTest>("Malformed Function Definition - No Args", "def scalar foo() { return a * b * c * f; }", true));
	runTest(instance<FunctionDefinitionTest>("Malformed Function Definition - No Arg Types", "def scalar foo(a, b, c) { return a * b * c * f; }", true));
	runTest(instance<FunctionDefinitionTest>("Malformed Function Definition - No Return", "def scalar foo(a, b, c) { lol = a * b * c * f; }", true));


//	assert(ia.getAlias() == "common");

//	assert(ia.resolvePath(baseDir) == baseDir + basic);

//	assert(ib.resolvePath(baseDir) == ros::package::getPath("giskard") + basic);


	return 0;
}