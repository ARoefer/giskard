#ifndef GISKARD_GISKARD_PARSER_HPP
#define GISKARD_GISKARD_PARSER_HPP

#include <giskard/giskard.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include <string>
#include <sstream>
#include <tuple>

//#define INCLUDE_ROSPKG
#ifdef INCLUDE_ROSPKG
#include <ros/package.h>
#endif


#include <unordered_map>

namespace giskard {

	using boost::static_pointer_cast;
	using boost::dynamic_pointer_cast;

	typedef typename std::vector<std::vector<gkDType>> TArgOverloads;

	template<class T>
	boost::shared_ptr<T> instance() {
		return boost::shared_ptr<T>(new T());
	}

	template<class T, typename A>
	boost::shared_ptr<T> instance(A a) {
		return boost::shared_ptr<T>(new T(a));
	}

	template<class T, typename A, typename B>
	boost::shared_ptr<T> instance(A a, B b) {
		return boost::shared_ptr<T>(new T(a, b));
	}

	template<class T, typename A, typename B, typename C>
	boost::shared_ptr<T> instance(A a, B b, C c) {
		return boost::shared_ptr<T>(new T(a, b, c));
	}

	template<class T, typename A, typename B, typename C, typename D>
	boost::shared_ptr<T> instance(A a, B b, C c, D d) {
		return boost::shared_ptr<T>(new T(a, b, c, d));
	}

	template<class T, typename A, typename B, typename C, typename D, typename E>
	boost::shared_ptr<T> instance(A a, B b, C c, D d, E e) {
		return boost::shared_ptr<T>(new T(a, b, c, d, e));
	}

	struct IToken {
		virtual SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) = 0;
		virtual std::string toString() = 0;
		virtual bool isValid() = 0;
	};

	typedef typename boost::shared_ptr<IToken> ITokenPtr;

	const DoubleSpecPtr constNegOne(new DoubleConstSpec(-1.0));
	const DoubleSpecPtr constZero(new DoubleConstSpec(0.0));

	struct NotImplementedException : public std::exception {
		const char* what() noexcept { return "TODO"; }
	};

// ----------------------------------------------------------------------------------------


	struct OperandException : ScopeException {
		OperandException(const Scope2Ptr& _scope, char _op, const gkDType _lhs, const gkDType _rhs, const TArgOverloads& _overloads)
		: ScopeException(_scope)
		, op(_op)
		, lhs(_lhs)
		, rhs(_rhs)
		, overloads(_overloads)
		{}
		
		virtual const char* what() const noexcept {
		  std::stringstream out;
		  out << ScopeException::what()
			  << "\n| No overload know for '" << op << "' matching '" << gkTypeNames[lhs] << " " << op << " " << gkTypeNames[rhs] << "'"
			  << "| Known overloads: \n";
		  for (size_t i = 0; i < overloads.size(); i++) {
			out << "|     ";
			for (size_t n = 0; n < overloads[i].size(); n++) {
				out << (n > 0? "," : "") << gkTypeNames[overloads[i][n]];
			}
			out << "\n";
		  }

		  return out.str().c_str();
		}
	  private:
		char op;
		const TArgOverloads& overloads;
		const gkDType lhs;
		const gkDType rhs;
	};

	struct UnaryOperandException : ScopeException {
		UnaryOperandException(const Scope2Ptr& _scope, char _op, const gkDType _got, const TArgOverloads& _overloads)
		: ScopeException(_scope)
		, op(_op)
		, got(_got)
		, overloads(_overloads)
		{}
		
		virtual const char* what() const noexcept {
		  std::stringstream out;
		  out << ScopeException::what()
			  << "\n| No overload know for '" << op << "' matching '" << op << " " << gkTypeNames[got] << "'"
			  << "| Known overloads: \n";
		  for (size_t i = 0; i < overloads.size(); i++) {
			out << "|     ";
			for (size_t n = 0; n < overloads[i].size(); n++) {
				out << (n > 0? "," : "") << gkTypeNames[overloads[i][n]];
			}
			out << "\n";
		  }

		  return out.str().c_str();
		}
	  private:
		char op;
		const TArgOverloads& overloads;
		const gkDType got;
	};

	struct FunctionException : ScopeException {
		FunctionException(const Scope2Ptr& _scope, std::string _name, const std::vector<SpecPtr>& _got, const TArgOverloads& _overloads)
		: ScopeException(_scope)
		, name(_name)
		, got(_got)
		, overloads(_overloads)
		{}

		virtual const char* what() const noexcept {
			std::stringstream out;
			out << ScopeException::what()
			<< "\n| No overload know for '" << name << "' matching '" << name << "("; 

			for (size_t i = 0; i < got.size(); i++) {
				if (i > 0)
					out << ", ";
				out << gkTypeNames[got[i]->type()];
			}

			out << ")\n | Known overloads: \n";
			for (size_t i = 0; i < overloads.size(); i++) {
				out << "|     ";
				for (size_t n = 0; n < overloads[i].size(); n++) {
					out << (n > 0? "," : "") << gkTypeNames[overloads[i][n]];
				}
				out << "\n";
			}

			return out.str().c_str();
		}
	private:
		std::string name;
		const TArgOverloads& overloads;
		const std::vector<SpecPtr>& got;
	};

// ----------------------------------------------------------------------------------------

	struct ConstScalar : public IToken {
		ConstScalar(double _value) 
		: value(_value)
		{}

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			return DoubleConstSpecPtr(new DoubleConstSpec(value));
		}

		std::string toString() {
			return "ConstScalar(" + std::to_string(value) + ")";
		}

		bool isValid() {
			return true;
		}

		const double value;
	};

	typedef typename boost::shared_ptr<ConstScalar> ConstScalarPtr;

	ConstScalarPtr const_scalar(double value) {
		return ConstScalarPtr(new ConstScalar(value));
	}

// ----------------------------------------------------------------------------------------

	struct ConstString : public IToken {
		ConstString(std::string _value) 
		: value(_value)
		{}

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			return StringSpecPtr(new StringSpec(value));
		}

		std::string toString() {
			return "ConstString(" + value + ")";
		}		

		bool isValid() {
			return value.compare("") != 0;
		}

	private:
		std::string value;
	};

	typedef typename boost::shared_ptr<ConstString> ConstStringPtr;

	ConstStringPtr const_string_constructor(std::string value) {
		return ConstStringPtr(new ConstString(value));
	}

