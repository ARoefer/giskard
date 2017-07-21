#pragma once

#include <unordered_map>
#include <vector>
#include "giskard_core/specifications.hpp"

using namespace std;

namespace giskard_core {

class GiskardLangParser {
public:
	struct EOSException : std::exception {
	  const char* what() const noexcept {return "UNEXPECTED END OF STREAM!\n";}
	};

	struct ParseException : std::exception {
		ParseException(const string& _msg)
		: msg(_msg) {}
	  const char* what() const noexcept {return msg.c_str();}
	private:
		const string msg;
	};


	GiskardLangParser()
	: constDZeroPtr(new DoubleConstSpec(0.0))
	, constDNegOnePtr(new DoubleConstSpec(-1.0))
	{}

	QPControllerSpec parseQPController(const string& text) {
		QPControllerSpec out;

		typeMap.clear();

		SIt cursor = text.begin();
		const SIt end = text.end();
		lineNumber = 1;
		lineStart = cursor;

		if (consumeSpaces(cursor, end))
			throw EOSException();

		if (consumeName(cursor, end).compare("scope") == 0) {
			if (consumeSpaces(cursor, end))
				throw EOSException();

			char c = *cursor;

			if (c != '=') {
				throwParseError(cursor, end, "expected '='");
			}

			cursor++;
			parseScope(cursor, end, out.scope_);
		} else {
			throwParseError(cursor, end, "Expected keyword 'scope'!");
		}

		bool ctrl, soft, hard;
		ctrl = false;
		hard = false;
		soft = false;


		while (!(ctrl && soft && hard)) {
			if (consumeSpaces(cursor, end))
				throw EOSException();

			string keyword = consumeName(cursor, end);
			if (keyword.compare("hardConstraints") != 0 && keyword.compare("softConstraints") != 0 && keyword.compare("controllableConstraints") != 0) {
				throwParseError(cursor, end, "Expected keyword 'hardConstraints', 'softConstraints' or 'controllableConstraints'! Got: " + keyword);
			}
		
			if (consumeSpaces(cursor, end))
				throw EOSException();

			char c = *cursor;

			if (c != '=') {
				printUnexpectedChar(cursor, cursor, '=');
				throwParseError(cursor, end, "expected '='");
			}

			cursor++;
			if (consumeSpaces(cursor, end))
				throw EOSException();
			
			vector<SpecPtr> specs;
			vector<EType> types;

			parseNTuple(specs, types, cursor, end);

			if (keyword.compare("hardConstraints") == 0) {
				for (size_t i = 0; i < types.size(); i++) {
					if (types[i] == HARDC)
						out.hard_constraints_.push_back(*dynamic_pointer_cast<HardConstraintSpec>(specs[i]));
					else {
						cerr << "Expected type '" << typeNames[HARDC] << "' got '" << typeNames[types[i]] << "'" << endl;
						throwParseError(cursor, end, "Unexpected type");
					}
				}
				hard = true;
			} else if (keyword.compare("softConstraints") == 0) {
				for (size_t i = 0; i < types.size(); i++) {
					if (types[i] == SOFTC)
						out.soft_constraints_.push_back(*dynamic_pointer_cast<SoftConstraintSpec>(specs[i]));
					else {
						cerr << "Expected type '" << typeNames[SOFTC] << "' got '" << typeNames[types[i]] << "'" << endl;
						throwParseError(cursor, end, "Unexpected type");
					}
				}
				soft = true;
			} else if (keyword.compare("controllableConstraints") == 0) {
				for (size_t i = 0; i < types.size(); i++) {
					if (types[i] == CTRLC)
						out.controllable_constraints_.push_back(*dynamic_pointer_cast<ControllableConstraintSpec>(specs[i]));
					else {
						cerr << "Expected type '" << typeNames[CTRLC] << "' got '" << typeNames[types[i]] << "'" << endl;
						throwParseError(cursor, end, "Unexpected type");
					}
				}
				ctrl = true;
			}
		}

		return out;
	}

private:
	enum EType : int {
		NONE = 0,
		DOUBLE,
		VECTOR,
		ROTATION,
		FRAME,
		HARDC,
		SOFTC,
		CTRLC,
		STRING,
		INTEGER,

		LAST
	};

