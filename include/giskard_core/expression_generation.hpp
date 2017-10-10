/*
 * Copyright (C) 2015-2017 Georg Bartels <georg.bartels@cs.uni-bremen.de>
 *                         Adrian Röfer <aroefer@uni-bremen.de>
 *
 * This file is part of giskard.
 *
 * giskard is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GISKARD_CORE_EXPRESSION_GENERATION_HPP
#define GISKARD_CORE_EXPRESSION_GENERATION_HPP

#include <giskard_core/scope.hpp>
#include <giskard_core/qp_controller.hpp>
#include <giskard_core/specifications.hpp>

namespace giskard_core
{

  class HardConstraint {
    public:
      void find_inputs(std::map<std::string, InputPtr>& inputs) {
        findInputs(lower_, inputs);
        findInputs(upper_, inputs);
        findInputs(expression_, inputs);
      }

      KDL::Expression<double>::Ptr lower_, upper_, expression_;
  };

  typename typedef boost::shared_ptr<HardConstraint> HardConstraintPtr;

  class SoftConstraint {
    public:
      void find_inputs(std::map<std::string, InputPtr>& inputs) {
        findInputs(lower_, inputs);
        findInputs(upper_, inputs);
        findInputs(expression_, inputs);
        findInputs(weight_, inputs);
      }

      KDL::Expression<double>::Ptr lower_, upper_, expression_, weight_;
      std::string name_;
  };

  typename typedef boost::shared_ptr<SoftConstraint> SoftConstraintPtr;

  class ControllableConstraint {
    public:
      void find_inputs(std::map<std::string, InputPtr>& inputs) {
        findInputs(lower_, inputs);
        findInputs(upper_, inputs);
        findInputs(weight_, inputs);
      }

      KDL::Expression<double>::Ptr lower_, upper_, weight_;
      std::string input_;
  };

  typename typedef boost::shared_ptr<ControllableConstraint> ControllableConstraintPtr;

  inline giskard_core::Scope generate(const giskard_core::ScopeSpec& scope_spec, const std::vector<std::string>& controllables)
  {
    giskard_core::Scope scope;

    std::vector<const InputSpec*> temp;
    for(size_t i=0; i<scope_spec.size(); ++i) {
      scope_spec[i].spec->get_input_specs(temp);
    }

    for(size_t i = 0; i < controllables.size(); i++) {
      bool found = false;
      for (size_t n = 0; n < temp.size(); n++) {
        if (controllables[i] == temp[n]->get_name() && temp[n]->get_type() == giskard_core::tJoint) {
          scope.add_joint_input(temp[n]->get_name());
          found = true;
          break;
        }
      }
      if (!found)
        throw std::domain_error("Scope generation: Could not find joint input with name '" + controllables[i] + "' as required by controllables.");
    }

    for (size_t i = 0; i < temp.size(); i++) {
      switch (temp[i]->get_type()) {
        case giskard_core::tScalar:
          scope.add_scalar_input(temp[i]->get_name());
          break;
        case giskard_core::tJoint:
          scope.add_joint_input(temp[i]->get_name());
          break;
        case giskard_core::tVector3:
          scope.add_vector_input(temp[i]->get_name());
          break;
        case giskard_core::tRotation:
          scope.add_rotation_input(temp[i]->get_name());
          break;
        case giskard_core::tFrame:
          scope.add_frame_input(temp[i]->get_name());
          break;
        default:
          throw std::domain_error("Scope generation: found input of non-supported type.");
      }
    }

    for(size_t i=0; i<scope_spec.size(); ++i)
    {
      std::string name = scope_spec[i].name;
      giskard_core::SpecPtr spec = scope_spec[i].spec;

      if(boost::dynamic_pointer_cast<giskard_core::DoubleSpec>(spec).get())
        scope.add_double_expression(name,
            boost::dynamic_pointer_cast<giskard_core::DoubleSpec>(spec)->get_expression(scope));
      else if(boost::dynamic_pointer_cast<giskard_core::VectorSpec>(spec).get())
        scope.add_vector_expression(name,
            boost::dynamic_pointer_cast<giskard_core::VectorSpec>(spec)->get_expression(scope));
      else if(boost::dynamic_pointer_cast<giskard_core::FrameSpec>(spec).get())
        scope.add_frame_expression(name,
            boost::dynamic_pointer_cast<giskard_core::FrameSpec>(spec)->get_expression(scope));
      else if(boost::dynamic_pointer_cast<giskard_core::RotationSpec>(spec).get())
        scope.add_rotation_expression(name,
            boost::dynamic_pointer_cast<giskard_core::RotationSpec>(spec)->get_expression(scope));
      else if(boost::dynamic_pointer_cast<giskard_core::AliasReferenceSpec>(spec).get())
      {
        // generation of alias references is extraordinarily convoluted;
        // it is a feature that was added relatively late... sorry!
        KDL::ExpressionBase::Ptr exp =
          boost::dynamic_pointer_cast<giskard_core::AliasReferenceSpec>(spec)->get_expression(scope);
        if (boost::dynamic_pointer_cast<KDL::Expression<double>>(exp).get())
          scope.add_double_expression(name, boost::dynamic_pointer_cast<KDL::Expression<double>>(exp));
        else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Vector>>(exp).get())
          scope.add_vector_expression(name, boost::dynamic_pointer_cast<KDL::Expression<KDL::Vector>>(exp));
        else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Rotation>>(exp).get())
          scope.add_rotation_expression(name, boost::dynamic_pointer_cast<KDL::Expression<KDL::Rotation>>(exp));
        else if (boost::dynamic_pointer_cast<KDL::Expression<KDL::Frame>>(exp).get())
          scope.add_frame_expression(name, boost::dynamic_pointer_cast<KDL::Expression<KDL::Frame>>(exp));
        else
          throw std::domain_error("Error during generation of alias reference! Could not cast into any existing expression type. This is an giskard-internal error that should not happen.");
      }
      else
        throw std::domain_error("Scope generation: found entry of non-supported type.");
    }

    return scope;
  }

  inline giskard_core::Scope generate(const giskard_core::ScopeSpec& scope_spec) {
    std::vector<std::string> empty;
    return generate(scope_spec, empty);
  }

  inline giskard_core::QPController generate(const std::vector<ControllableConstraintPtr>& controllable_constraints,
    const std::vector<SoftConstraintPtr>& soft_constraints,
    const std::vector<HardConstraintPtr>& hard_constraints)
  {
    // generate controllable constraints and collect joint names
    std::vector<std::string> controllable_name;
    std::map<std::string, InputPtr> input_map;

    std::vector< KDL::Expression<double>::Ptr > controllable_lower, controllable_upper, controllable_weight;
    for(size_t i=0; i<controllable_constraints_.size(); ++i)
    {
      controllable_lower.push_back(controllable_constraints[i].lower_);
      controllable_upper.push_back(controllable_constraints[i].upper_);
      controllable_weight.push_back(controllable_constraints[i].weight_);

      // Collect controllable names
      controllable_name.push_back(controllable_constraints[i]->input_);
      controllable_constraints[i]->find_inputs(input_map);
    }

    // generate soft constraints
    std::vector< KDL::Expression<double>::Ptr > soft_lower, soft_upper, soft_weight, soft_exp;
    std::vector< std::string> soft_name;
    for(size_t i=0; i<soft_constraints_.size(); ++i)
    {

      soft_lower.push_back(soft_constraints[i].lower_);
      soft_upper.push_back(soft_constraints[i].upper_);
      soft_weight.push_back(soft_constraints[i].weight_);
      soft_exp.push_back(soft_constraints[i].expression_);
      soft_name.push_back(soft_constraints[i].name_);
    }

    // generate hard constraints
    std::vector< KDL::Expression<double>::Ptr > hard_lower, hard_upper, hard_exp;
    for(size_t i=0; i<hard_constraints_.size(); ++i)
    {
      hard_lower.push_back(hard_constraints_[i].lower_);
      hard_upper.push_back(hard_constraints_[i].upper_);
      hard_exp.push_back(hard_constraints_[i].expression_);
    }

    std::unordered_set<std::string> remainingInputs;
    for (auto it = input_map.begin(); it != input_map.end(); it++) {
      remainingInputs.insert(it->first);
    }

    int nextIndex = 0;
    for(size_t i = 0; i < controllable_name.size(); i++) {
      auto it = input_map.find(controllable_name[i]);
      if (it != input_map.end()) {
        if (controllable_name[i] == it->second->name_ && it->second->get_type() == giskard_core::tJoint) {
          it->second->update_indices(nextIndex);
          remainingInputs.erase(it->first);
          nextIndex++;
        } else {
          throw std::domain_error("Controller generation: Could not find joint input with name '" + controllables[i] + "' as required by controllables.");
        }
      }
    }

    auto it = remainingInputs.begin();
    while(it != remainingInputs.end()) {
      it->update_indices(nextIndex);

      switch (it->get_type()) {
        case giskard_core::tScalar:
        case giskard_core::tJoint:
          nextIndex++;
          break;
        case giskard_core::tVector3:
          nextIndex += 3;
          break;
        case giskard_core::tRotation:
          nextIndex += 4;
          break;
        case giskard_core::tFrame:
          nextIndex += 7;
          break;
        default:
          throw std::domain_error("Controller generation: found input of non-supported type.");
      }

      it = remainingInputs.erase(it);
    }

    giskard_core::QPController controller;

    if(!(controller.init(controllable_lower, controllable_upper, controllable_weight,
                           controllable_name, soft_exp, soft_lower, soft_upper,
                           soft_weight, soft_name, hard_exp, hard_lower, hard_upper)))
      throw std::runtime_error("QPController generation: Init of controller failed.");

    return controller;
  }
}

#endif // GISKARD_CORE_EXPRESSION_GENERATION_HPP