// ----------------------------------------------------------------------------------------

	struct StarOperator : public IToken {
		StarOperator(ITokenPtr _lhs, ITokenPtr _rhs) 
		: lhs(_lhs)
		, rhs(_rhs)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr left = lhs->generateSpec(local, global);
			SpecPtr right = rhs->generateSpec(local, global);

			gkDType lt = left->type();
			gkDType rt = right->type();

			if (lt == rt) {
				if (lt == tScalar) {
					DoubleSpecPtr lC = static_pointer_cast<DoubleSpec>(left);
					DoubleSpecPtr rC = static_pointer_cast<DoubleSpec>(right);

					return instance<DoubleMultiplicationSpec>(lC, rC);
				} else if (lt == tVec3) {
					VectorSpecPtr lC = static_pointer_cast<VectorSpec>(left);
					VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

					return instance<VectorDotSpec>(lC, rC);
				} else if (lt == tRotation) {
					RotationSpecPtr lC = static_pointer_cast<RotationSpec>(left);
					RotationSpecPtr rC = static_pointer_cast<RotationSpec>(right);

					return instance<RotationMultiplicationSpec>(lC, rC);
				} else if (lt == tFrame) {
					FrameSpecPtr lC = static_pointer_cast<FrameSpec>(left);
					FrameSpecPtr rC = static_pointer_cast<FrameSpec>(right);

					return instance<FrameMultiplicationSpec>(lC, rC);
				}
			} else {
				if (lt == tScalar && rt == tVec3) {
					DoubleSpecPtr lC = static_pointer_cast<DoubleSpec>(left);
					VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

					return instance<VectorDoubleMultiplicationSpec>(lC, rC);
				} else if (lt == tVec3 && rt == tScalar) {
					VectorSpecPtr lC = static_pointer_cast<VectorSpec>(left);
					DoubleSpecPtr rC = static_pointer_cast<DoubleSpec>(right);

					return instance<VectorDoubleMultiplicationSpec>(rC, lC);
				} else if (lt == tRotation && rt == tVec3){
					RotationSpecPtr lC = static_pointer_cast<RotationSpec>(left);
					VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

					return instance<VectorRotationMultiplicationSpec>(lC, rC);
				} else if (lt == tFrame && rt == tVec3){
					FrameSpecPtr lC = static_pointer_cast<FrameSpec>(left);
					VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

					return instance<VectorFrameMultiplicationSpec>(lC, rC);
				}
			}
			throw OperandException(local, '*', lt, rt, overloads);
		}

		std::string toString() {
			return "StarOperator(" + (lhs? lhs->toString() : "NULL") + ", " + (rhs? rhs->toString() : "NULL") + ")";
		}

		bool isValid() {
			return (lhs && rhs) && (lhs->isValid() && rhs->isValid());
		}

	private:
		ITokenPtr lhs;
		ITokenPtr rhs;
	};

	const TArgOverloads StarOperator::overloads {{tScalar, tScalar}, {tVec3, tVec3}, {tRotation, tRotation}, {tFrame, tFrame}, {tScalar, tVec3}, {tVec3, tScalar}, {tRotation, tVec3}, {tFrame, tVec3}};

	typedef typename boost::shared_ptr<StarOperator> StarOperatorPtr;

	StarOperatorPtr star_operator(ITokenPtr lhs, ITokenPtr rhs) {
		return StarOperatorPtr(new StarOperator(lhs, rhs));
	}

// ----------------------------------------------------------------------------------------

	struct SlashOperator : public IToken {
		SlashOperator(ITokenPtr _lhs, ITokenPtr _rhs) 
		: lhs(_lhs)
		, rhs(_rhs)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr left = lhs->generateSpec(local, global);
			SpecPtr right = rhs->generateSpec(local, global);

			gkDType lt = left->type();
			gkDType rt = right->type();

			if (lt == rt && lt == tScalar) {
				DoubleSpecPtr lC = static_pointer_cast<DoubleSpec>(left);
				DoubleSpecPtr rC = static_pointer_cast<DoubleSpec>(right);

				return instance<DoubleDivisionSpec>(lC, rC);
			}
			throw OperandException(local, '/', lt, rt, overloads);
		}
		
		std::string toString() {
			return "SlashOperator(" + (lhs? lhs->toString() : "NULL") + ", " + (rhs? rhs->toString() : "NULL") + ")";
		}

		bool isValid() {
			return (lhs && rhs) && (lhs->isValid() && rhs->isValid());
		}
	private:
		ITokenPtr lhs;
		ITokenPtr rhs;
	};

	const TArgOverloads SlashOperator::overloads {{tScalar, tScalar}};

	typedef typename boost::shared_ptr<SlashOperator> SlashOperatorPtr;

	SlashOperatorPtr slash_operator(ITokenPtr lhs, ITokenPtr rhs) {
		return SlashOperatorPtr(new SlashOperator(lhs, rhs));
	}

// ----------------------------------------------------------------------------------------

	struct PlusOperator : public IToken {
		PlusOperator(ITokenPtr _lhs, ITokenPtr _rhs) 
		: lhs(_lhs)
		, rhs(_rhs)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr left = lhs->generateSpec(local, global);
			SpecPtr right = rhs->generateSpec(local, global);

			gkDType lt = left->type();
			gkDType rt = right->type();

			if (lt == rt && lt == tScalar) {
				DoubleSpecPtr lC = static_pointer_cast<DoubleSpec>(left);
				DoubleSpecPtr rC = static_pointer_cast<DoubleSpec>(right);

				return instance<DoubleAdditionSpec>(lC, rC);
			} else if (lt == rt && lt == tVec3) {
				VectorSpecPtr lC = static_pointer_cast<VectorSpec>(left);
				VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

				return instance<VectorAdditionSpec>(lC, rC);
			}
			throw OperandException(local, '+', lt, rt, overloads);	
		}
		
		std::string toString() {
			return "PlusOperator(" + (lhs? lhs->toString() : "NULL") + ", " + (rhs? rhs->toString() : "NULL") + ")";
		}

		bool isValid() {
			return (lhs && rhs) && (lhs->isValid() && rhs->isValid());
		}
	private:
		ITokenPtr lhs;
		ITokenPtr rhs;
	};

	const TArgOverloads PlusOperator::overloads {{tScalar, tScalar}, {tVec3, tVec3}};

	typedef typename boost::shared_ptr<PlusOperator> PlusOperatorPtr;

	PlusOperatorPtr plus_operator(ITokenPtr lhs, ITokenPtr rhs) {
		return PlusOperatorPtr(new PlusOperator(lhs, rhs));
	}