	DoubleConstSpecPtr constDZeroPtr; 
	DoubleConstSpecPtr constDNegOnePtr;
	unordered_map<string, EType> typeMap;

	typedef string::const_iterator SIt;
	int lineNumber;
	SIt lineStart;

	string typeNames[LAST] = {"None", "Double", "Vector", "Rotation", "Frame", "Hard Constraint", "Soft Constraint", "Controllable Constraint", "String", "Integer"};


	void throwParseError(const SIt& current, const SIt& end, const string& msg) {
		stringstream ss;
		SIt cursor = current;
		while(cursor != end && (*cursor) != '\n')
			cursor++;

		ss << msg << endl
		   << "l." << lineNumber << ": " << string(lineStart, cursor); 
		throw ParseException(ss.str());
	}

	bool consumeSpaces(SIt& cursor, const SIt& end) {
		bool comment = false;
		while(cursor != end) {
			char c = *cursor;
			if (!comment && c == '#')
				comment = true;

			if (c == '\n') {
				lineNumber++;
				lineStart = cursor + 1;

				if (comment)
					comment = false;
			}

			if (!comment && c != ' ' && c != '\t' && c != '\n')
				return false;
			cursor++;
		}

		return true;
	}



	void printBinaryTypeError(const SIt& a1, const SIt& a2, const SIt& b1, const SIt& b2, EType aT, EType bT, char c) {
		cerr << "TYPE MISMATCH: No overload for '" << c << "' known for types: " << endl
		 << "   " << typeNames[aT] << ": " << string(a1, a2) <<  endl
		 << "   " << typeNames[bT] << ": " << string(b1, b2) << endl;
	}

	template <typename T, typename E>
	boost::shared_ptr<T> resolveChainedExpression(boost::shared_ptr<E> a, boost::shared_ptr<E> b) {
		boost::shared_ptr<T> temp = dynamic_pointer_cast<T>(b);
		vector<boost::shared_ptr<E>> inputs {a,b};
		if (temp) {
			inputs = temp->get_inputs();
			inputs.insert(inputs.begin(), a);
		} else {
			temp = boost::shared_ptr<T>(new T());
		}
		temp->set_inputs(inputs);
		return temp;
	}

	EType parseExpression(SpecPtr &spec, SIt& current, const SIt& end) {
		SIt first = current;
		SpecPtr a;
		EType aT = parseTerm(a, current, end);

		// if (aT == NONE) {
		// 	cerr << "TYPE ASSESMENT FAILED:" << endl;
		// 		 << "'" << string(first, current) << "'" << endl;
		// 	return NONE;
		// }

		if(!consumeSpaces(current, end)) {
			char c = *current;
			if (c == '+' || c == '-') {
				SIt last1 = current-1;
				current++;
				if (current != end) {
					SIt first2 = current;
					SpecPtr b;
					EType bT = parseExpression(b, current, end);

					if (aT == DOUBLE && bT == DOUBLE) {
						
						DoubleSpecPtr da = static_pointer_cast<DoubleSpec>(a); 
						DoubleSpecPtr db = static_pointer_cast<DoubleSpec>(b);
						if (c == '+') {
							spec = resolveChainedExpression<DoubleAdditionSpec>(da,db);
						} else {
							spec = resolveChainedExpression<DoubleSubtractionSpec>(da,db);
						}
						return DOUBLE;
					} else if (aT == VECTOR && bT == VECTOR) {
						
						VectorSpecPtr da = static_pointer_cast<VectorSpec>(a); 
						VectorSpecPtr db = static_pointer_cast<VectorSpec>(b);
						if (c == '+') {
							spec = resolveChainedExpression<VectorAdditionSpec>(da,db);
						} else {
							spec = resolveChainedExpression<VectorSubtractionSpec>(da,db);
						}
						return VECTOR;
					}

					printBinaryTypeError(first, last1, first2, current, aT, bT, c);
					throwParseError(current, end, "Unexpected type");
				} else {
					throw EOSException();
				}
			}
		}

		spec = a;
		return aT;
	}

