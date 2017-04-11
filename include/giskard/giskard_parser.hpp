#pragma once

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <string>

#include "giskard/giskard.hpp"

namespace giskard {
	namespace fusion = boost::fusion;
    namespace phoenix = boost::phoenix;
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

	// template <typename Iterator>
	// struct giskard_grammar : qi::grammar<Iterator, std::vector<giskard::SpecPtr>(), ascii::space_type> {
		
	// 	giskard_grammar() : giskard_grammar::base_type(giskard) {
	// 		using qi::lit;
	// 		using qi::lexeme;
	// 		using qi::double_;
	// 		using ascii::char_;

	// 		lstr %= +(char_ - ('"', '.', ':', ';', '+', '-', '*', '/'));
	// 		quotedString %= lexeme['"' >> lstr >> '"'];


	// 		scalExpr %= scalTerm >> -('+' >> scalTerm) | scalTerm >> -('-' >> scalTerm);
	// 		scalTerm %= scalFactor >> -('*' >> scalFactor) | scalFactor >> -('/' >> scalFactor) | vecFactor >> '*' >> vecFactor;

	// 		scalFactor %= double_[DoubleSpecPtr(new DoubleConstSpec(_1))]
	// 			       | '(' >> scalExpr >> ')'
	// 			       | (vecFactor >> ".x")[DoubleXCoordOfSpecPtr(new DoubleXCoordOfSpec(_1))]
	// 			       | (vecFactor >> ".y")[DoubleYCoordOfSpecPtr(new DoubleXCoordOfSpec(_1))]
	// 			       | (vecFactor >> ".z")[DoubleZCoordOfSpecPtr(new DoubleXCoordOfSpec(_1))]
	// 			       | (lit("input") >> '(' >> quotedString >> ')')[DoubleInputSpecPtr(new DoubleInputSpec(_1))]
	// 			       | (lit("norm")  >> '(' >> vecExpr >> ')')[DoubleNormOfSpecPtr(new DoubleNormOfSpec(_1))]
	// 			       | (lit("sin")   >> '(' >> scalExpr >> ')')[SinSpecPtr(new SinSpec(_1))]
	// 			       | (lit("cos")   >> '(' >> scalExpr >> ')')[CosSpecPtr(new CosSpec(_1))]
	// 			       | (lit("tan")   >> '(' >> scalExpr >> ')')[TanSpecPtr(new TanSpec(_1))]
	// 			       | (lit("asin")   >> '(' >> scalExpr >> ')')[ASinSpecPtr(new ASinSpec(_1))]
	// 			       | (lit("acos")   >> '(' >> scalExpr >> ')')[ACosSpecPtr(new ACosSpec(_1))]
	// 			       | (lit("atan")   >> '(' >> scalExpr >> ')')[ATanSpecPtr(new ATanSpec(_1))]
	// 			       | (lit("abs")   >> '(' >> scalExpr >> ')')[AbsSpecPtr(new AbsSpec(_1))]
	// 			       | (lit("min")   >> '(' >> scalExpr >> ',' >> scalExpr >> ')')[MinSpecPtr(new MinSpec(_1, _2))]
	// 			       | (lit("max")   >> '(' >> scalExpr >> ',' >> scalExpr >> ')')[MaxSpecPtr(new MaxSpec(_1, _2))]
	// 			       | (lit("if")    >> '(' >> scalExpr >> ',' >> scalExpr >> ',' >> scalExpr >> ')')[DoubleIfSpecPtr(new DoubleIfSpec(_1, _2, _3))]
	// 			       | (lstr)[DoubleReferenceSpecPtr(new DoubleReferenceSpec(_1))];

	//         vecExpr %= vecTerm >> -('+' >> vecTerm) | vecTerm >> -('-' >> vecTerm);
	//         vecTerm %= vecFactor >> -('*' >> scalFactor)
	//                 | scalFactor  >> '*' >> vecFactor
	//                 | frameFactor >> '*' >> vecFactor
	//                 | rotFactor   >> '*' >> vecFactor
	//                 | lstr;

	//         vecFactor %= lit("vec3") >> '(' >> scalExpr >> ',' >> scalExpr >> ',' >> scalExpr >> ')'
	//                   | lit("inputVec3") >> '(' >> quotedString >> ')'
	//                   | '(' >> vecExpr >> ')'
	//                   | '-' >> vecExpr
	//                   | lit("originOf") >> '(' >> frameExpr >> ')'
	//                   | frameExpr >> ".pos"
	//                   | lit("cross") >> '(' >> vecExpr >> ',' >> vecExpr >> ')'
	//                   | lstr;

	//         rotExpr %= rotTerm;
	//         rotTerm %= rotFactor >> -('*' >> rotFactor);