// ----------------------------------------------------------------------------------------

	struct MinusOperator : public IToken {
		MinusOperator(ITokenPtr _lhs, ITokenPtr _rhs) 
		: lhs(_lhs)
		, rhs(_rhs)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr left = lhs->generateSpec(local, global);
			SpecPtr right = rhs->generateSpec(local, global);

			gkDType lt = left->type();
			gkDType rt = right->type();

			if (lt == rt && lt == tScalar) {
				DoubleSpecPtr lC = static_pointer_cast<DoubleSpec>(left);
				DoubleSpecPtr rC = static_pointer_cast<DoubleSpec>(right);

				return instance<DoubleSubtractionSpec>(lC, rC);
			} else if (lt == rt && lt == tVec3) {
				VectorSpecPtr lC = static_pointer_cast<VectorSpec>(left);
				VectorSpecPtr rC = static_pointer_cast<VectorSpec>(right);

				return instance<VectorSubtractionSpec>(lC, rC);
			}
			throw OperandException(local, '-', lt, rt, overloads);	
		}
		
		std::string toString() {
			return "MinusOperator(" + (lhs? lhs->toString() : "NULL") + ", " + (rhs? rhs->toString() : "NULL") + ")";
		}

		bool isValid() {
			return (lhs && rhs) && (lhs->isValid() && rhs->isValid());
		}
	private:
		ITokenPtr lhs;
		ITokenPtr rhs;
	};

	const TArgOverloads MinusOperator::overloads {{tScalar, tScalar}, {tVec3, tVec3}};

	typedef typename boost::shared_ptr<MinusOperator> MinusOperatorPtr;

	MinusOperatorPtr minus_operator(ITokenPtr lhs, ITokenPtr rhs) {
		return MinusOperatorPtr(new MinusOperator(lhs, rhs));
	}

// ----------------------------------------------------------------------------------------

	struct ReferenceToken : public IToken {
		ReferenceToken(const std::string& _name) 
		: name(_name)
		{ }

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr spec;
			if (local == global) {
				spec = local->getSpec(name);
			} else {
				spec = local->getLocalSpec(name);
			}

			if (!spec)
				throw CustomScopeException(local, "'" + name + "' does not name an expression!");

			gkDType sT = spec->type();
			if (sT == tScalar) {
				DoubleSpecPtr pSp = static_pointer_cast<DoubleSpec>(spec);
				return instance<DoubleReferenceSpec>(name, local, pSp);
			} else if (sT == tVec3) {
				VectorSpecPtr pSp = static_pointer_cast<VectorSpec>(spec);
				return instance<VectorReferenceSpec>(name, local, pSp);
			} else if (sT == tRotation) {
				RotationSpecPtr pSp = static_pointer_cast<RotationSpec>(spec);
				return instance<RotationReferenceSpec>(name, local, pSp);
			} else if (sT == tFrame) {
				FrameSpecPtr pSp = static_pointer_cast<FrameSpec>(spec);
				return instance<FrameReferenceSpec>(name, local, pSp);
			} else if (sT == tList) {
				throw NotImplementedException();
			} else if (sT == tMap) {
				throw NotImplementedException();
			} else if (sT == tHardC) {
				throw NotImplementedException();
			} else if (sT == tSoftC) {
				throw NotImplementedException();
			} else if (sT == tContC) {
				throw NotImplementedException();
			} 

			return spec;
		}
		
		std::string toString() {
			return "ReferenceToken(" + name + ")";
		}

		bool isValid() {
			return name.compare("") != 0;
		}

		const std::string name;
	};

	typedef typename boost::shared_ptr<ReferenceToken> ReferenceTokenPtr;

	ReferenceTokenPtr reference_token(std::string name) {
		return ReferenceTokenPtr(new ReferenceToken(name));
	}

// ----------------------------------------------------------------------------------------

	struct AttributeOperator : public IToken {
		AttributeOperator(ITokenPtr _lhs, ITokenPtr _rhs) 
		: lhs(_lhs)
		, rhs(_rhs)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr left = lhs->generateSpec(local, global);
			gkDType lt = left->type();


			if (lt == tNamespace) {
				Scope2Ptr newLocal = static_pointer_cast<Scope2>(left);
				return rhs->generateSpec(newLocal, global);
			} else if (lt == tFrame) {
				ReferenceTokenPtr ref = dynamic_pointer_cast<ReferenceToken>(rhs);
				FrameSpecPtr lC = static_pointer_cast<FrameSpec>(left);

				if (!ref)
					throw CustomScopeException(local, "Right hand side of 'frame.?' must be a name!"); 
				
				if (ref->name.compare("pos") == 0) {
					return instance<VectorOriginOfSpec>(lC);
				} else if (ref->name.compare("rot") == 0) {
					return instance<OrientationOfSpec>(lC);
				}

				throw CustomScopeException(local, "'frame' does not have an attribute '" + ref->name + "'");
			} else if (lt == tVec3) {
				ReferenceTokenPtr ref = dynamic_pointer_cast<ReferenceToken>(rhs);
				VectorSpecPtr lC = static_pointer_cast<VectorSpec>(left);

				if (!ref)
					throw CustomScopeException(local, "Right hand side of 'vec3.?' must be a name!"); 
				
				if (ref->name.compare("x") == 0) {
					return instance<DoubleXCoordOfSpec>(lC);
				} else if (ref->name.compare("y") == 0) {
					return instance<DoubleYCoordOfSpec>(lC);
				} else if (ref->name.compare("z") == 0) {
					return instance<DoubleZCoordOfSpec>(lC);
				}

				throw CustomScopeException(local, "'vec3' does not have an attribute '" + ref->name + "'");
			}

			throw CustomScopeException(local, "'"+gkTypeNames[lt]+"' does not have attributes");
		}
		
		std::string toString() {
			return "AttributeOperator(" + (lhs? lhs->toString() : "NULL") + ", " + (rhs? rhs->toString() : "NULL") + ")";
		}

		bool isValid() {
			return (lhs && rhs) && (lhs->isValid() && rhs->isValid());
		}
	private:
		ITokenPtr lhs;
		ITokenPtr rhs;
	};

	const TArgOverloads AttributeOperator::overloads {{tNamespace, tNamespace}, {tVec3, tNamespace}, {tFrame, tNamespace}};

	typedef typename boost::shared_ptr<AttributeOperator> AttributeOperatorPtr;

	AttributeOperatorPtr attribute_operator(ITokenPtr lhs, ITokenPtr rhs) {
		return AttributeOperatorPtr(new AttributeOperator(lhs, rhs));
	}