	EType parseTerm(SpecPtr &spec, SIt& current, const SIt& end) {
		SIt first1 = current;
		SpecPtr a;
		EType aT = parseFactor(a, current, end);

		if(!consumeSpaces(current, end)) {
			char c = *current;
			if (c == '*' || c == '/') {
				SIt last1 = current - 1;
				current++;
				if (current != end) {
					SIt first2 = current;
					SpecPtr b;
					EType bT = parseTerm(b, current, end);

					switch(c) {
					case '*':
						if (aT == DOUBLE && bT == DOUBLE) {
							DoubleSpecPtr da = static_pointer_cast<DoubleSpec>(a); 
							DoubleSpecPtr db = static_pointer_cast<DoubleSpec>(b);						
							spec = resolveChainedExpression<DoubleMultiplicationSpec>(da, db);
							return DOUBLE;
						} else if (aT == DOUBLE && bT == VECTOR) {
							DoubleSpecPtr da = static_pointer_cast<DoubleSpec>(a); 
							VectorSpecPtr db = static_pointer_cast<VectorSpec>(b);
							VectorDoubleMultiplicationSpecPtr temp = VectorDoubleMultiplicationSpecPtr(new VectorDoubleMultiplicationSpec());
							temp->set_double(da);
							temp->set_vector(db);
							spec = temp;
							return VECTOR;
						} else if (aT == VECTOR && bT == DOUBLE) {
							DoubleSpecPtr da = static_pointer_cast<DoubleSpec>(b); 
							VectorSpecPtr db = static_pointer_cast<VectorSpec>(a);
							VectorDoubleMultiplicationSpecPtr temp = VectorDoubleMultiplicationSpecPtr(new VectorDoubleMultiplicationSpec());
							temp->set_double(da);
							temp->set_vector(db);
							spec = temp;
							return VECTOR;
						} else if (aT == VECTOR && bT == VECTOR) {
							VectorSpecPtr da = static_pointer_cast<VectorSpec>(a); 
							VectorSpecPtr db = static_pointer_cast<VectorSpec>(b);
							VectorDotSpecPtr temp = VectorDotSpecPtr(new VectorDotSpec());
							temp->set_lhs(da);
							temp->set_rhs(db);
							spec = temp;
							return DOUBLE;
						} else if (aT == ROTATION && bT == VECTOR) {
							RotationSpecPtr da = static_pointer_cast<RotationSpec>(a); 
							VectorSpecPtr db = static_pointer_cast<VectorSpec>(b);
							VectorRotationMultiplicationSpecPtr temp = VectorRotationMultiplicationSpecPtr(new VectorRotationMultiplicationSpec());
							temp->set_rotation(da);
							temp->set_vector(db);
							spec = temp;
							return VECTOR;
						} else if (aT == FRAME && bT == VECTOR) {
							FrameSpecPtr da = static_pointer_cast<FrameSpec>(a); 
							VectorSpecPtr db = static_pointer_cast<VectorSpec>(b);
							VectorFrameMultiplicationSpecPtr temp = VectorFrameMultiplicationSpecPtr(new VectorFrameMultiplicationSpec());
							temp->set_frame(da);
							temp->set_vector(db);
							spec = temp;
							return VECTOR;
						} else if (aT == FRAME && bT == FRAME) {
							FrameSpecPtr da = static_pointer_cast<FrameSpec>(a); 
							FrameSpecPtr db = static_pointer_cast<FrameSpec>(b);						
							spec = resolveChainedExpression<FrameMultiplicationSpec>(da, db);
							return FRAME;
						} else if (aT == ROTATION && bT == ROTATION) {
							RotationSpecPtr da = static_pointer_cast<RotationSpec>(a); 
							RotationSpecPtr db = static_pointer_cast<RotationSpec>(b);						
							spec = resolveChainedExpression<RotationMultiplicationSpec>(da, db);
							return ROTATION;
						}
						printBinaryTypeError(first1, last1, first2, current, aT, bT, c);
						return NONE;
						break;
					default:
						if (aT == DOUBLE && bT == DOUBLE) {
							DoubleSpecPtr da = static_pointer_cast<DoubleSpec>(a); 
							DoubleSpecPtr db = static_pointer_cast<DoubleSpec>(b);
							spec = resolveChainedExpression<DoubleDivisionSpec>(da, db);	
							return DOUBLE;
						}
						printBinaryTypeError(first1, last1, first2, current, aT, bT, c);
						throwParseError(current, end, "Unexpected type");
					}
				} else {
					throw EOSException();
				}
			}
		}

		spec = a;
		return aT;
	}

