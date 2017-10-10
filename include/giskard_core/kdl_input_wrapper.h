
namespace giskard_core {
enum InputType {
    tScalar,
    tJoint,
    tVector3,
    tRotation,
    tFrame
  };

struct AInput {
	AInput(std::string name) : name_(name) {}
	virtual InputType get_type() const = 0;
	virtual void update_indices(int idx) = 0;
	const std::string name_;
};

struct JointInput : public AInput, public KDL::InputType {
	JointInput(std::string name, int idx)
	: AInput(name)
	, KDL::InputType(idx, 0.0) {}

	void update_indices(int idx) {
		if (idx >= 0)
			variable_number = idx;
		else
			throw std::domain_error("Input index may not be negative!");
	}

	static std::string type_string() { return "joint"; }
	InputType get_type() const { return tJoint; };
};

struct ScalarInput : public AInput, public KDL::InputType {
	ScalarInput(std::string name, int idx)
	: AInput(name)
	, KDL::InputType(idx, 0.0) {}

	void update_indices(int idx) {
		if (idx >= 0)
			variable_number = idx;
		else
			throw std::domain_error("Input index may not be negative!");
	}

	static std::string type_string() { return "scalar"; }
	InputType get_type() const { return tScalar; };
};

struct Vec3Input : public AInput, public KDL::Vector_DoubleDoubleDouble {
	Vec3Input(std::string name, int idx)
	: AInput(name)
	, x_(KDL::input(idx))
	, y_(KDL::input(idx + 1))
	, z_(KDL::input(idx + 2))
	, KDL::Vector_DoubleDoubleDouble(x_, y_, z_) {}

	void update_indices(int idx) {
		if (idx >= 0) {
			x_->variable_number = idx;
			y_->variable_number = idx + 1;
			z_->variable_number = idx + 2;
		} else
			throw std::domain_error("Input index may not be negative!");
	}

	static std::string type_string() { return "vec3"; }
	InputType get_type() const { return tVector3; };
private:
	KDL::InputType::Ptr x_, y_, z_;
};

struct RotationInput : public AInput, public KDL::RotVec_Double {
	RotationInput(std::string name, int idx)
	: AInput(name)
	, x_(KDL::input(idx))
	, y_(KDL::input(idx + 1))
	, z_(KDL::input(idx + 2))
	, a_(KDL::input(idx + 3))
	, KDL::RotVec_Double(KDL::vector(x_, y_, z_), a_) {}

	void update_indices(int idx) {
		if (idx >= 0) {
			x_->variable_number = idx;
			y_->variable_number = idx + 1;
			z_->variable_number = idx + 2;
			a_->variable_number = idx + 3;
		} else
			throw std::domain_error("Input index may not be negative!");
	}

	static std::string type_string() { return "rotation"; }
	InputType get_type() const { return tRotation; };
	const KDL::Expression<KDL::Rotation>::Ptr expr_;
private:
	KDL::InputType::Ptr x_, y_, z_, a_;
};

struct FrameInput : public AInput, public KDL::Frame_RotationVector {
	FrameInput(std::string name, int idx)
	: AInput(name)
	, rx_(KDL::input(idx))
	, ry_(KDL::input(idx + 1))
	, rz_(KDL::input(idx + 2))
	,  a_(KDL::input(idx + 3))
	,  x_(KDL::input(idx + 4))
	,  y_(KDL::input(idx + 5))
	,  z_(KDL::input(idx + 6))
	, KDL::Frame_RotationVector(KDL::rotVec(KDL::vector(rx_, ry_, rz_), a_), KDL::vector(x_, y_, z_)) {}

	void update_indices(int idx) {
		if (idx >= 0) {
			rx_->variable_number = idx;
			ry_->variable_number = idx + 1;
			rz_->variable_number = idx + 2;
			 a_->variable_number = idx + 3;
			 x_->variable_number = idx + 4;
			 y_->variable_number = idx + 5;
			 z_->variable_number = idx + 6;
		} else
			throw std::domain_error("Input index may not be negative!");
	}

	static std::string type_string() { return "frame"; }
	InputType get_type() const { return tFrame; };
	const KDL::Expression<KDL::Frame>::Ptr expr_;
private:
	KDL::InputType::Ptr rx_, ry_, rz_, a_, x_, y_, z_;
};

typedef typename boost::shared_ptr<AInput> InputPtr;
typedef typename boost::shared_ptr<const AInput> ConstInputPtr;
typedef typename boost::shared_ptr<JointInput> JointInputPtr;
typedef typename boost::shared_ptr<ScalarInput> ScalarInputPtr;
typedef typename boost::shared_ptr<Vec3Input> Vec3InputPtr;
typedef typename boost::shared_ptr<RotationInput> RotationInputPtr;
typedef typename boost::shared_ptr<FrameInput> FrameInputPtr;

template<typename T>
void findSubInputs(Expression<T> exprPtr, std::map<std::string, InputPtr>& inputs);

template<>
void findInputs(KDL::ExpressionBase::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
	if (boost::dynamic_pointer_cast<KDL::Expression<double>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::Expression<double>>(exprPtr), inputs);
	} else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Vector>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::Expression<KDL::Vector>>(exprPtr), inputs);
	} else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Rotation>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::Expression<KDL::Rotation>>(exprPtr), inputs);
	} else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Frame>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::Expression<KDL::Frame>>(exprPtr), inputs);
	}
}