// ----------------------------------------------------------------------------------------

	struct NegationOperator : public IToken {
		NegationOperator(ITokenPtr _factor) 
		: factor(_factor)
		{ }

		static const TArgOverloads overloads;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			ConstScalarPtr cnstScal = dynamic_pointer_cast<ConstScalar>(factor);
			if (cnstScal) {
				return instance<DoubleConstSpec>(-cnstScal->value);
			}

			SpecPtr spec = factor->generateSpec(local, global);
			gkDType t = spec->type();

			if (t == tScalar) {
				DoubleSpecPtr sC = static_pointer_cast<DoubleSpec>(spec);
				return instance<DoubleSubtractionSpec>(constZero, sC);
			} else if (t == tVec3) {
				VectorSpecPtr sC = static_pointer_cast<VectorSpec>(spec);
				return instance<VectorDoubleMultiplicationSpec>(constNegOne, sC);
			}

			throw UnaryOperandException(local, '-', t, overloads);
		}
		

		std::string toString() {
			return "NegationOperator(" + (factor? factor->toString() : "NULL") + ")";
		}

		bool isValid() {
			return factor && factor->isValid();
		}
	private:
		ITokenPtr factor;
	};

	const TArgOverloads NegationOperator::overloads {{tScalar}, {tVec3}};

	typedef typename boost::shared_ptr<NegationOperator> NegationOperatorPtr;

	NegationOperatorPtr negation_operator(ITokenPtr value) {
		return NegationOperatorPtr(new NegationOperator(value));
	}
	

// ----------------------------------------------------------------------------------------

	struct NamedExpression : public IToken {
		NamedExpression(std::string _name, ITokenPtr _expression) 
		: name(_name)
		, expression(_expression) {}

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			SpecPtr spec = expression->generateSpec(local, global);
			local->addNamedSpec(name, spec);

			return spec;
		}

		std::string toString() {
			return "NamedExpression(" + name + ", " + (expression? expression->toString() : "NULL") + ")";
		}

		bool isValid() {
			return name.compare("") != 0 && expression && expression->isValid();
		}
	private:
		std::string name;
		ITokenPtr expression;
	};

	typedef typename boost::shared_ptr<NamedExpression> NamedExpressionPtr;

	NamedExpressionPtr named_expression(std::string name, ITokenPtr expression) {
		return NamedExpressionPtr(new NamedExpression(name, expression));
	}