	void printUnexpectedChar(SIt& from, SIt& to, char c) {
		cerr << "Expected " << c << ":" << endl
		 << "   '" << string(from, to) << "'" << endl;
	}

	double parseDouble(SIt& cursor, const SIt& end) {
		SIt it = cursor;
		char c = *cursor;
		while(cursor != end && (isdigit(c) || c == '.')) {
			cursor++;
			c = *cursor;
		}


		double out = stod(string(it, cursor));
		return out;
	}

	string consumeName(SIt& cursor, const SIt& end) {
		SIt start = cursor;
		char c = *cursor;

		locale loc;
		while(cursor != end && (isalpha(c, loc) || c == '_' || isdigit(c))) {
			cursor++;
			c = *cursor;
		} 

		return string(start, cursor);
	}

	template <typename T>
	boost::shared_ptr<T> createReferenceSpec(const string& name) {
		boost::shared_ptr<T> out = boost::shared_ptr<T>(new T());
		out->set_reference_name(name);
		return out;
	}


	void parseScope(SIt& current, const SIt& end, ScopeSpec& out) {
		if (consumeSpaces(current, end))
			throw EOSException();

		char c = *current;
		if (c != '{') {
			printUnexpectedChar(current, current, '{');
			throwParseError(current, end, "Unexpected type");
		}

		do {
			current++;
			if (consumeSpaces(current, end))
				throw EOSException();

			ScopeEntry se;
			EType t = parseScopeEntry(current, end, se);
			if (typeMap.find(se.name) != typeMap.end()) {
				cerr << "Redefinition of '" << se.name << "'" << endl;
				throwParseError(current, end, "Redefinition of expression");
			}
			typeMap[se.name] = t;
			out.push_back(se);

			// cout << "Added entry with name: '" << se.name << "'" << endl;
			// cout << "current: " << *current << endl;

			c = *current;
		} while(c == ';');

		if (c != '}') {
			throwParseError(current, end, "Expected '}' got: " + c);
		}

		current++;
	}

	EType parseScopeEntry(SIt& current, const SIt& end, ScopeEntry& out) {
		if (consumeSpaces(current, end))
			throw EOSException();

		out.name = consumeName(current, end);

		if (consumeSpaces(current, end))
			throw EOSException();

		char c = *current;
		if (c == '=') {
			current++;
			return parseExpression(out.spec, current, end);
		}

		return NONE;
	}

	EType parseAttributeAccess(SpecPtr& spec, SIt& current, const SIt& end, EType t) {

		if (current == end)
			throw EOSException();

		char c = *current;
		if (c == '.') {
			current++;

			string attr = consumeName(current, end);

			switch(t) {
				case VECTOR:
					if (attr.compare("x") == 0) {
						DoubleXCoordOfSpecPtr temp = DoubleXCoordOfSpecPtr(new DoubleXCoordOfSpec());
						temp->set_vector(dynamic_pointer_cast<VectorSpec>(spec));
						spec = temp;
						return parseAttributeAccess(spec, current, end, DOUBLE);
					} else if (attr.compare("y") == 0) {
						DoubleYCoordOfSpecPtr temp = DoubleYCoordOfSpecPtr(new DoubleYCoordOfSpec());
						temp->set_vector(dynamic_pointer_cast<VectorSpec>(spec));
						spec = temp;
						return parseAttributeAccess(spec, current, end, DOUBLE);
					} else if (attr.compare("z") == 0) {
						DoubleZCoordOfSpecPtr temp = DoubleZCoordOfSpecPtr(new DoubleZCoordOfSpec());
						temp->set_vector(dynamic_pointer_cast<VectorSpec>(spec));
						spec = temp;
						return parseAttributeAccess(spec, current, end, DOUBLE);
					} else {
						cerr << "Unrecognized attribute '" << attr << "' " << endl;
						throwParseError(current, end, "Unexpected type");
					}
				case FRAME:
					if (attr.compare("pos") == 0) {
						VectorOriginOfSpecPtr temp = VectorOriginOfSpecPtr(new VectorOriginOfSpec());
						temp->set_frame(dynamic_pointer_cast<FrameSpec>(spec));
						spec = temp;
						return parseAttributeAccess(spec, current, end, VECTOR);
					} else if (attr.compare("rot") == 0) {
						spec = OrientationOfSpecPtr(new OrientationOfSpec(dynamic_pointer_cast<FrameSpec>(spec)));
						return parseAttributeAccess(spec, current, end, ROTATION);
					} else {
						cerr << "Unrecognized attribute '" << attr << "' " << endl;
						throwParseError(current, end, "Unexpected type");	
					}
				break;
				default:
					cerr << "Unknown attribute '" << attr << "' for type '" << typeNames[t] << "'" << endl;
					throwParseError(current, end, "Unexpected type");
			}
		}

		return t;
	}