template<>
void findInputs(KDL::Expression<double>::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
	if (boost::dynamic_pointer_cast<ScalarInput>(exprPtr)) {
		ScalarInputPtr ptr = boost::dynamic_pointer_cast<ScalarInput>(exprPtr);
		auto it = inputs.find(ptr->name_);
		if (it != input.end() && it->second->get_type() != ptr->get_type()) {
			throw std::domain_error("Tried to add scalar input '" + it->first + "' but the name is already taken.");
		} else if (it == inputs.end()) {
			inputs[ptr->name_] = ptr;
		}
	} else if (boost::dynamic_pointer_cast<JointInput>(exprPtr)) {
		JointInputPtr ptr = boost::dynamic_pointer_cast<JointInput>(exprPtr);
		auto it = inputs.find(ptr->name_);
		if (it != input.end() && it->second->get_type() != ptr->get_type()) {
			throw std::domain_error("Tried to add joint input '" + it->first + "' but the name is already taken.");
		} else if (it == inputs.end()) {
			inputs[ptr->name_] = ptr;
		}
	} else {
		findSubInputs<double>(exprPtr);
	}
}

template<>
void findInputs(KDL::Expression<KDL::Vector>::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
	if (boost::dynamic_pointer_cast<Vec3Input>(exprPtr)) {
		Vec3InputPtr ptr = boost::dynamic_pointer_cast<Vec3Input>(exprPtr);
		auto it = inputs.find(ptr->name_);
		if (it != input.end() && it->second->get_type() != ptr->get_type()) {
			throw std::domain_error("Tried to add vector input '" + it->first + "' but the name is already taken.");
		} else if (it == inputs.end()) {
			inputs[ptr->name_] = ptr;
		}
	} else {
		findSubInputs<KDL::Vector>(exprPtr);
	}
}

template<>
void findInputs(KDL::Expression<KDL::Rotation>::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
	if (boost::dynamic_pointer_cast<RotationInput>(exprPtr)) {
		RotationInputPtr ptr = boost::dynamic_pointer_cast<RotationInput>(exprPtr);
		auto it = inputs.find(ptr->name_);
		if (it != input.end() && it->second->get_type() != ptr->get_type()) {
			throw std::domain_error("Tried to add rotation input '" + it->first + "' but the name is already taken.");
		} else if (it == inputs.end()) {
			inputs[ptr->name_] = ptr;
		}
	} else {
		findSubInputs<KDL::Rotation>(exprPtr);
	}
}

template<>
void findInputs(KDL::Expression<KDL::Frame>::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
	if (boost::dynamic_pointer_cast<FrameInput>(exprPtr)) {
		FrameInputPtr ptr = boost::dynamic_pointer_cast<FrameInput>(exprPtr);
		auto it = inputs.find(ptr->name_);
		if (it != input.end() && it->second->get_type() != ptr->get_type()) {
			throw std::domain_error("Tried to add frame input '" + it->first + "' but the name is already taken.");
		} else if (it == inputs.end()) {
			inputs[ptr->name_] = ptr;
		}
	} else {
		findSubInputs<KDL::Frame>(exprPtr);
	}
}

template<typename RType>
void findSubInputs(Expression<RType>::Ptr exprPtr, std::map<std::string, InputPtr>& inputs) {
    if (boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, double>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, double>>(exprPtr)->argument, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Vector>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Vector>>(exprPtr)->argument, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Rotation>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Rotation>>(exprPtr)->argument, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Frame>>(exprPtr)) {
		findInputs(boost::dynamic_pointer_cast<KDL::UnaryExpression<RType, KDL::Frame>>(exprPtr)->argument, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, double, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Vector, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Rotation, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::BinaryExpression<RType, KDL::Frame, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);

	}
	///////////////// TERNARY DOUBLE
	else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, double>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, double>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	}
	/////////////// TERNARY VECTOR
	else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Vector>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Vector>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	}
	/////////////// TERNARY ROTATION
	else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Rotation>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Rotation>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	}
	/////////////// TERNARY FRAME
	else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, double, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Vector, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Rotation, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, double, KDL::Frame, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, double, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Vector, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Rotation, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Vector, KDL::Frame, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, double, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Vector, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Rotation, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Rotation, KDL::Frame, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, double, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Vector, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Rotation, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	} else if (boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Frame>>(exprPtr)) {
		auto ptr = boost::dynamic_pointer_cast<KDL::TernaryExpression<RType, KDL::Frame, KDL::Frame, KDL::Frame>>(exprPtr);
		findInputs(ptr->argument1, inputs);
		findInputs(ptr->argument2, inputs);
		findInputs(ptr->argument3, inputs);

	}

}