// ----------------------------------------------------------------------------------------

	struct FunctionCall : public IToken {
		FunctionCall(std::string _name, std::vector<ITokenPtr> _args) 
		: name(_name)
		, args(_args)
		{ }

		static const TArgOverloads singleScalarOL;
		static const TArgOverloads binaryScalarOL;
		static const TArgOverloads trinaryScalarOL;
		static const TArgOverloads singleVectorOL;
		static const TArgOverloads frameConstructorOL;

		static const TArgOverloads rotationConstructorOL;
		static const TArgOverloads slerpOL;
		static const TArgOverloads crossOL;
		static const TArgOverloads vec3OL;
		static const TArgOverloads invertOL;

		static const TArgOverloads controllableCConstructorOL;
		static const TArgOverloads softCConstructorOL;
		static const TArgOverloads hardCConstructorOL;

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			std::vector<SpecPtr> as;

			if (local == global) {
				if (name.compare("abs") == 0) {
					generateArgs(as, global);				

					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<AbsSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("sin") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<SinSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("asin") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<ASinSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("cos") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<CosSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("acos") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<ACosSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("tan") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<TanSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("atan") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tScalar) {	
						return instance<ATanSpec>(static_pointer_cast<DoubleSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleScalarOL);
				} else if (name.compare("fmod") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tScalar && 
						as[1]->type() == tScalar) {	
						return instance<FmodSpec>(static_pointer_cast<DoubleSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]));
					}

					throw FunctionException(local, name, as, binaryScalarOL);
				} else if (name.compare("max") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tScalar && 
						as[1]->type() == tScalar) {	
						return instance<MaxSpec>(static_pointer_cast<DoubleSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]));
					}

					throw FunctionException(local, name, as, binaryScalarOL);
				} else if (name.compare("min") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tScalar && 
						as[1]->type() == tScalar) {	
						return instance<MinSpec>(static_pointer_cast<DoubleSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]));
					}

					throw FunctionException(local, name, as, binaryScalarOL);
				} else if (name.compare("if") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 3 && 
						as[0]->type() == tScalar && 
						as[1]->type() == tScalar &&
						as[2]->type() == tScalar) {	
						return instance<DoubleIfSpec>(static_pointer_cast<DoubleSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]), static_pointer_cast<DoubleSpec>(as[2]));
					}

					throw FunctionException(local, name, as, trinaryScalarOL);
				} else if (name.compare("norm") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && as[0]->type() == tVec3) {	
						return instance<DoubleNormOfSpec>(static_pointer_cast<VectorSpec>(as[0]));
					}

					throw FunctionException(local, name, as, singleVectorOL);
				} else if (name.compare("frame") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tRotation && 
						as[1]->type() == tVec3) {	
						return instance<FrameConstructorSpec>(static_pointer_cast<VectorSpec>(as[1]), static_pointer_cast<RotationSpec>(as[0]));
					}

					throw FunctionException(local, name, as, frameConstructorOL);
				} else if (name.compare("rotation") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tVec3 && 
						as[1]->type() == tScalar) {	
						return instance<AxisAngleSpec>(static_pointer_cast<VectorSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]));
					} else if (as.size() == 4 &&
						as[0]->type() == tScalar && 
						as[1]->type() == tScalar &&
						as[2]->type() == tScalar &&
						as[3]->type() == tScalar) {
						DoubleConstSpecPtr d0 = dynamic_pointer_cast<DoubleConstSpec>(as[0]);
						DoubleConstSpecPtr d1 = dynamic_pointer_cast<DoubleConstSpec>(as[1]);
						DoubleConstSpecPtr d2 = dynamic_pointer_cast<DoubleConstSpec>(as[2]);
						DoubleConstSpecPtr d3 = dynamic_pointer_cast<DoubleConstSpec>(as[3]);

						if (!d0 || !d1 || !d2 || !d3) {
							throw CustomScopeException(local, "All values of the quaternion constructor must be constant!");
						}

						return instance<RotationQuaternionConstructorSpec>(d0->get_value(),d1->get_value(),d2->get_value(),d3->get_value());
					}

					throw FunctionException(local, name, as, rotationConstructorOL);
				} else if (name.compare("slerp") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 3 && 
						as[0]->type() == tRotation &&
						as[1]->type() == tRotation && 
						as[2]->type() == tScalar) {	
						return instance<SlerpSpec>(static_pointer_cast<RotationSpec>(as[0]), static_pointer_cast<RotationSpec>(as[1]), static_pointer_cast<DoubleSpec>(as[2]));
					}

					throw FunctionException(local, name, as, slerpOL);
				} else if (name.compare("cross") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 2 && 
						as[0]->type() == tVec3 &&
						as[1]->type() == tVec3) {	
						return instance<VectorCrossSpec>(static_pointer_cast<VectorSpec>(as[0]), static_pointer_cast<VectorSpec>(as[1]));
					}

					throw FunctionException(local, name, as, crossOL);
				} else if (name.compare("vec3") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && 
						as[0]->type() == tRotation) {	
						return instance<VectorRotationVectorSpec>(static_pointer_cast<RotationSpec>(as[0]));
					} else if (as.size() == 3 && 
						as[0]->type() == tScalar &&
						as[1]->type() == tScalar &&
						as[2]->type() == tScalar) {	
						return instance<VectorConstructorSpec>(static_pointer_cast<DoubleSpec>(as[0]), static_pointer_cast<DoubleSpec>(as[1]), static_pointer_cast<DoubleSpec>(as[2]));
					}

					throw FunctionException(local, name, as, vec3OL);
				} else if (name.compare("invert") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 1 && 
						as[0]->type() == tRotation) {	
						return instance<InverseRotationSpec>(static_pointer_cast<RotationSpec>(as[0]));
					} else if (as.size() == 1 && 
						as[0]->type() == tFrame) {	
						return instance<InverseFrameSpec>(static_pointer_cast<FrameSpec>(as[0]));
					}

					throw FunctionException(local, name, as, invertOL);
				} else if (name.compare("controllableC") == 0) {
					generateArgs(as, global);
					
					throw NotImplementedException();

				} else if (name.compare("softC") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 5 && 
						as[0]->type() == tScalar &&
						as[1]->type() == tScalar &&
						as[2]->type() == tScalar &&
						as[3]->type() == tScalar &&
						as[4]->type() == tString) {	
						return instance<SoftConstraintSpec>(static_pointer_cast<DoubleSpec>(as[0]), 
															static_pointer_cast<DoubleSpec>(as[1]),
															static_pointer_cast<DoubleSpec>(as[2]),
															static_pointer_cast<DoubleSpec>(as[3]),
															static_pointer_cast<StringSpec>(as[4])->get_value());
					}

					throw FunctionException(local, name, as, softCConstructorOL);
				} else if (name.compare("hardC") == 0) {
					generateArgs(as, global);
					
					if (as.size() == 3 && 
						as[0]->type() == tScalar &&
						as[1]->type() == tScalar &&
						as[2]->type() == tScalar) {	
						return instance<HardConstraintSpec>(static_pointer_cast<DoubleSpec>(as[0]), 
															static_pointer_cast<DoubleSpec>(as[1]),
															static_pointer_cast<DoubleSpec>(as[2]));
					}

					throw FunctionException(local, name, as, hardCConstructorOL);
				}
			}

			AFunctionSpecPtr fn; 
			if (local != global)
				fn = local->getLocalFunctionSpec(name);
			else
				fn = local->getFunctionSpec(name);


			if (!fn)
				throw SemanticException(local, "'" + name + "' does not name a function!");

			generateArgs(as, global);

			TArgOverloads ol;
			ol.resize(1);
			fn->getArgumentSignature(ol[0]);

			if (ol[0].size() == as.size()) {
				bool ok = true;
				for (size_t i = 0; i < as.size(); ++i) {
					ok = ok && ol[0][i] == as[i]->type();
				}

				if (ok) {
					gkDType fnT = fn->getType();
					if (fnT == tScalar) {
						DoubleFunctionSpecPtr pfn = static_pointer_cast<DoubleFunctionSpec>(fn);
						return instance<DoubleFunctionInstanceSpec>(pfn, as);
					} else if (fnT == tVec3) {
						VectorFunctionSpecPtr pfn = static_pointer_cast<VectorFunctionSpec>(fn);
						return instance<VectorFunctionInstanceSpec>(pfn, as);
					} else if (fnT == tRotation) {
						RotationFunctionSpecPtr pfn = static_pointer_cast<RotationFunctionSpec>(fn);
						return instance<RotationFunctionInstanceSpec>(pfn, as);
					} else if (fnT == tFrame) {
						FrameFunctionSpecPtr pfn = static_pointer_cast<FrameFunctionSpec>(fn);
						return instance<FrameFunctionInstanceSpec>(pfn, as);
					} else if (fnT == tList) {
						throw NotImplementedException();
					} else if (fnT == tMap) {
						throw NotImplementedException();
					} else if (fnT == tHardC) {
						throw NotImplementedException();
					} else if (fnT == tSoftC) {
						throw NotImplementedException();
					} else if (fnT == tContC) {
						throw NotImplementedException();
					} 
				}
			}

			throw FunctionException(local, fn->getName(), as, ol);
		}

		void generateArgs(std::vector<SpecPtr> &out, Scope2Ptr& scope) {
			for (size_t i = 0; i < args.size(); i++) {
				out.push_back(args[i]->generateSpec(scope, scope));
			}
		}
		
		std::string toString() {
			std::ostringstream out;
			out << "FunctionCall(" << name;
			for (size_t i = 0; i < args.size(); i++) {
				out << ", " <<(args[i]? args[i]->toString() : "NULL");
			}
			out << ")";
			return out.str();
		}

		bool isValid() {
			bool ok = name.compare("") != 0;
			for (size_t i = 0; i < args.size() && ok; i++) {
				ok = ok && args[i] && args[i]->isValid();
			}
			return ok;
		}
	private:
		std::string name;
		std::vector<ITokenPtr> args;
	};

	const TArgOverloads FunctionCall::singleScalarOL {{tScalar}};
	const TArgOverloads FunctionCall::singleVectorOL {{tVec3}};
	const TArgOverloads FunctionCall::binaryScalarOL {{tScalar, tScalar}};
	const TArgOverloads FunctionCall::trinaryScalarOL {{tScalar, tScalar, tScalar}};

	const TArgOverloads FunctionCall::frameConstructorOL {{tRotation, tVec3}};
	const TArgOverloads FunctionCall::rotationConstructorOL {{tVec3, tScalar}, {tScalar, tScalar, tScalar, tScalar}};
	const TArgOverloads FunctionCall::slerpOL {{tRotation, tRotation, tScalar}};
	const TArgOverloads FunctionCall::crossOL {{tVec3, tVec3}};
	const TArgOverloads FunctionCall::vec3OL {{tRotation}, {tScalar, tScalar, tScalar}};
	const TArgOverloads FunctionCall::invertOL {{tRotation}, {tFrame}};

	const TArgOverloads FunctionCall::controllableCConstructorOL {{tScalar, tScalar, tScalar, tScalar}};
	const TArgOverloads FunctionCall::softCConstructorOL {{tScalar, tScalar, tScalar, tScalar, tString}};
	const TArgOverloads FunctionCall::hardCConstructorOL {{tScalar, tScalar, tScalar}};


	typedef typename boost::shared_ptr<FunctionCall> FunctionCallPtr;

	FunctionCallPtr function_call(std::string name, std::vector<ITokenPtr> args) {
		return FunctionCallPtr(new FunctionCall(name, args));
	}