	EType parseFactor(SpecPtr &spec, SIt& current, const SIt& end) {
		if (consumeSpaces(current, end))
			throw EOSException();

		SIt first1 = current;
		char c = *current;
		bool negate = c == '-';
		bool isConstDouble = false;
		if (negate) {
			current++;
			consumeSpaces(current, end);
		}

		if (current == end)
			throw EOSException();

		c = *current;

		EType t = NONE;

		if (c == '(') {
			current++;
			t = parseExpression(spec, current, end);
			if(consumeSpaces(current, end))
				throw EOSException();

			c = *current;
			if (c != ')') {
				printUnexpectedChar(first1, current, ')');
				throwParseError(current, end, "Unexpected type");
			}

			current++;

			t = parseAttributeAccess(spec, current, end, t);
		} else if (isdigit(c) || c == '.') {
			double d = parseDouble(current, end);
			if (negate)
				d = -d;

			spec = DoubleConstSpecPtr(new DoubleConstSpec(d));
			t = DOUBLE;
			isConstDouble = true;
		} else if (c == '"'){
			current++;

			if (current == end)
				throwParseError(current, end, "Unexpected type");

			SIt start = current;
			c = *current;
			while(current != end && c != '"') {
				c = *current;
				if (c == '"')
					break;
				current++;
			}

			if (c != '"')
				throwParseError(current, end, "Unexpected char. Expected '\"' got: "+c);

			spec = const_string_spec(string(start, current));
			current++;

			t = parseAttributeAccess(spec, current, end, STRING);
		} else {
			string name = consumeName(current, end);

			if (current != end && (*current) == '(') {
				// PARSE FUNCTION
				t = parseFunction(spec, name, current, end);
			} else {
				// RESOLVE TYPE ASSOCIATED WITH NAME
				if (typeMap.find(name) != typeMap.end()) {
					t = typeMap[name];

					switch(t) {
						case DOUBLE:
							spec = createReferenceSpec<DoubleReferenceSpec>(name);
							break;
						case VECTOR:
							spec = createReferenceSpec<VectorReferenceSpec>(name);
							break;
						case FRAME:
							spec = createReferenceSpec<FrameReferenceSpec>(name);
							break;
						case ROTATION:
							spec = createReferenceSpec<RotationReferenceSpec>(name);
							break;
						default:
							cerr << "Can not create reference of type '" << typeNames[t] << "': " << endl
								 << "   '" << string(first1, current) << "'" << endl;
							throwParseError(current, end, "Unexpected type"); 
					}

				} else {
					cerr << "Name '" << name << "' is undefined: " << endl
						 << "   '" << string(first1, current) << "'" << endl;
					throwParseError(current, end, "Unexpected type");
				}
			}

			t = parseAttributeAccess(spec, current, end, t);
		}

		if (negate && !isConstDouble) {
			switch(t) {
				case DOUBLE:
					{
					DoubleSpecPtr b = dynamic_pointer_cast<DoubleSpec>(spec);
					DoubleSubtractionSpecPtr temp = DoubleSubtractionSpecPtr(new DoubleSubtractionSpec());
					vector<DoubleSpecPtr> inputs {constDZeroPtr, b};
					temp->set_inputs(inputs);
					spec = temp;
					}
					break;
				case VECTOR:
					{
					VectorSpecPtr b = dynamic_pointer_cast<VectorSpec>(spec);
					VectorDoubleMultiplicationSpecPtr temp = VectorDoubleMultiplicationSpecPtr(new VectorDoubleMultiplicationSpec());
					temp->set_double(constDNegOnePtr);
					temp->set_vector(b);
					spec = temp;
					}
					break;
				default:
					cerr << "Can not negate '" << typeNames[t] << "' in: " << endl
						 << "   '" << string(first1, current) << "' " << endl;

				throwParseError(current, end, "Unexpected type");
			}
		}

		return t;
	}