	//         rotFactor %= lit("rotation") >> '(' >> double_ >> ',' >> double_ >> ',' >> double_ >> ',' >> double_ >> ')'
	//                   | lit("rotation") >> '(' >> vecExpr >> ',' >> scalExpr >> ')'
	//                   | lit("inputRotation") >> '(' >> quotedString >> ')'
	//                   | '(' >> rotExpr >> ')'
	//                   | lit("invert") >> '(' >> rotExpr >> ')'
	//                   | lit("orientationOf") >> '(' >> frameExpr >> ')'
	//                   | frameExpr >> ".rot"
	//                   | lstr;

	//       	frameExpr %= frameTerm;
	//       	frameTerm %= frameFactor >> -('*' >> frameFactor);

	//       	frameFactor %= lit("frame") >> '(' >> rotExpr >> ',' >> vecExpr >> ')'
	//       	            | lit("inputFrame") >> '(' >> quotedString >> ')'
	//       	            | '(' >> frameExpr >> ')'
	//       	            | lstr;


	//         expression %= scalExpr | vecExpr | rotExpr | frameExpr | lstr;

	//         giskard %= *(expression >> ';');
	//     }

	//     qi::rule<Iterator, std::string(), ascii::space_type> lstr;
	//     qi::rule<Iterator, std::string(), ascii::space_type> quotedString;

	//     qi::rule<Iterator, std::vector<giskard::SpecPtr>, ascii::space_type> giskard;
	//     qi::rule<Iterator, giskard::SpecPtr, ascii::space_type> expression;
	    
	//     qi::rule<Iterator, giskard::DoubleSpecPtr, ascii::space_type> scalExpr;
	//     qi::rule<Iterator, giskard::DoubleSpecPtr, ascii::space_type> scalTerm;
	//     qi::rule<Iterator, giskard::DoubleSpecPtr, ascii::space_type> scalFactor;

	//     qi::rule<Iterator, giskard::VectorSpecPtr, ascii::space_type> vecExpr;
	//     qi::rule<Iterator, giskard::VectorSpecPtr, ascii::space_type> vecTerm;
	//     qi::rule<Iterator, giskard::VectorSpecPtr, ascii::space_type> vecFactor;

	//     qi::rule<Iterator, giskard::RotationSpecPtr, ascii::space_type> rotExpr;
	//     qi::rule<Iterator, giskard::RotationSpecPtr, ascii::space_type> rotTerm;
	//     qi::rule<Iterator, giskard::RotationSpecPtr, ascii::space_type> rotFactor;

	//     qi::rule<Iterator, giskard::FrameSpecPtr, ascii::space_type> frameExpr;
	//     qi::rule<Iterator, giskard::FrameSpecPtr, ascii::space_type> frameTerm;
	//     qi::rule<Iterator, giskard::FrameSpecPtr, ascii::space_type> frameFactor;
 //    };

	template <typename Iterator>
	struct test_grammar : qi::grammar<Iterator, giskard::SpecPtr(), ascii::space_type> {
		
		test_grammar() : test_grammar::base_type(test) {
			using qi::lit;
			using qi::lexeme;
			using qi::double_;
			using ascii::char_;
			using qi::_val;
			using boost::spirit::qi::_1;
			using boost::spirit::qi::_2;
			using boost::spirit::qi::_3;
			using boost::spirit::qi::_4;
			using boost::spirit::qi::_5;
			using boost::spirit::qi::_6;
			using boost::phoenix::ref;			
			using boost::phoenix::bind;

			test = scalar | vector;

    		scalar = double_[_val = bind(giskard::double_const_spec, _1)];
			vector = (lit('(') >> scalar >> lit(',') >> scalar >> lit(',') >> scalar >> lit(')'))[_val = bind(giskard::vector_constructor_spec, _1, _2, _3)];
	    }

	    double bla;

	    qi::rule<Iterator, giskard::SpecPtr(), ascii::space_type> test;
	    qi::rule<Iterator, giskard::DoubleSpecPtr(), ascii::space_type> scalar;
	    qi::rule<Iterator, giskard::VectorSpecPtr(), ascii::space_type> vector;
    };

	inline bool parse_giskard(const std::string& msg, giskard::Scope& scope) {
		typedef std::string::const_iterator itType;
		using qi::lit;
		using qi::lexeme;
		using qi::double_;
		using ascii::char_;
		using boost::spirit::qi::phrase_parse;

		test_grammar<itType> g;
		giskard::SpecPtr ptr;

		bool r = phrase_parse(msg.begin(), msg.end(), g, boost::spirit::ascii::space, ptr);

		return r; //r;
	}

}