// ----------------------------------------------------------------------------------------

	typedef typename std::tuple<gkDType, std::string> TDeclaration;
	typedef typename std::vector<TDeclaration> TArgumentList;

	TDeclaration t_declaration(gkDType type, std::string name) {
		return TDeclaration(type, name);
	}

	inline std::ostream& operator << (std::ostream& os, const TDeclaration& d) 
	{
		os << "TDeclaration(" << gkTypeNames[std::get<0>(d)] << ", " << std::get<1>(d) << ")";
	    return os;
	}

	inline std::ostream& operator << (std::ostream& os, const TArgumentList& v) 
	{
	    os << "TArgumentList(";
	    std::string delim = "";
	    for (auto it = v.begin(); it != v.end(); ++it)
	    {
	        os << delim << *it;
	        delim = ", ";
	    }
	    os << ")";
	    return os;
	}

// ----------------------------------------------------------------------------------------

	struct FunctionDefinition : public IToken {
		FunctionDefinition(TDeclaration declaration, 
						   TArgumentList _arguments,
						   std::vector<ITokenPtr> _interna, 
						   ITokenPtr _returnExpression) 
		: returnType(std::get<0>(declaration))
		, name(std::get<1>(declaration))
		, arguments(_arguments)
		, interna(_interna)
		, returnExpression(_returnExpression)
		{ }

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			if (local->isNameTaken(name))
				throw CustomScopeException(local, "Name '"+name+"' is already taken.");

			Scope2Ptr localScope(new Scope2(local->filePath + "::" + name, local));

			std::vector<ISocketSpecPtr> sockets;
			for (TDeclaration decl: arguments) {
				std::string argName = std::get<1>(decl);
				switch (std::get<0>(decl)) {
					case tScalar: 
					{
						DoubleSocketSpecPtr sock(new DoubleSocketSpec(argName));
						SpecPtr asSpec = static_pointer_cast<Spec>(sock);
						localScope->addNamedSpec(argName, asSpec);
						sockets.push_back(sock); 
					}
					break;
					case tVec3: 
					{
						VectorSocketSpecPtr sock(new VectorSocketSpec(argName));
						SpecPtr asSpec = static_pointer_cast<Spec>(sock);
						localScope->addNamedSpec(argName, asSpec);
						sockets.push_back(sock); 
					}
					break;
					case tRotation: 
					{
						RotationSocketSpecPtr sock(new RotationSocketSpec(argName));
						SpecPtr asSpec = static_pointer_cast<Spec>(sock);
						localScope->addNamedSpec(argName, asSpec);
						sockets.push_back(sock); 
					}
					break;
					case tFrame: 
					{
						FrameSocketSpecPtr sock(new FrameSocketSpec(argName));
						SpecPtr asSpec = static_pointer_cast<Spec>(sock);
						localScope->addNamedSpec(argName, asSpec);
						sockets.push_back(sock); 
					}
					break;
					default:
						throw CustomScopeException(local, "Unsupported argument type in function definition! Type: " + gkTypeNames[std::get<0>(decl)]);
				}
			}

			for (size_t i = 0; i < interna.size(); i++) {
				interna[i]->generateSpec(localScope, localScope);
			}

			SpecPtr ret = returnExpression->generateSpec(localScope, localScope);
			if (ret->type() != returnType) {
				throw CustomScopeException(local, "Function was declared as " + gkTypeNames[returnType] + " but return is " + gkTypeNames[ret->type()]);
			}

			AFunctionSpecPtr function;
			switch (returnType) {
				case tScalar: 
				{
					function = instance<DoubleFunctionSpec>(local, name);
				}
				break;
				case tVec3: 
				{
					function = instance<VectorFunctionSpec>(local, name);
				}
				break;
				case tRotation: 
				{
					function = instance<RotationFunctionSpec>(local, name);
				}
				break;
				case tFrame: 
				{
					function = instance<FrameFunctionSpec>(local, name);
				}
				break;
				default:
					throw CustomScopeException(local, "Unsupported return type in function definition! Type: " + gkTypeNames[returnType]);
			}

			function->setArguments(sockets);
			local->addFunctionSpec(function);
		
			return SpecPtr();
		}
		
		std::string toString() {
			std::ostringstream out;
			out << "FunctionDefinition(" << gkTypeNames[returnType] << ", " << name << ", " << arguments << ") {" << std::endl;
			
			for (size_t i = 0; i < interna.size(); i++) {
				out << (interna[i]? interna[i]->toString() : "NULL") << ";" << std::endl;
			}

			out << "return " << (returnExpression? returnExpression->toString() : "NULL") << ";" << std::endl << "}";

			return out.str();
		}

		bool isValid() {
			bool ok = name.compare("") != 0;
			for (size_t i = 0; i < interna.size() && ok; i++) {
				ok = ok && interna[i] && interna[i]->isValid();
			}
			return ok && returnExpression && returnExpression->isValid();
		}
	private:
		gkDType returnType;
		std::string name;
		TArgumentList arguments;
		std::vector<ITokenPtr> interna;
		ITokenPtr returnExpression;
	};

	typedef typename boost::shared_ptr<FunctionDefinition> FunctionDefinitionPtr;

	ITokenPtr function_definition(std::tuple<gkDType, std::string> declaration, 
						   TArgumentList arguments,
						   std::vector<ITokenPtr> interna, 
						   ITokenPtr returnExpression) {
		return ITokenPtr(new FunctionDefinition(declaration, arguments, interna, returnExpression));
	}

// ----------------------------------------------------------------------------------------

	struct ListDefinition : public IToken {
		ListDefinition(std::vector<ITokenPtr> _elements) 
		: elements(_elements)
		{}

		SpecPtr generateSpec(Scope2Ptr& local, Scope2Ptr& global) {
			std::vector<SpecPtr> elems;

			gkDType type = tNONE;
			for (size_t i = 0; i < elements.size(); i++) {
				SpecPtr elem = elements[i]->generateSpec(local, global);
				if (type == tNONE)
					type = elem->type();

				if (type != elem->type())
					CustomScopeException(local, "List elements must be of the same type!");

				elems.push_back(elem);
			}

			switch(type) {
				case tScalar:
					return instance<ListSpec<DoubleSpec>>(elems);
				case tVec3:
					return instance<ListSpec<VectorSpec>>(elems);
				case tRotation:
					return instance<ListSpec<RotationSpec>>(elems);
				case tFrame:
					return instance<ListSpec<FrameSpec>>(elems);
				case tString:
					return instance<ListSpec<StringSpec>>(elems);
				case tList:
					return instance<ListSpec<AListSpec>>(elems);
				case tMap:
					return instance<ListSpec<AMapSpec>>(elems);
				case tHardC:
					return instance<ListSpec<HardConstraintSpec>>(elems);
				case tSoftC:
					return instance<ListSpec<SoftConstraintSpec>>(elems);
				case tContC:
					return instance<ListSpec<ControllableConstraintSpec>>(elems);
				default:
					throw CustomScopeException(local, "Unsupported inner type for list! Type: " + gkTypeNames[type]);
			}
		}

		std::string toString() {
			std::ostringstream out;
			out << "ListDefinition(";
			for (size_t i = 0; i < elements.size(); i++) {
				if (i > 0)
					out << ", ";
				out << (elements[i]? elements[i]->toString() : "NULL");
			}
			out << ")";

			return out.str();
		}

		bool isValid() {
			bool ok = true;
			for (size_t i = 0; i < elements.size() && ok; i++) {
				ok = ok && elements[i] && elements[i]->isValid();
			}
			return ok;
		}
	private:
		std::vector<ITokenPtr> elements;
	};
	
	typedef typename boost::shared_ptr<ListDefinition> ListDefinitionPtr;

	ListDefinitionPtr list_definition(std::vector<ITokenPtr> elements) {
		return ListDefinitionPtr(new ListDefinition(elements));
	}