	void printTypes(const vector<EType>& v) {
		for(size_t i = 0; i < v.size(); i++)
			cerr << v[i] << " ";
	}

	int constDoubleToInt(const SpecPtr& spec) {
		DoubleConstSpecPtr p = dynamic_pointer_cast<DoubleConstSpec>(spec);
		if (p)
			return round(p->get_value());
		else {
			cerr << "UNABLE TO CAST TO CONST DOUBLE!" << endl;
			throw exception();
		}
	}

	EType parseFunction(SpecPtr &spec, const string& name, SIt& current, const SIt& end) {
		vector<SpecPtr> specs;
		vector<EType> types;

		if (name.compare("abs") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				AbsSpecPtr temp = AbsSpecPtr(new AbsSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");
		
		} else if (name.compare("sin") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				SinSpecPtr temp = SinSpecPtr(new SinSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("cos") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				CosSpecPtr temp = CosSpecPtr(new CosSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");
		
		} else if (name.compare("tan") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				TanSpecPtr temp = TanSpecPtr(new TanSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");
		
		} else if (name.compare("asin") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				ASinSpecPtr temp = ASinSpecPtr(new ASinSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("acos") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				ACosSpecPtr temp = ACosSpecPtr(new ACosSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");
		
		} else if (name.compare("atan") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == DOUBLE) {
				ATanSpecPtr temp = ATanSpecPtr(new ATanSpec());
				temp->set_value(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("fmod") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == DOUBLE && types[1] == DOUBLE) {
				FmodSpecPtr temp = FmodSpecPtr(new FmodSpec());
				temp->set_nominator(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				temp->set_denominator(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");
		
		} else if (name.compare("inputScalar") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == STRING) {
				spec = DoubleInputSpecPtr(new DoubleInputSpec(dynamic_pointer_cast<StringSpec>(specs[0])));
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("inputJoint") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == STRING) {
				spec = JointInputSpecPtr(new JointInputSpec(dynamic_pointer_cast<StringSpec>(specs[0])));
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("inputVec3") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == STRING) {
				spec = VectorInputSpecPtr(new VectorInputSpec(dynamic_pointer_cast<StringSpec>(specs[0])));
				return VECTOR;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("inputRot") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == STRING) {
				spec = RotationInputSpecPtr(new RotationInputSpec(dynamic_pointer_cast<StringSpec>(specs[0])));
				return ROTATION;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("inputFrame") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == STRING) {
				spec = FrameInputSpecPtr(new FrameInputSpec(dynamic_pointer_cast<StringSpec>(specs[0])));
				return FRAME;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("max") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == DOUBLE && types[1] == DOUBLE) {
				MaxSpecPtr temp = MaxSpecPtr(new MaxSpec());
				temp->set_lhs(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				temp->set_rhs(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	
		
		} else if (name.compare("min") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == DOUBLE && types[1] == DOUBLE) {
				MinSpecPtr temp = MinSpecPtr(new MinSpec());
				temp->set_lhs(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				temp->set_rhs(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	

		} else if (name.compare("if") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 3 && types[0] == DOUBLE && types[1] == DOUBLE && types[2] == DOUBLE) {
				DoubleIfSpecPtr temp = DoubleIfSpecPtr(new DoubleIfSpec());
				temp->set_condition(dynamic_pointer_cast<DoubleSpec>(specs[0]));
				temp->set_if(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				temp->set_else(dynamic_pointer_cast<DoubleSpec>(specs[2]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("norm") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == VECTOR) {
				DoubleNormOfSpecPtr temp = DoubleNormOfSpecPtr(new DoubleNormOfSpec());
				temp->set_vector(dynamic_pointer_cast<VectorSpec>(specs[0]));
				spec = temp;
				return DOUBLE;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[VECTOR] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("frame") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == ROTATION && types[1] == VECTOR) {
				spec = FrameConstructorSpecPtr(new FrameConstructorSpec(dynamic_pointer_cast<VectorSpec>(specs[1]), dynamic_pointer_cast<RotationSpec>(specs[0])));
				return FRAME;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[ROTATION] << ", " << typeNames[VECTOR] << ")" << endl;
			throwParseError(current, end, "Unexpected type");	
		
		} else if (name.compare("controllableConstraint") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 4 && types[0] == DOUBLE && types[1] == DOUBLE && types[2] == DOUBLE && types[3] == STRING) {
				ControllableConstraintSpecPtr temp = ControllableConstraintSpecPtr(new ControllableConstraintSpec());
				temp->lower_ = dynamic_pointer_cast<DoubleSpec>(specs[0]);
				temp->upper_ = dynamic_pointer_cast<DoubleSpec>(specs[1]);
				temp->weight_ = dynamic_pointer_cast<DoubleSpec>(specs[2]);
				temp->input_ = dynamic_pointer_cast<StringSpec>(specs[3]);
				spec = temp;
				return CTRLC;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[INTEGER] << ", " << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("hardConstraint") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 3 && types[0] == DOUBLE && types[1] == DOUBLE && types[2] == DOUBLE) {
				HardConstraintSpecPtr temp = HardConstraintSpecPtr(new HardConstraintSpec());
				temp->lower_ = dynamic_pointer_cast<DoubleSpec>(specs[0]);
				temp->upper_ = dynamic_pointer_cast<DoubleSpec>(specs[1]);
				temp->expression_ = dynamic_pointer_cast<DoubleSpec>(specs[2]);
				spec = temp;
				return HARDC;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");		
		
		} else if (name.compare("softConstraint") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 5 && types[0] == DOUBLE && types[1] == DOUBLE && types[2] == DOUBLE && types[3] == DOUBLE && types[4] == STRING) {
				SoftConstraintSpecPtr temp = SoftConstraintSpecPtr(new SoftConstraintSpec());
				temp->lower_ = dynamic_pointer_cast<DoubleSpec>(specs[0]);
				temp->upper_ = dynamic_pointer_cast<DoubleSpec>(specs[1]);
				temp->weight_ = dynamic_pointer_cast<DoubleSpec>(specs[2]);
				temp->expression_ = dynamic_pointer_cast<DoubleSpec>(specs[3]);
				temp->name_ = dynamic_pointer_cast<StringSpec>(specs[4]);
				spec = temp;
				return SOFTC;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[INTEGER] << ", " << typeNames[STRING] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("rotation") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == VECTOR && types[1] == DOUBLE) {
				AxisAngleSpecPtr temp = AxisAngleSpecPtr(new AxisAngleSpec());
				temp->set_axis(dynamic_pointer_cast<VectorSpec>(specs[0]));
				temp->set_angle(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				spec = temp;
				return ROTATION;
			} else if (types.size() == 4 && types[0] == DOUBLE && types[1] == DOUBLE && types[2] == DOUBLE && types[3] == DOUBLE) {
				double vals[4];
				for (size_t i = 0; i < 4; i++) {
					DoubleConstSpecPtr cd = dynamic_pointer_cast<DoubleConstSpec>(specs[i]);
					if (cd)
						vals[i] = cd->get_value();
					else {
						cerr << "Quaternions can only be used with constants!" << endl;
						throwParseError(current, end, "Unexpected type");
					}
				}
				spec = RotationQuaternionConstructorSpecPtr(new RotationQuaternionConstructorSpec(vals[0], vals[1], vals[2], vals[3]));
				return ROTATION;
			}
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[VECTOR] << ", " << typeNames[DOUBLE] << ")" << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");		
		
		} else if (name.compare("slerp") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 3 && types[0] == ROTATION && types[1] == ROTATION && types[2] == DOUBLE) {
				SlerpSpecPtr temp = SlerpSpecPtr(new SlerpSpec());
				temp->set_from(dynamic_pointer_cast<RotationSpec>(specs[0]));
				temp->set_to(dynamic_pointer_cast<RotationSpec>(specs[1]));
				temp->set_param(dynamic_pointer_cast<DoubleSpec>(specs[1]));
				spec = temp;
				return ROTATION;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[ROTATION] << ", " << typeNames[ROTATION] << ", " << typeNames[DOUBLE] << ")" << endl;
			throwParseError(current, end, "Unexpected type");			

		} else if (name.compare("originOf") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == FRAME) {
				VectorOriginOfSpecPtr temp = VectorOriginOfSpecPtr(new VectorOriginOfSpec());
				temp->set_frame(dynamic_pointer_cast<FrameSpec>(specs[0]));
				spec = temp;
				return VECTOR;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[FRAME] << ")" << endl;
			throwParseError(current, end, "Unexpected type");		
		
		} else if (name.compare("orientationOf") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == FRAME) {
				spec = OrientationOfSpecPtr(new OrientationOfSpec(dynamic_pointer_cast<FrameSpec>(specs[0])));
				return ROTATION;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[FRAME] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("cross") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 2 && types[0] == VECTOR && types[1] == VECTOR) {
				VectorCrossSpecPtr temp = VectorCrossSpecPtr(new VectorCrossSpec());
				temp->set_lhs(dynamic_pointer_cast<VectorSpec>(specs[0]));
				temp->set_rhs(dynamic_pointer_cast<VectorSpec>(specs[1]));
				spec = temp;
				return VECTOR;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[VECTOR] << ", " << typeNames[VECTOR] << ")" << endl;
			throwParseError(current, end, "Unexpected type");			
		
		} else if (name.compare("vec3") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == ROTATION) {
				VectorRotationVectorSpecPtr temp = VectorRotationVectorSpecPtr(new VectorRotationVectorSpec());
				temp->set_rotation(dynamic_pointer_cast<RotationSpec>(specs[0]));
				spec = temp;
				return VECTOR;
			} else if (types.size() == 3 && types[0] == DOUBLE && types[1] == DOUBLE && types[1] == DOUBLE) {
				spec = VectorConstructorSpecPtr(new VectorConstructorSpec(dynamic_pointer_cast<DoubleSpec>(specs[0]), dynamic_pointer_cast<DoubleSpec>(specs[1]), dynamic_pointer_cast<DoubleSpec>(specs[2])));
				return VECTOR;
			}
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ", " << typeNames[DOUBLE] << ")" << endl
				 << "   " << name << "(" << typeNames[ROTATION] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else if (name.compare("invert") == 0) {
			parseNTuple(specs, types, current, end);
			if (types.size() == 1 && types[0] == ROTATION) {
				spec = InverseRotationSpecPtr(new InverseRotationSpec(dynamic_pointer_cast<RotationSpec>(specs[0])));
				return ROTATION;
			} else if (types.size() == 1 && types[0] == FRAME) {
				spec = InverseFrameSpecPtr(new InverseFrameSpec(dynamic_pointer_cast<FrameSpec>(specs[0])));
				return FRAME;
			} 
			cerr << "No overload for " << name << " that takes: ";
			printTypes(types);
			cerr << endl << "Candidates are: " << endl
				 << "   " << name << "(" << typeNames[ROTATION] << ")" << endl
				 << "   " << name << "(" << typeNames[FRAME] << ")" << endl;
			throwParseError(current, end, "Unexpected type");

		} else {
			throwParseError(current, end, "Unknown function " + name);
		}

		return NONE;
	}

	void parseNTuple(vector<SpecPtr>& specs, vector<EType>& types, SIt& current, const SIt& end) {
		if (current != end) {
			char c = *current;
			char clB = ')';
			char delim = ',';

			if (c == '(' || c == '{') {
				if (c == '{') {
					clB = '}';
					delim = ';';
				}

				do {
					current++;
					SpecPtr a;
					EType aT = parseExpression(a, current, end);
					
					if (aT != NONE) {
						specs.push_back(a);
						types.push_back(aT);
					}

					if (consumeSpaces(current, end))
						throw EOSException();

					c = *current;
				} while(c == delim);

				if (c != clB) {
					printUnexpectedChar(current, current, clB);
					throwParseError(current, end, "Unexpected char");
				}

				current++;
			} else {
				printUnexpectedChar(current, current, '(');
				throwParseError(current, end, "Unexpected char");
			}
		} else {
			throw EOSException();
		}
	}
};
}