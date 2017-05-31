/*
 * Copyright (C) 2015 Georg Bartels <georg.bartels@cs.uni-bremen.de>
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

#ifndef GISKARD_SCOPE_HPP
#define GISKARD_SCOPE_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <giskard/expressiontree.hpp>

namespace giskard
{
  class Scope
  {
    public:
      enum InputTypes {
        Scalar,
        Joint,
        Vector,
        Rotation,
        Frame
      };

      struct ScopeInput {
        std::string name;
        InputTypes type;
        size_t idx;
      };

      Scope() 
      : bJointvectorCompleted(false)
      , nextInputIndex(0) {}

      const KDL::Expression<double>::Ptr& find_double_expression(const std::string& reference_name) const
      {
        if(!has_double_expression(reference_name))
          throw std::invalid_argument("Could not find double expression with name: "+ reference_name);

        std::map< std::string, KDL::Expression<double>::Ptr >::const_iterator it =
            double_references_.find(reference_name);

        return it->second;
      }

      const KDL::Expression<KDL::Vector>::Ptr& find_vector_expression(const std::string& reference_name) const
      {
        if(!has_vector_expression(reference_name))
          throw std::invalid_argument("Could not find vector expression with name: "+ reference_name);

        std::map< std::string, KDL::Expression<KDL::Vector>::Ptr >::const_iterator it =
            vector_references_.find(reference_name);

        return it->second;
      }

      const KDL::Expression<KDL::Rotation>::Ptr& find_rotation_expression(const std::string& reference_name) const
      {
        if(!has_rotation_expression(reference_name))
          throw std::invalid_argument("Could not find rotation expression with name: "+ reference_name);


        std::map< std::string, KDL::Expression<KDL::Rotation>::Ptr >::const_iterator it =
            rotation_references_.find(reference_name);

        return it->second;
      }

      const KDL::Expression<KDL::Frame>::Ptr& find_frame_expression(const std::string& reference_name) const
      {
        if(!has_frame_expression(reference_name))
          throw std::invalid_argument("Could not find frame expression with name: "+ reference_name);

        std::map< std::string, KDL::Expression<KDL::Frame>::Ptr >::const_iterator it =
            frame_references_.find(reference_name);

        return it->second;
      }

      bool has_double_expression(const std::string& expression_name) const
      {
        return (double_references_.count(expression_name) == 1);
      }

      bool has_vector_expression(const std::string& expression_name) const
      {
        return (vector_references_.count(expression_name) == 1);
      }

      bool has_rotation_expression(const std::string& expression_name) const
      {
        return (rotation_references_.count(expression_name) == 1);
      }

      bool has_frame_expression(const std::string& expression_name) const
      {
        return (frame_references_.count(expression_name) == 1);
      }

      void add_double_expression(const std::string& reference_name, const KDL::Expression<double>::Ptr& expression)
      {
        if(has_double_expression(reference_name))
          throw std::invalid_argument("Could not add double expression to scope because name already taken: "
              + reference_name);

        double_references_[reference_name] = expression;
      }

      void add_vector_expression(const std::string& reference_name, const KDL::Expression<KDL::Vector>::Ptr& expression)
      {
        if(has_vector_expression(reference_name))
          throw std::invalid_argument("Could not add vector expression to scope because name already taken: "
              + reference_name);

        vector_references_[reference_name] = expression;
      }

      void add_rotation_expression(const std::string& reference_name, const KDL::Expression<KDL::Rotation>::Ptr& expression)
      {
        if(has_rotation_expression(reference_name))
          throw std::invalid_argument("Could not add rotation expression to scope because name already taken: "
              + reference_name);

        rotation_references_[reference_name] = expression;
      }

      void add_frame_expression(const std::string& reference_name, const KDL::Expression<KDL::Frame>::Ptr& expression)
      {
        if(has_frame_expression(reference_name))
          throw std::invalid_argument("Could not add frame expression to scope because name already taken: "
              + reference_name);

        frame_references_[reference_name] = expression;
      }

      // Add a joint input
      KDL::Expression<double>::Ptr add_joint_input(const std::string& name) {
        auto it = inputs.find(name);
        if (it != inputs.end()) {
          if (it->second.type == Joint) {
            return KDL::input(it->second.idx);
          }

          throw std::invalid_argument("Can't add joint input with name '" + name + "'. The name is already taken.");
        }

        if (bJointvectorCompleted)
          throw std::invalid_argument("Can't add joint input with name '" + name + "'. Joints always need to be the first inputs in the controller.");


        KDL::Expression<double>::Ptr out = KDL::input(nextInputIndex);
        inputs[name] = { name, Joint, nextInputIndex };
        jointInputs.push_back(name);

        nextInputIndex++;
        return out;
      }

      // Add a scalar input
      KDL::Expression<double>::Ptr add_scalar_input(const std::string& name) {
        auto it = inputs.find(name);
        if (it != inputs.end()) {
          if (it->second.type == Scalar) {
            return KDL::input(it->second.idx);
          }

          throw std::invalid_argument("Can't add scalar input with name '" + name + "'. The name is already taken.");
        }

        bJointvectorCompleted = true;

        KDL::Expression<double>::Ptr out = KDL::input(nextInputIndex);
        inputs[name] = { name, Scalar, nextInputIndex };

        nextInputIndex++;
        return out;
      }

      // Add a vector input
      KDL::Expression<KDL::Vector>::Ptr add_vector_input(const std::string& name) {
        auto it = inputs.find(name);
        if (it != inputs.end()) {
          if (it->second.type == Vector) {
            return KDL::vector(KDL::input(it->second.idx), KDL::input(it->second.idx + 1), KDL::input(it->second.idx + 2));
          }
          
          throw std::invalid_argument("Can't add vector input with name '" + name + "'. The name is already taken.");
        }

        bJointvectorCompleted = true;
        KDL::Expression<KDL::Vector>::Ptr out = KDL::vector(KDL::input(nextInputIndex), 
                                                       KDL::input(nextInputIndex + 1), 
                                                       KDL::input(nextInputIndex + 2));
        inputs[name] = { name, Vector, nextInputIndex };

        nextInputIndex += 3;
        return out;
      }

      // Add a rotation input
      KDL::Expression<KDL::Rotation>::Ptr add_rotation_input(const std::string& name) {
        auto it = inputs.find(name);
        if (it != inputs.end()) {
          if (it->second.type == Rotation) {
            return KDL::rotVec(KDL::vector(KDL::input(it->second.idx), KDL::input(it->second.idx + 1), KDL::input(it->second.idx + 2)), KDL::input(it->second.idx + 3));
          }
          
          throw std::invalid_argument("Can't add rotation input with name '" + name + "'. The name is already taken.");
        }

        bJointvectorCompleted = true;
        KDL::Expression<KDL::Rotation>::Ptr out = KDL::rotVec(KDL::vector(KDL::input(nextInputIndex), 
                                                                   KDL::input(nextInputIndex + 1), 
                                                                   KDL::input(nextInputIndex + 2)),
                                                       KDL::input(nextInputIndex + 3));
        inputs[name] = { name, Rotation, nextInputIndex };

        nextInputIndex += 4;
        return out;
      }

      // Add a frame input
      KDL::Expression<KDL::Frame>::Ptr add_frame_input(const std::string& name) {
        auto it = inputs.find(name);
        if (it != inputs.end()) {
          if (it->second.type == Frame) {
            return KDL::frame(KDL::rotVec(KDL::vector(KDL::input(it->second.idx), KDL::input(it->second.idx + 1), KDL::input(it->second.idx + 2)), KDL::input(it->second.idx + 3)), 
                              KDL::vector(KDL::input(it->second.idx + 4), KDL::input(it->second.idx + 5), KDL::input(it->second.idx + 6)));
          }
          
          throw std::invalid_argument("Can't add frame input with name '" + name + "'. The name is already taken.");
        }

        bJointvectorCompleted = true;
        KDL::Expression<KDL::Frame>::Ptr out = KDL::frame(KDL::rotVec(KDL::vector(KDL::input(nextInputIndex), 
                                                                                  KDL::input(nextInputIndex + 1), 
                                                                                  KDL::input(nextInputIndex + 2)),
                                                                      KDL::input(nextInputIndex + 3)),
                                                          KDL::vector(KDL::input(nextInputIndex + 4),
                                                                      KDL::input(nextInputIndex + 5),
                                                                      KDL::input(nextInputIndex + 6)));
        inputs[name] = { name, Frame, nextInputIndex };

        nextInputIndex += 7;
        return out;
      }

      std::vector<std::string> get_joint_inputs() const {
        return jointInputs;
      }

      std::map<const std::string, ScopeInput> get_inputs() const {
        return inputs;
      }

      // Set scalar input
      bool set_input(const std::string& name, double value, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            (it->second.type == Scalar || it->second.type == Joint) &&
            it->second.idx < inputVector.size()) {
          
          inputVector[it->second.idx] = value;
          return true;
        }
        return false;
      }

      // Set vector input
      bool set_input(const std::string& name, Eigen::Vector3d value, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            it->second.type == Vector &&
            it->second.idx + 2 < inputVector.size()) {
          
          inputVector[it->second.idx] = value[0];
          inputVector[it->second.idx + 1] = value[1];
          inputVector[it->second.idx + 2] = value[2];

          return true;
        }
        return false;
      }

      // Set rotation input from quaternion
      bool set_input(const std::string& name, Eigen::Quaterniond value, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            it->second.type == Rotation &&
            it->second.idx + 3 < inputVector.size()) {
          
          Eigen::AngleAxisd aa(value);
          Eigen::Vector3d ax = aa.axis();

          inputVector[it->second.idx] = ax[0];
          inputVector[it->second.idx + 1] = ax[1];
          inputVector[it->second.idx + 2] = ax[2];
          inputVector[it->second.idx + 3] = aa.angle();

          return true;
        }
        return false;
      }

      // Set rotation input from axis and angle
      bool set_input(const std::string& name, Eigen::Vector3d axis, double angle, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            it->second.type == Rotation &&
            it->second.idx + 3 < inputVector.size()) {
          
          inputVector[it->second.idx] = axis[0];
          inputVector[it->second.idx + 1] = axis[1];
          inputVector[it->second.idx + 2] = axis[2];
          inputVector[it->second.idx + 3] = angle;

          return true;
        }
        return false;
      }

      // Set frame input from Affine3d
      bool set_input(const std::string& name, Eigen::Affine3d value, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            it->second.type == Frame &&
            it->second.idx + 6 < inputVector.size()) {
          
          Eigen::AngleAxisd aa(value.rotation());
          Eigen::Vector3d ax = aa.axis();
          Eigen::Vector3d trans = value.translation();

          inputVector[it->second.idx] = ax[0];
          inputVector[it->second.idx + 1] = ax[1];
          inputVector[it->second.idx + 2] = ax[2];
          inputVector[it->second.idx + 3] = aa.angle();
          inputVector[it->second.idx + 4] = trans[0];
          inputVector[it->second.idx + 5] = trans[1];
          inputVector[it->second.idx + 6] = trans[2];

          return true;
        }
        return false;
      }

      // Set frame input from axis, angle and translation
      bool set_input(const std::string& name, Eigen::Vector3d axis, double angle, Eigen::Vector3d translation, Eigen::VectorXd& inputVector) const {
        auto it = inputs.find(name);
        if (it != inputs.end() && 
            it->second.type == Frame &&
            it->second.idx + 6 < inputVector.size()) {

          inputVector[it->second.idx] = axis[0];
          inputVector[it->second.idx + 1] = axis[1];
          inputVector[it->second.idx + 2] = axis[2];
          inputVector[it->second.idx + 3] = angle;
          inputVector[it->second.idx + 4] = translation[0];
          inputVector[it->second.idx + 5] = translation[1];
          inputVector[it->second.idx + 6] = translation[2];

          return true;
        }
        return false;
      }

      size_t get_input_size() const {
        return nextInputIndex;
      }

    private:
      std::map< std::string, KDL::Expression<double>::Ptr > double_references_;
      std::map< std::string, KDL::Expression<KDL::Vector>::Ptr > vector_references_;
      std::map< std::string, KDL::Expression<KDL::Rotation>::Ptr > rotation_references_;
      std::map< std::string, KDL::Expression<KDL::Frame>::Ptr > frame_references_;

      bool bJointvectorCompleted;
      size_t nextInputIndex;
      std::map<const std::string, ScopeInput> inputs;
      std::vector<std::string> jointInputs;
  };
}

#endif // GISKARD_SCOPE_HPP