// ----------------------------------------------------------------------------------------

	struct ImportOperation {
		ImportOperation(std::string _localPath, std::string _as)
		: localPath(_localPath)
		, as(_as)
		{}

		std::string resolvePath(std::string currentDir) {
			#ifdef INCLUDE_ROSPKG
			size_t pkgStart = localPath.find("$(");
			if (pkgStart != -1) {
				size_t end = localPath.find(")");
				std::string pkg = ros::package::getPath(localPath.substr(pkgStart + 2, end - 1));
				std::string path = localPath.substr(end + 1);
				
				return pkg + path;
			}
			#endif

			return currentDir + localPath;
		}

		std::string getAlias() {
			return as;
		}

		std::string toString() {
			return "ImportOperation(" + localPath + ", " + as + ")";
		}

		bool isValid() {
			return localPath.compare("") != 0;
		}

	private:
		std::string localPath;
		std::string as;
	};

	typedef typename boost::shared_ptr<ImportOperation> ImportOperationPtr;

	ImportOperationPtr import_operation(std::string path, std::string as) {
		return ImportOperationPtr(new ImportOperation(path, as));
	}

// ----------------------------------------------------------------------------------------

	struct IGiskardParser {
		virtual Scope2Ptr parse_giskard_from_file(const std::string& path) = 0;
		virtual Scope2Ptr parse_giskard_from_file(const std::string& path, std::unordered_map<std::string, Scope2Ptr>& loaded) = 0;
		virtual Scope2Ptr parse_giskard_from_msg(const std::string& msg) = 0;
		virtual Scope2Ptr parse_giskard_from_msg(const std::string& msg, std::unordered_map<std::string, Scope2Ptr>& loaded) = 0;
	};


	struct GiskardSpec {
		GiskardSpec(std::vector<ImportOperationPtr> _imports, std::vector<ITokenPtr> _content)
		: imports(_imports)
		, content(_content)
		{}

		Scope2Ptr generateScope(IGiskardParser* parser, const std::string& path, std::unordered_map<std::string, Scope2Ptr>& imported) {
			Scope2Ptr scope(new Scope2(path));
			//loadedScopes = imported;
			
			std::string baseDir = path.substr(0, path.rfind('/'));
			for (ImportOperationPtr p: imports) {
				std::string resolvedPath = p->resolvePath(baseDir);
				auto it = imported.find(resolvedPath);
				if (it != imported.end()) {
					if (p->getAlias().compare("") == 0) {
						scope->addSuperScope(it->second);
					} else {
						scope->addNamedScope(p->getAlias(), it->second);
					}
				} else {
					Scope2Ptr newScope = parser->parse_giskard_from_file(resolvedPath, imported);
					loadedScopes[resolvedPath] = newScope;
					if (p->getAlias().compare("") == 0) {
						scope->addSuperScope(newScope);
					} else {
						scope->addNamedScope(p->getAlias(), newScope);
					}
				}
			}

			for (size_t i = 0; i < content.size(); i++) {
				content[i]->generateSpec(scope, scope);
			}
		}

		std::string toString() {
			std::ostringstream out;
			out << "GiskardSpec(" << std::endl;
			for (size_t i = 0; i < imports.size(); i++) {
				out << (imports[i]? imports[i]->toString() : "NULL") << ";" << std::endl;
			}
			for (size_t i = 0; i < content.size(); i++) {
				out << (content[i]? content[i]->toString() : "NULL") << ";" << std::endl;
			}
			out << ")";

			return out.str();
		}

		bool isValid() {
			bool ok = true;
			for (size_t i = 0; i < imports.size() && ok; i++) {
				ok = ok && imports[i]->isValid();
			}

			for (size_t i = 0; i < content.size() && ok; i++) {
				ok = ok && content[i] && content[i]->isValid();
			}

			return ok;
		}
	private:
		std::unordered_map<std::string, Scope2Ptr> loadedScopes;
		std::vector<ImportOperationPtr> imports;
		std::vector<ITokenPtr> content;
	};

	typedef typename boost::shared_ptr<GiskardSpec> GiskardSpecPtr;

	GiskardSpecPtr giskard_spec(std::vector<ImportOperationPtr> imports, std::vector<ITokenPtr> content) {
		return GiskardSpecPtr(new GiskardSpec(imports, content));
	}

// ----------------------------------------------------------------------------------------

	namespace fusion = boost::fusion;
	namespace phoenix = boost::phoenix;
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;

	template<typename Iterator>
	struct giskard_skipper : public qi::grammar<Iterator> {

		giskard_skipper() : giskard_skipper::base_type(skip, "PL/0") {
			skip = ascii::space | (qi::lit('#') >> *(qi::char_ - qi::eol) >> qi::eol);
		}

		qi::rule<Iterator> skip;
	};

	template <typename Iterator, typename Skipper = giskard_skipper<Iterator>>
	struct giskard_grammar : qi::grammar<Iterator, GiskardSpecPtr(), Skipper> {
		
		giskard_grammar() 
		: giskard_grammar::base_type(giskardSpec) 
		{

			using qi::lit;
			using qi::lexeme;
			using qi::double_;
			using ascii::char_;
			using qi::_val;
			using qi::eps;
			using qi::_1;
			using qi::_2;
			using qi::_3;
			using qi::_4;		
			using phoenix::bind;

			giskardSpec = ((*(importStatement))                                             		   // Imports
						>> (*(namedExpression >> ';' | function)))[_val = bind(giskard_spec, _1, _2)]; // Definintions

			lstr %= lexeme[ //((char_("a-zA-Z") >> 
					+(char_ - (lit('"') | lit('.') | lit(':') 
				 			 | lit(';') | lit('+') | lit('-') 
				 			 | lit('*') | lit('/') | lit('(') 
				 			 | lit(')') | lit('=') | lit(',') 
				 			 | lit(' ') | lit('{') | lit('}')
				 			 | lit('[') | lit(']') ))]-keywords;
			
			quotedTempStr %= (lit('"') >> lexeme[*(char_ - '"')] >> lit('"'));
			quotedString = quotedTempStr[_val = bind(const_string_constructor, _1)];

			importStatement = ("import" >> quotedTempStr >> "as" >> lstr)[_val = bind(import_operation, _1, _2)] // Named import
							| ("import" >> quotedTempStr)[_val = bind(import_operation, _1, "")];                // Anonymous import

			namedExpression = (lstr >> lit('=') >> expression)[_val = bind(named_expression, _1, _2)];

			expression = (term >> lit('+') >> term)[_val = bind(plus_operator, _1, _2)]
					   | (term >> lit('-') >> term)[_val = bind(minus_operator, _1, _2)]
					   | term[_val = _1];

			term  = (factor >> lit('*') >> factor)[_val = bind(star_operator, _1, _2)] 
				  | (factor >> lit('/') >> factor)[_val = bind(slash_operator, _1, _2)]
				  | factor[_val = _1];
			
			factor = memberAccess[_val = _1]                                             // negatable Factors
				   | (lit('-') >> factor)[_val = bind(negation_operator, _1)]; // negated negatable factors

			memberAccess = (literal >> lit('.') >> memberAccess)[_val = bind(attribute_operator, _1, _2)]
						 | literal[_val = _1];

			argList %= (expression % ',');

			literal = (lit('(') >> expression >> lit(')'))[_val = _1] 
				 | (double_)[_val = bind(const_scalar, _1)]
				 | quotedString[_val = _1]
				 | varOrCall[_val = _1]
				 | (lit('[') >> expression % ',' >> lit(']'))[_val = bind(list_definition, _1)];

			varOrCall = (lstr >> lit('(') >> argList >> lit(')'))[_val = bind(function_call, _1, _2)]
					  | (lstr)[_val = bind(reference_token, _1)];


			argDefinitionsList %= lit('(') >> (declaration % ',') >> lit(')');
			declaration = (type >> lstr)[_val = bind(t_declaration, _1, _2)];

			function = ("def" >> declaration >> argDefinitionsList >> lit('{') 
						>> (*(namedExpression >> ';'))
						>> "return" >> expression >> lit(';')
						>> lit('}'))[_val = bind(function_definition, _1, _2, _3, _4)];


			keywords = type | "def" | "return" | "import";

			type = lit("scalar")[_val = gkDType::tScalar] 
				  | lit("vec3")[_val = gkDType::tVec3] 
				  | lit("rotation")[_val = gkDType::tRotation] 
				  | lit("frame")[_val = gkDType::tFrame] 
				  | lit("list")[_val = gkDType::tList] 
				  | lit("map")[_val = gkDType::tMap]
				  | lit("hardConst")[_val = gkDType::tHardC] 
				  | lit("softConst")[_val = gkDType::tSoftC] 
				  | lit("contConst")[_val = gkDType::tContC];
		}

		qi::rule<Iterator, std::string(), Skipper> lstr;
		qi::rule<Iterator, std::string(), Skipper> quotedTempStr;

		qi::rule<Iterator, GiskardSpecPtr(), Skipper> giskardSpec;

		qi::rule<Iterator, ITokenPtr(), Skipper> test;
		qi::rule<Iterator, ITokenPtr(), Skipper> namedExpression;
		qi::rule<Iterator, ITokenPtr(), Skipper> expression;
		qi::rule<Iterator, ITokenPtr(), Skipper> term;
		qi::rule<Iterator, ITokenPtr(), Skipper> factor;
		qi::rule<Iterator, ITokenPtr(), Skipper> varOrCall;
		qi::rule<Iterator, ITokenPtr(), Skipper> literal;
		qi::rule<Iterator, ITokenPtr(), Skipper> memberAccess;
		qi::rule<Iterator, ITokenPtr(), Skipper> quotedString;

		qi::rule<Iterator, ITokenPtr(), Skipper> function;
		qi::rule<Iterator, ImportOperationPtr(), Skipper> importStatement;

		qi::rule<Iterator, std::vector<ITokenPtr>(), Skipper> argList;

		qi::rule<Iterator, giskard::gkDType(), Skipper> type;
		qi::rule<Iterator, TArgumentList(), Skipper> argDefinitionsList;
		qi::rule<Iterator, TDeclaration(), Skipper> declaration;
		qi::rule<Iterator, Skipper> keywords;
	};

	class GiskardParser : public IGiskardParser {
	public:
		Scope2Ptr parse_giskard_from_file(const std::string& path) {
			std::unordered_map<std::string, Scope2Ptr> loaded;
			return parse_giskard_from_file(path, loaded);
		}

		Scope2Ptr parse_giskard_from_file(const std::string& path, std::unordered_map<std::string, Scope2Ptr>& loaded) {
			// open file, disable skipping of whitespace
			std::ifstream in(path);
			in.unsetf(std::ios::skipws);

			if (!in.is_open()) {
				throw std::exception();
			}
			 
			// wrap istream into iterator
			boost::spirit::istream_iterator begin(in);
			boost::spirit::istream_iterator end;

			return parse_giskard(begin, end, "", loaded);
		}

		Scope2Ptr parse_giskard_from_msg(const std::string& msg) {
			std::unordered_map<std::string, Scope2Ptr> loaded;
			return parse_giskard_from_file(msg, loaded);
		}

		Scope2Ptr parse_giskard_from_msg(const std::string& msg, std::unordered_map<std::string, Scope2Ptr>& loaded) {
			typedef std::string::const_iterator itType;

			giskard_grammar<itType> g;
			giskard_skipper<itType> skip;

			auto begin = msg.begin();
			auto end = msg.end();

			return parse_giskard(begin, end, "", loaded);
		}

	private:
		template <typename Iterator>
		Scope2Ptr parse_giskard(Iterator begin, Iterator end, std::string path, std::unordered_map<std::string, Scope2Ptr>& loaded) {
			using boost::spirit::qi::phrase_parse;

			// open file, disable skipping of whitespace
			giskard_grammar<Iterator> g;
			giskard_skipper<Iterator> skip;

			GiskardSpecPtr temp;

			bool r = phrase_parse(begin, end, g, skip, temp);

			if (begin != end) {
				std::cerr << "parsing failed! Remaining: '" << std::string(begin, end) << "'" << std::endl;
				throw std::exception();
			}

			return temp->generateScope(this, path, loaded);
		}			
	};

}

#endif